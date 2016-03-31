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

/*Unit test routines for library code.
 */

// Arduino libraries imported here (even for use in other .cpp files).
//#include <SPI.h>

#define UNIT_TESTS

// Include libraries that this depends on.
#include <OTV0p2Base.h>
#include <OTRadioLink.h>

// Include the library under test.
#include <OTProtocolCC.h>


void setup()
  {
#ifdef ON_V0P2_BOARD
  // initialize serial communications at 4800 bps for typical use with V0p2 board.
  Serial.begin(4800);
#else
  // initialize serial communications at 9600 bps for typical use with (eg) Arduino UNO.
  Serial.begin(9600);
#endif
  }



// Error exit from failed unit test, one int parameter and the failing line number to print...
// Expects to terminate like panic() with flashing light can be detected by eye or in hardware if required.
static void error(int err, int line)
  {
  for( ; ; )
    {
    Serial.print(F("***Test FAILED*** val="));
    Serial.print(err, DEC);
    Serial.print(F(" =0x"));
    Serial.print(err, HEX);
    if(0 != line)
      {
      Serial.print(F(" at line "));
      Serial.print(line);
      }
    Serial.println();
//    LED_HEATCALL_ON();
//    tinyPause();
//    LED_HEATCALL_OFF();
//    sleepLowPowerMs(1000);
    delay(1000);
    }
  }

// Deal with common equality test.
static inline void errorIfNotEqual(int expected, int actual, int line) { if(expected != actual) { error(actual, line); } }
// Allowing a delta.
static inline void errorIfNotEqual(int expected, int actual, int delta, int line) { if(abs(expected - actual) > delta) { error(actual, line); } }

// Test expression and bucket out with error if false, else continue, including line number.
// Macros allow __LINE__ to work correctly.
#define AssertIsTrueWithErr(x, err) { if(!(x)) { error((err), __LINE__); } }
#define AssertIsTrue(x) AssertIsTrueWithErr((x), 0)
#define AssertIsEqual(expected, x) { errorIfNotEqual((expected), (x), __LINE__); }
#define AssertIsEqualWithDelta(expected, x, delta) { errorIfNotEqual((expected), (x), (delta), __LINE__); }


// Check that correct version of this library is under test.
static void testLibVersion()
  {
  Serial.println("LibVersion");
#if !(0 == ARDUINO_LIB_OTPROTOCOLCC_VERSION_MAJOR) || !(3 == ARDUINO_LIB_OTPROTOCOLCC_VERSION_MINOR)
#error Wrong library version!
#endif
  }

// Check that correct versions of underlying libraries are in use.
static void testLibVersions()
  {
  Serial.println("LibVersions");
#if !(1 == ARDUINO_LIB_OTV0P2BASE_VERSION_MAJOR) || !(0 <= ARDUINO_LIB_OTV0P2BASE_VERSION_MINOR)
#error Wrong OTV0p2Base library version!
#endif  
#if !(1 == ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR) || !(0 <= ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR)
#error Wrong OTRadioLink library version!
#endif
  }

// Do some basic testing around the CRC.
static void testCommonCRC()
  {
  Serial.println("CommonCRC");
  uint8_t buf[13]; // More than long enough.
  // Test that a zero leading byte is rejected with a zero result.
  buf[0] = 0;
  AssertIsEqual(0, OTProtocolCC::CC1Base::computeSimpleCRC(buf, sizeof(buf)));
  // Test that a plausible non-zero byte and long-enough buffer is non-zero.
  buf[0] = '!';
  AssertIsTrue(0 != OTProtocolCC::CC1Base::computeSimpleCRC(buf, sizeof(buf)));
  // Test that a plausible non-zero byte and not-long-enough buffer is rejected with a zero result.
  buf[0] = '!';
  AssertIsEqual(0, OTProtocolCC::CC1Base::computeSimpleCRC(buf, 1));
  AssertIsEqual(0, OTProtocolCC::CC1Base::computeSimpleCRC(buf, 6));
  AssertIsTrue(0 != OTProtocolCC::CC1Base::computeSimpleCRC(buf, OTProtocolCC::CC1Alert::primary_frame_bytes)); // Should be long enough.
  // CC1Alert contains:
  //   * House code (hc1, hc2) of valve controller that the alert is being sent from (or on behalf of).
  //   * Four extension bytes, currently reserved and of value 1.
  // Should generally be fixed length on the wire, and protected by non-zero version of CRC7_5B.
  //     '!' hc2 hc2 1 1 1 1 crc
  // Note that most values are whitened to be neither 0x00 nor 0xff on the wire.
  // Minimal alert with zero house codes.
  uint8_t bufAlert0[] = {'!', 0, 0, 1, 1, 1, 1};
  AssertIsEqual(80, OTProtocolCC::CC1Base::computeSimpleCRC(bufAlert0, sizeof(bufAlert0)));
  // Minimal alert with non-zero house codes.
  uint8_t bufAlert1[] = {'!', 10, 21, 1, 1, 1, 1};
  AssertIsEqual(55, OTProtocolCC::CC1Base::computeSimpleCRC(bufAlert1, sizeof(bufAlert1)));
  }

// Do some basic testing of the CC1 Alert object.
static void testCC1Alert()
  {
  Serial.println("CC1Alert");
  const OTProtocolCC::CC1Alert a0 = OTProtocolCC::CC1Alert::make(0xff, 0);
  AssertIsTrue(!a0.isValid());
  const OTProtocolCC::CC1Alert a1 = OTProtocolCC::CC1Alert::make(10, 21);
  AssertIsTrue(a1.isValid());
  AssertIsEqual(10, a1.getHC1());
  AssertIsEqual(21, a1.getHC2());
  // Try encoding a simple alert.
  uint8_t buf[13]; // More than long enough.
  AssertIsEqual(8, a1.encodeSimple(buf, sizeof(buf), true));
  AssertIsEqual('!', buf[0]); // FTp2_CC1Alert.
  AssertIsEqual(10,  buf[1]);
  AssertIsEqual(21,  buf[2]);
  AssertIsEqual(1,   buf[3]);
  AssertIsEqual(1,   buf[4]);
  AssertIsEqual(1,   buf[5]);
  AssertIsEqual(1,   buf[6]);
  AssertIsEqual(55,  buf[7]);
  // Decode alert with non-zero house codes.
  uint8_t bufAlert0[] = {'!', 99,99, 1, 1, 1, 1, 12};
  OTProtocolCC::CC1Alert a2;
  // Bare instance should be invalid by default.
  AssertIsTrue(!a2.isValid());
  a2.OTProtocolCC::CC1Alert::decodeSimple(bufAlert0, sizeof(bufAlert0));
  // After decode instance should be valid and with correct house code.
  AssertIsTrue(a2.isValid());
  AssertIsEqual(99, a2.getHC1());
  AssertIsEqual(99, a2.getHC2());
  // Check that corrupting any single bit causes message rejection.
  bufAlert0[OTV0P2BASE::randRNG8() & 7] ^= (1 << (OTV0P2BASE::randRNG8() & 7));
  a2.OTProtocolCC::CC1Alert::decodeSimple(bufAlert0, sizeof(bufAlert0));
  AssertIsTrue(!a2.isValid());
  }

// Do some basic testing of the CC1 Poll-and-Command object.
static void testCC1PAC()
  {
  Serial.println("CC1PAC");
  const OTProtocolCC::CC1PollAndCommand a0 = OTProtocolCC::CC1PollAndCommand::make(0xff, 0, 0, 0, 0, 0);
  AssertIsTrue(!a0.isValid());
  const OTProtocolCC::CC1PollAndCommand a1 = OTProtocolCC::CC1PollAndCommand::make(10, 21, 1, 2, 3, 1);
  AssertIsTrue(a1.isValid());
  AssertIsEqual(10, a1.getHC1());
  AssertIsEqual(21, a1.getHC2());
  AssertIsEqual(1, a1.getRP());
  AssertIsEqual(2, a1.getLC());
  AssertIsEqual(3, a1.getLT());
  AssertIsEqual(1, a1.getLF());
  // Try encoding a simple instance.
  uint8_t buf[13]; // More than long enough.
  AssertIsEqual(8, a1.encodeSimple(buf, sizeof(buf), true));
  AssertIsEqual('?', buf[0]); // FTp2_CC1PollAndCmd.
  AssertIsEqual(10,  buf[1]);
  AssertIsEqual(21,  buf[2]);
  AssertIsEqual(2,   buf[3]);
  AssertIsEqual((1 << 6) | (3 << 2) | (2), buf[4]);
  AssertIsEqual(1,   buf[5]);
  AssertIsEqual(1,   buf[6]);
  AssertIsEqual(92,  buf[7]);
  // Decode instance.
  OTProtocolCC::CC1PollAndCommand a2;
  // Bare instance should be invalid by default.
  AssertIsTrue(!a2.isValid());
  a2.OTProtocolCC::CC1PollAndCommand::decodeSimple(buf, sizeof(buf));
  // After decode instance should be valid and with correct house code.
  AssertIsTrue(a2.isValid());
  AssertIsEqual(10, a2.getHC1());
  AssertIsEqual(21, a2.getHC2());
  AssertIsEqual(1, a2.getRP());
  AssertIsEqual(2, a2.getLC());
  AssertIsEqual(3, a2.getLT());
  AssertIsEqual(1, a2.getLF());
  // Check that corrupting any single bit causes message rejection.
  buf[OTV0P2BASE::randRNG8() & 7] ^= (1 << (OTV0P2BASE::randRNG8() & 7));
  a2.OTProtocolCC::CC1PollAndCommand::decodeSimple(buf, sizeof(buf));
  AssertIsTrue(!a2.isValid());
  }

// Do some basic testing of the CC1 Poll-Response object.
static void testCC1PR()
  {
  Serial.println("CC1PR");
  const OTProtocolCC::CC1PollResponse a0 = OTProtocolCC::CC1PollResponse::make(0xff, 0, 0, 0, 0, 0, false, false, false);
  AssertIsTrue(!a0.isValid());
  const OTProtocolCC::CC1PollResponse a1 = OTProtocolCC::CC1PollResponse::make(10, 21, 45, 160, 101, 35, true, false, false);
  AssertIsTrue(a1.isValid());
  AssertIsEqual(10, a1.getHC1());
  AssertIsEqual(21, a1.getHC2());
  AssertIsEqual(45, a1.getRH());
  AssertIsEqual(160, a1.getTP());
  AssertIsEqual(101, a1.getTR());
  AssertIsEqual(35, a1.getAL());
  AssertIsEqual(true, a1.getS());
  AssertIsEqual(false, a1.getW());
  AssertIsEqual(false, a1.getSY());
  // Try encoding a simple instance.
  uint8_t buf[13]; // More than long enough.
  AssertIsEqual(8, a1.encodeSimple(buf, sizeof(buf), true));
  AssertIsEqual('*', buf[0]); // FTp2_CC1PollResponse.
  AssertIsEqual(10,  buf[1]);
  AssertIsEqual(21,  buf[2]);
  AssertIsEqual(46 | 0x40, buf[3]);
  AssertIsEqual(161, buf[4]);
  AssertIsEqual(102, buf[5]);
  AssertIsEqual(70,  buf[6]);
  AssertIsEqual(1,   buf[7]);
  // Decode instance.
  OTProtocolCC::CC1PollResponse a2;
  // Bare instance should be invalid by default.
  AssertIsTrue(!a2.isValid());
  a2.OTProtocolCC::CC1PollResponse::decodeSimple(buf, sizeof(buf));
  // After decode instance should be valid and with correct house code.
  AssertIsTrue(a2.isValid());
  AssertIsEqual(10, a2.getHC1());
  AssertIsEqual(21, a2.getHC2());
  AssertIsEqual(45, a2.getRH());
  AssertIsEqual(160, a2.getTP());
  AssertIsEqual(101, a2.getTR());
  AssertIsEqual(35, a2.getAL());
  AssertIsEqual(true, a2.getS());
  AssertIsEqual(false, a2.getW());
  AssertIsEqual(false, a2.getSY());
  // Check that corrupting any single bit causes message rejection.
  buf[OTV0P2BASE::randRNG8() & 7] ^= (1 << (OTV0P2BASE::randRNG8() & 7));
  a2.OTProtocolCC::CC1PollResponse::decodeSimple(buf, sizeof(buf));
  AssertIsTrue(!a2.isValid());
  }





// To be called from loop() instead of main code when running unit tests.
// Tests generally flag an error and stop the test cycle with a call to panic() or error().
void loop()
  {
  static int loopCount = 0;

  // Allow the terminal console to be brought up.
  for(int i = 3; i > 0; --i)
    {
//    Serial.print(F("Tests (compiled "__TIME__") starting... " )); // FIXME! commented this as it was preventing compilation and is not present in other test.inos
    Serial.print(i);
    Serial.println();
    delay(1000);
    }
  Serial.println();


  // Run the tests, fastest / newest / most-fragile / most-interesting first...
  testLibVersion();
  testLibVersions();

  testCommonCRC();
  testCC1Alert();
  testCC1PAC();
  testCC1PR();


  // Announce successful loop completion and count.
  ++loopCount;
  Serial.println();
  Serial.print(F("%%% All tests completed OK, round "));
  Serial.print(loopCount);
  Serial.println();
  Serial.println();
  Serial.println();
  delay(2000);
  }
