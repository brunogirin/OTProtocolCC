/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s): Damon Hart-Davis 2015
*/

#include "OTProtocolCC_OTProtocolCC.h"

#include <Arduino.h>
#include <OTRadioLink.h>

// Use namespaces to help avoid collisions.
namespace OTProtocolCC
    {

// Compute the (non-zero) CRC for simple messages, for encode or decode.
// Nominally looks at the message type to decide who many bytes to apply the CRC to.
// The result should match the actual CRC on decode,
// and can be used to set the CRC from on encode.
// Returns 0 (invalid) if the buffer is too short or the message otherwise invalid.
uint8_t CC1Base::computeSimpleCRC(const uint8_t *buf, uint8_t buflen)
    {
    // Assume a fixed message length.
    const uint8_t len = 7;

    if(buflen < len) { return(0); } // FAIL

    // Start with first (type) byte, which should always be non-zero.
    // NOTE: this does not start with a separate (eg -1) value, nor invert the result, to save time for these fixed length messages.
    uint8_t crc = buf[0];
    if(0 == crc) { return(0); } // FAIL.

    for(uint8_t i = 1; i < len; ++i)
        { crc = OTRadioLink::crc7_5B_update(crc, buf[i]); }

    // Replace a zero CRC value with a non-zero.
    if(0 != crc) { return(crc); }
    return(OTRadioLink::crc7_5B_update_nz_ALT);
    }

// Encode in simple form to the uint8_t array (no auth/enc).
// Returns number of bytes written if successful,
// 0 if not successful, eg because the buffer is too small.
uint8_t CC1Alert::encodeSimple(uint8_t *const buf, const uint8_t buflen, const bool includeCRC) const
    {
    if(!encodeSimpleArgsSane(buf, buflen, includeCRC)) { return(0); } // FAIL.
    buf[0] = frame_type; // OTRadioLink::FTp2_CC1Alert;
    buf[1] = hc1;
    buf[2] = hc2;
    buf[3] = 1;
    buf[4] = 1;
    buf[5] = 1;
    buf[6] = 1;
    if(!includeCRC) { return(7); }
    buf[7] = computeSimpleCRC(buf, buflen); // CRC computation should never fail here.
    return(8);
    }


// Decode from the wire, including CRC, into the current instance.
// Invalid parameters (eg 0xff house codes) will be rejected.
// Returns number of bytes read, 0 if unsuccessful; also check isValid().
uint8_t CC1Alert::decodeSimple(const uint8_t *const buf, const uint8_t buflen)
    {
    forceInvalid(); // Invalid by default.
    // Validate args.
    if(!decodeSimpleArgsSane(buf, buflen, true)) { return(0); } // FAIL.
    // Check frame type.
    if(frame_type /* OTRadioLink::FTp2_CC1Alert */ != buf[0]) { return(0); } // FAIL.
    // Explicitly test at least first extension byte is as expected.
    if(1 != buf[3]) { return(0); } // FAIL.
    // Check CRC.
    if(computeSimpleCRC(buf, buflen) != buf[7]) { return(0); } // FAIL.
    // Extract house code.
    hc1 = buf[1];
    hc2 = buf[2];
    // Instance will be valid if house code is.
    // Reads a fixed number of bytes when successful.
    return(8);
    }

// Factory method to create instance.
// Invalid parameters (except house codes) will be coerced into range.
//   * House code (hc1, hc2) of valve controller that the poll/command is being sent to.
//   * rad-open-percent     [0,100] 0-100 in 1% steps, percent open approx to set rad valve (rp)
//   * light-colour         [0,3] bit flags 1==red 2==green (lc) 0 => stop everything
//   * light-on-time        [1,15] (0 not allowed) 30-450s in units of 30s (lt) ???
//   * light-flash          [1,3] (0 not allowed) 1==single 2==double 3==on (lf)
// Returns instance; check isValid().
CC1PollAndCommand CC1PollAndCommand::make(const uint8_t hc1, const uint8_t hc2,
                                          const uint8_t rp,
                                          const uint8_t lc, const uint8_t lt, const uint8_t lf)
    {
    CC1PollAndCommand r;
    r.hc1 = hc1;
    r.hc2 = hc2;
    r.rp = constrain(rp, 0, 100);
    r.lc = lc & 3; // Logical bit pattern for LEDs.
    r.lt = constrain(lt, 1, 15);
    r.lf = constrain(lf, 1, 3);
    return(r);
    }

// Encode in simple form to the uint8_t array (no auth/enc).
// Returns number of bytes written if successful,
// 0 if not successful, eg because the buffer is too small.
//     '?' hc2 hc2 1+rp lf|lt|lc 1 1 nzcrc
uint8_t CC1PollAndCommand::encodeSimple(uint8_t *const buf, const uint8_t buflen, const bool includeCRC) const
    {
    if(!encodeSimpleArgsSane(buf, buflen, includeCRC)) { return(0); } // FAIL.
    buf[0] = frame_type; // OTRadioLink::FTp2_CC1Alert;
    buf[1] = hc1;
    buf[2] = hc2;
    buf[3] = rp + 1;
    buf[4] = (lf << 6) | ((lt << 2) & 0x3c) | (lc & 3);
    buf[5] = 1;
    buf[6] = 1;
    if(!includeCRC) { return(7); }
    buf[7] = computeSimpleCRC(buf, buflen); // CRC computation should never fail here.
    return(8);
    }

// Decode from the wire, including CRC, into the current instance.
// Invalid parameters (eg 0xff house codes) will be rejected.
// Returns number of bytes read, 0 if unsuccessful; also check isValid().
//     '?' hc2 hc2 1+rp lf|lt|lc 1 1 nzcrc
uint8_t CC1PollAndCommand::decodeSimple(const uint8_t *const buf, const uint8_t buflen)
    {
    forceInvalid(); // Invalid by default.
    // Validate args.
    if(!decodeSimpleArgsSane(buf, buflen, true)) { return(0); } // FAIL.
    // Check frame type.
    if(frame_type /* OTRadioLink::FTp2_CC1PollAndCommand */ != buf[0]) { return(0); } // FAIL.
    // Explicitly test at least first extension byte is as expected.
    if(1 != buf[5]) { return(0); } // FAIL.
    // Check inbound values for validity.
    const uint8_t _rp = buf[3] - 1;
    if(_rp >= 101) { return(0); } // FAIL.
    rp = _rp;
    // Extract light values.
    lc = buf[4] & 3;
    lt = (buf[4] >> 2) & 0xf;
    if(0 == lt) { return(0); } // FAIL.
    lf = (buf[4] >> 6) & 3;
    if(0 == lf) { return(0); } // FAIL.
    // Check CRC.
    if(computeSimpleCRC(buf, buflen) != buf[7]) { return(0); } // FAIL.
    // Extract house code last, leaving object invalid if bad value forced abort above.
    hc1 = buf[1];
    hc2 = buf[2];
    // Instance will be valid if house code is.
    // Reads a fixed number of bytes when successful.
    return(8);
    }


// Factory method to create instance.
//   * House code (hc1, hc2) of valve controller that the poll/command is being sent to.
//   * relative-humidity    [0,50] 0-100 in 2% steps (rh)
//   * temperature-ds18b20  [0,199] 0.000-99.999C in 1/2 C steps, pipe temp (tp)
//   * temperature-opentrv  [0,199] 0.000-49.999C in 1/4 C steps, room temp (tr)
//   * ambient-light        [1,62] no units, dark to light (al)
//   * switch               [false,true] activation toggle, helps async poll detect intermittent use (s)
//   * window               [false,true] false=closed,true=open (w)
//   * syncing              [false,true] if true, (re)syncing to FHT8V (sy)
// Returns instance; check isValid().
CC1PollResponse CC1PollResponse::make(const uint8_t hc1, const uint8_t hc2,
                                      const uint8_t rh,
                                      const uint8_t tp, const uint8_t tr,
                                      const uint8_t al,
                                      const bool s, const bool w, const bool sy)
    {
    CC1PollResponse r;
    r.hc1 = hc1;
    r.hc2 = hc2;
    r.rh = constrain(rh, 0, 50);
    r.tp = constrain(tp, 0, 199);
    r.tr = constrain(tr, 0, 199);
    r.al = constrain(al, 1, 62);
    r.s = s;
    r.w = w;
    r.sy = sy;
    return(r);
    }

// Encode in simple form to the uint8_t array (no auth/enc).
// Returns number of bytes written if successful,
// 0 if not successful, eg because the buffer is too small.
//     '*' hc2 hc2 w|s|1+rh 1+tp 1+tr sy|al|0 nzcrc
uint8_t CC1PollResponse::encodeSimple(uint8_t *const buf, const uint8_t buflen, const bool includeCRC) const
    {
    if(!encodeSimpleArgsSane(buf, buflen, includeCRC)) { return(0); } // FAIL.
    buf[0] = frame_type; // OTRadioLink::FTp2_CC1Alert;
    buf[1] = hc1;
    buf[2] = hc2;
    buf[3] = rh + 1;
    if(w) { buf[3] |= 0x80; }
    if(s) { buf[3] |= 0x40; }
    buf[4] = tp + 1;
    buf[5] = tr + 1;
    buf[6] = (al << 1);
    if(sy) { buf[6] |= 0x80; }
    if(!includeCRC) { return(7); }
    buf[7] = computeSimpleCRC(buf, buflen); // CRC computation should never fail here.
    return(8);
    }

// Decode from the wire, including CRC, into the current instance.
// Invalid parameters (eg 0xff house codes) will be rejected.
// Returns number of bytes read, 0 if unsuccessful; also check isValid().
//     '*' hc2 hc2 w|s|1+rh 1+tp 1+tr sy|al|0 nzcrc
uint8_t CC1PollResponse::decodeSimple(const uint8_t *const buf, const uint8_t buflen)
    {
    forceInvalid(); // Invalid by default.
    // Validate args.
    if(!decodeSimpleArgsSane(buf, buflen, true)) { return(0); } // FAIL.
    // Check frame type.
    if(frame_type /* OTRadioLink::FTp2_CC1PollResponse */ != buf[0]) { return(0); } // FAIL.
// TODO
    // Check inbound values for validity.
    // Extract RH%.
    const uint8_t _rh = (buf[3] & 0x3f);
    if((0 == _rh) || (_rh > 51)) { return(0); } // FAIL.
    rh = _rh - 1;
    w = (0 != (0x80 & buf[3]));
    s = (0 != (0x40 & buf[3]));
    const uint8_t _tp = buf[4] - 1;
    if(_tp >= 200) { return(0); } // FAIL.
    tp = _tp;
    const uint8_t _tr = buf[5] - 1;
    if(_tr >= 200) { return(0); } // FAIL.
    tr = _tr;
    const uint8_t _al = (buf[6] >> 1) & 0x3f;
    if((0 == _al) || (0x3f == _al)) { return(0); } // FAIL.
    al = _al;
    sy = (0 != (0x80 & buf[6]));
    // Check CRC.
    if(computeSimpleCRC(buf, buflen) != buf[7]) { return(0); } // FAIL.
    // Extract house code last, leaving object invalid if bad value forced abort above.
    hc1 = buf[1];
    hc2 = buf[2];
    // Instance will be valid if house code is.
    // Reads a fixed number of bytes when successful.
    return(8);
    }





    }


