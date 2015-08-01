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
    buf[0] = '!';
    buf[1] = hc1;
    buf[2] = hc2;
    buf[3] = 1;
    buf[4] = 1;
    buf[5] = 1;
    buf[6] = 1;
    if(includeCRC) { buf[7] = computeSimpleCRC(buf, buflen); } // CRC computation should never fail here.
    return(true);
    }

// Decode from simple form (no auth/enc) from the uint8_t array.
// Returns number of bytes read if successful, 0 if not.
uint8_t CC1Alert::decodeSimple(const uint8_t *const buf, const uint8_t buflen, const bool includeCRC)
    {
    return(0); // FIXME: fail
    }





    }


