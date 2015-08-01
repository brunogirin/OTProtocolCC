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
    // NO virtual destructor, so don't delete from point to any base class.
    class CC1Base
        {
        protected:
            // House code /ID for device.
            // Normally each byte is in the range [0,99].
            // Anything other than 0xff can be considered valid.
            uint8_t hc1, hc2;

        public:
            // True if the current state of this CC1 instance is valid.
            // Always false for the base instance.
            virtual bool isValid() const { return(false); }

            // Get house code 1; any non-0xff value is potentially valid.
            uint8_t getHC1() const { return(hc1); }
            // Get house code 2; any non-0xff value is potentially valid.
            uint8_t getHC2() const { return(hc2); }
            // True iff the house code is valid (ie neither byte is 0xff).
            bool houseCodeIsValid() const { return((0xff != hc1) && (0xff != hc2)); }

            // Encode in simple form to the uint8_t array (no auth/enc).
            // Returns number of bytes written if successful,
            // 0 if not successful, eg because the buffer is too small.
            //   * includeCRC  if true then append/set the trailing CRC;
            //     note that the call will fail and return 0 if the buffer is not large enough
            //     to accept the CRC as well as the body.
            virtual uint8_t encodeSimple(uint8_t *buf, uint8_t buflen, bool includeCRC) const = 0;

            // Decode from simple form (no auth/enc) from the uint8_t array.
            // Returns number of bytes read if successful, 0 if not.
            //   * includeCRC  if true then check the CRC
            //     and include it in the bytes-read count if successful
            virtual uint8_t decodeSimple(const uint8_t *buf, uint8_t buflen, bool includeCRC) = 0;

            // Compute the (non-zero) CRC for simple messages, for encode or decode.
            // Nominally looks at the message type to decide who many bytes to apply the CRC to.
            // The result should match the actual CRC on decode,
            // and can be used to set the CRC from on encode.
            // Returns CRC on success,
            // else 0 (invalid) if the buffer is too short or the message otherwise invalid.
            static uint8_t computeSimpleCRC(const uint8_t *buf, uint8_t buflen);
        };

    // CC1Alert contains:
    //   * House code (hc1, hc2) of valve controller that the alert is being sent from (or on behalf of).
    //   * Four extension bytes, currently reserved and of value 1.
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
    //     '!' hc2 hc2 1 1 1 1 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // Protocol note: sent asynchronously by the relay, though not generally at most once every 30s.
    // This message is simple enough that most of the methods can be inline.
    // This representation is immutable.
    class CC1Alert : public CC1Base
        {
        public:
            // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
            // The CRC7_5B is most effective at no more than 7 bytes.
            static const int primary_frame_bytes = 7;
            // Factory method to create instance.
            // Invalid parameters (eg 0xff house codes) will be rejected.
            // Returns instance; check isValid().
            static CC1Alert makeAlert(uint8_t hc1, uint8_t hc2) { CC1Alert r; r.hc1=hc1; r.hc2=hc2; return(r); }
            // True if the current state of this CC1 instance is valid.
            virtual bool isValid() const { return(houseCodeIsValid()); }
            // Encode/decode to/from uint8_t buffer.
            virtual uint8_t encodeSimple(uint8_t *buf, uint8_t buflen, bool includeCRC) const;
            virtual uint8_t decodeSimple(const uint8_t *buf, uint8_t buflen, bool includeCRC);
//        private:
//            CC1Alert(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1), hc2(_hc2) { }
        };

    // CC1PollAndCommand contains:
    //   * House code (hc1, hc2) of valve controller that the poll/command is being sent to.
    //   * rad-open-percent     [0,100] 0-100 in 1% steps, percent open approx to set rad valve (rp)
    //   * light-colour         [0,3] bit flags 1==red 2==green (lc) 0 => stop everything
    //   * light-on-time        [1,15] (0 not allowed) 30-450s in units of 30s (lt) ???
    //   * light-flash          [1,3] (0 not allowed) 1==single 2==double 3==on (lf)
    //   * Two extension bytes, currently reserved and of value 1.
    // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
    //     '?' hc2 hc2 rp lf|lt|lc 1 1 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // Protocol note: sent asynchronously by the hub to the relay, at least every 15m, generally no more than once per 30s.
    // Protocol note: after ~30m without hearing one of these from its hub a relay may go into fallback mode.
    // This representation is immutable.
    struct CC1PollAndCommand : public CC1Base
        {
//        CC1PollAndCommand(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1), hc2(_hc2) { }
//        const uint8_t hc1, hc2;
//        const uint8_t rp:7;
//        const uint8_t lc:2;
//        const uint8_t lt:4;
//        const uint8_t lf:2;
//        const uint8_t ext1, ext2;
        // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
        // The CRC7_5B is most effective at no more than 7 bytes.
        static const int primary_frame_bytes = 7;
        // Encode/decode.
        virtual uint8_t encodeSimple(uint8_t *buf, uint8_t buflen, bool includeCRC) const;
        virtual uint8_t decodeSimple(const uint8_t *buf, uint8_t buflen, bool includeCRC);
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
    //     '*' hc2 hc2 w|s|rh tp tr sy|al|0 crc
    // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
    // Protocol note: sent synchronously by the relay, within 10s of a poll/cmd from its hub.
    // This representation is immutable.
    struct CC1PollResponse : public CC1Base
        {
//        CC1PollResponse(uint8_t _hc1, uint8_t _hc2) : hc1(_hc1), hc2(_hc2)      { }
//        const uint8_t hc1, hc2;
//        const uint8_t rh;
//        const uint8_t tp;
//        const uint16_t tr;
//        const uint8_t aml:6;
//        const bool w;
//        const bool s;
//        const boot sy;
        // Length including leading type, but excluding trailing CRC (to allow other encapsulation).
        // The CRC7_5B is most effective at no more than 7 bytes.
        static const int primary_frame_bytes = 7;
        // Encode/decode.
        virtual uint8_t encodeSimple(uint8_t *buf, uint8_t buflen, bool includeCRC) const;
        virtual uint8_t decodeSimple(const uint8_t *buf, uint8_t buflen, bool includeCRC);
        };

    }


#endif
