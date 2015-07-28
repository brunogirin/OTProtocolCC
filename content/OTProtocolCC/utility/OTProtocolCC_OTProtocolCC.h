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
    //  c) The second byte is a length byte (nn) of the rest of the frame excluding CRC.
    //  d) nn bytes of data follow, of which the first two bytes will be the house code.
    //  e) The 7-bit CRC follows, arranged to never be 0x00 or 0xff.
    //  f) For the secure forms the message type and length and the house code will be part of the authenticated data.
    //
    // Use 7-bit CRC with Polynomial 0x5B (1011011, Koopman) = (x+1)(x^6 + x^5 + x^3 + x^2 + 1) = 0x37 (0110111, Normal).
    // See: http://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf
    // Should detect all 3-bit errors in up to 7 bytes of payload,
    // see: http://users.ece.cmu.edu/~koopman/crc/0x5b.txt
    // For 2 or 3 byte payloads this should have a Hamming distance of 4 and be within a factor of 2 of optimal error detection.


    // CC1Alert contains:
    //   * House code (hc1, hc2) of device alert is being sent from (or on behalf of).
    //   * Two extension bytes, currently reserved and of value 0.
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5V.
    //     '!' 4 hc2 hc2 0 0 crc
    // This representation is immutable.
    struct CC1Alert
        {
        CC1Alert(uint8_t _hc1, _hc2) : hc1(_hc1_), hc2(_hc2), ext1(0), ext2(0) { }
        const uint8_t hc1, hc2,
        const uint8_t ext1, ext2;
        };

    }


#endif
