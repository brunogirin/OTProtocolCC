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

/*
 * OpenTRV OTProtocolCC minimal Central Control protocol support library.
 */

#ifndef ARDUINO_LIB_OTPROTOCOLCC_OTPROTOCOLCC_H
#define ARDUINO_LIB_OTPROTOCOLCC_OTPROTOCOLCC_H

#include <stddef.h>
#include <stdint.h>


// Use namespaces to help avoid collisions.
namespace OTProtocolCC
    {
    // From OTRadioLink:
    //namespace OTRadioLink
    //    {
    //    // For V0p2 messages on an FS20 carrier (868.35MHz, OOK, 5kbps raw)
    //    // the leading byte received indicates the frame type that follows.
    //    enum FrameType_V0p2_FS20
    //        {
    //...
    //        // Messages for minimal central-control V1 (eg REV9 variant).
    //        FTp2_CC1Alert                = '!', // 0x21
    //        FTp2_CC1PollAndCmd           = '?', // 0x3f
    //        FTp2_CC1PollResponse         = '*', // 0x2a
    //...
    //        }
    //     }

    // General byte-level format of the (CC1) hub/relay messages: type len HC1 HC2 body* crc7nz
    //
    // In part to be compatible with existing custom use of the FS20 carrier (but not encoding),
    // the following will hold:
    //
    //  a) The first byte is one of '!', '?' or '*' to indicate the message type for the initial forms.
    //  b) (The first byte will later be one of '!', '?' or '*' ORed with '0x80' to indicate a secure message variant.)
    //  c) Length is implicit/fixed and always 7 bytes excluding the trailing CRC.
    //  d) nn bytes of data follow, of which the first two bytes will be the house code.
    //  e) The 7-bit CRC follows, arranged to never be 0x00 or 0xff.
    //  f) For the secure forms the message type and length and the house code will be part of the authenticated data.
    //
    // Use 7-bit CRC with Polynomial 0x5B (1011011, Koopman) = (x+1)(x^6 + x^5 + x^3 + x^2 + 1) = 0x37 (0110111, Normal).
    // See: http://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf
    // Should detect all 3-bit errors in up to 7 bytes of payload,
    // see: http://users.ece.cmu.edu/~koopman/crc/0x5b.txt
    // For 2 or 3 byte payloads this should have a Hamming distance of 4 and be within a factor of 2 of optimal error detection.

    // CC1Base
    // Base class for common operations.
    // NO virtual destructor, so don't delete from base class.
    class CC1Base
        {
        public:
            // Encode in simple form (no auth/enc).
            // Returns number of bytes written if successful, 0 if not.
            //   * includeCRC  if true then append the CRC.
            uint8_t encodeSimple(uint8_t *buf, uint8_t buflen, bool includeCRC) = 0;

            // Decode from simple form (no auth/enc).
            // Returns number of bytes read if successful, 0 if not.
            //   * includeCRC  if true then check the CRC.
            unit8_t decodeSimple(const uint8_t *buf, uint8_t buflen, bool includeCRC) = 0;
        };

    // CC1Alert contains:
    //   * House code (hc1, hc2) of valve controller that the alert is being sent from (or on behalf of).
    //   * Four extension bytes, currently reserved and of value 1.
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
    //     '!' hc2 hc2 1 1 1 1 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // This representation is immutable.
    struct CC1Alert : public CC1Base
        {
        CC1Alert(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1), hc2(_hc2) { }
        const uint8_t hc1, hc2,
//        const uint8_t ext1, ext2, ext3, ext4;
        // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
        // The CRC7_5B is most effective at no more than 7 bytes.
        static const int primary_frame_bytes = 7;
        };

    // CC1PollAndCommand contains:
    //   * House code (hc1, hc2) of valve controller that the poll/command is being sent to.
    //   * rad-open-percent     [0,100] 0-100 in 1% steps, percent open approx to set rad valve (rp)
    //   * light-colour         [0,3] bit flags 1==red 2==green (lc) 0 => stop everything
    //   * light-on-time        [1,15] (0 not allowed) 2-30s in units of 2s (lt)
    //   * light-flash          [1,3] (0 not allowed) 1==single 2==double 3==on (lf)
    //   * Two extension bytes, currently reserved and of value 1.
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
    //     '?' hc2 hc2 rp lf|lt|lc 1 1 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // This representation is immutable.
// *** Unresolved note from spreadsheet: colour 0-3 where 0 is off: steady off =0; single flash = 1; double flash = 2; steady on = 3: repeat (flash mode) every n seconds, where 30 <= n <= 600; e.g. 1-1-30 = colour 1, single flash, every 30s
    struct CC1PollAndCommand : public CC1Base
        {
        CC1PollAndCommand(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1_), hc2(_hc2) { }
        const uint8_t hc1, hc2,
        const uint8_t rp:7;
        const uint8_t lc:2;
        const uint8_t lt:4;
        const uint8_t lf:2;
//        const uint8_t ext1, ext2;
        // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
        // The CRC7_5B is most effective at no more than 7 bytes.
        static const int primary_frame_bytes = 7;
        };

    // CC1PollResponse contains:
    //   * House code (hc1, hc2) of valve controller that the poll/command is being sent to.
    //   * relative-humidity    [0,50] 0-100 in 2% steps (rh)
    //   * temperature-ds18b20  [0,199] 0.000-99.999C in 1/2 C steps, pipe temp (tp)
    //   * temperature-opentrv  [0,199] 0.000-49.999C in 1/4 C steps, room temp (tr)
    //   * ambient-light        [1,62] no units, dark to light (l)
    //   * switch               [false,true] activation toggle, helps async poll detect intermittent use (s)
    //   * window               [false,true] false=closed,true=open (w)
    //   * syncing              [false,true] if true, (re)syncing to FHT8V (sy)
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
    //     '*' hc2 hc2 w|s|rh tp tr sy|al|1 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // This representation is immutable.
    struct CC1PollResponse : public CC1Base
        {
        CC1PollResponse(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1_), hc2(_hc2)      { }
        const uint8_t hc1, hc2;
        const uint8_t rh;
        const uint8_t tp;
        const uint16_t tr;
        const uint8_t aml:6;
        const bool w;
        const bool s;
        const boot sy;
        // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
        // The CRC7_5B is most effective at no more than 7 bytes.
        static const int primary_frame_bytes = 7;
        };

    }


#endif
