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
  // initialize serial communications at 9600 bps for typical use with (eg) Arduino UNO.
  Serial.begin(9600); 
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
#if !(0 == ARDUINO_LIB_OTPROTOCOLCC_VERSION_MAJOR) || !(1 == ARDUINO_LIB_OTPROTOCOLCC_VERSION_MINOR)
#error Wrong library version!
#endif
  }

// Check that correct versions of underlying libraries are in use.
static void testLibVersions()
  {
  Serial.println("LibVersions");
#if !(0 == ARDUINO_LIB_OTV0P2BASE_VERSION_MAJOR) || !(5 <= ARDUINO_LIB_OTV0P2BASE_VERSION_MINOR)
#error Wrong OTV0p2Base library version!
#endif  
#if !(0 == ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR) || !(5 <= ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR)
#error Wrong OTRadioLink library version!
#endif
  }

// Do some basic testing.
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
  static uint8_t bufAlert0[] = {'!', 0, 0, 1, 1, 1, 1};
  AssertIsEqual(80, OTProtocolCC::CC1Base::computeSimpleCRC(bufAlert0, sizeof(bufAlert0)));
  // Minimal alert with non-zero house codes.
  static uint8_t bufAlert1[] = {'!', 10, 21, 1, 1, 1, 1};
  AssertIsEqual(55, OTProtocolCC::CC1Base::computeSimpleCRC(bufAlert1, sizeof(bufAlert1)));
  }







// To be called from loop() instead of main code when running unit tests.
// Tests generally flag an error and stop the test cycle with a call to panic() or error().
void loop()
  {
  static int loopCount = 0;

  // Allow the terminal console to be brought up.
  for(int i = 3; i > 0; --i)
    {
    Serial.print(F("Tests (compiled "__TIME__") starting... " ));
    Serial.print(i);
    Serial.println();
    delay(1000);
    }
  Serial.println();


  // Run the tests, fastest / newest / most-fragile / most-interesting first...
  testLibVersion();
  testLibVersions();

  testCommonCRC();


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
