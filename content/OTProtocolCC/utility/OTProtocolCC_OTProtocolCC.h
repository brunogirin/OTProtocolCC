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



    }


#endif
