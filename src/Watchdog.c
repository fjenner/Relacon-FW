/*
Copyright 2021 Frank Jenner

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Watchdog.h"
#include "boards/Board.h"
#include <stdint.h>

/** The time at which the watchdog was last kicked */
static uint32_t LastKickTimeUs;

/** The watchdog timeout period. A period of zero means watchdog is disabled */
static uint32_t WatchdogTimeoutPeriodUs;

void WatchdogInit()
{
    // Disable the watchdog timer by default
    WatchdogTimeoutPeriodUs = WATCHDOG_TIMEOUT_DISABLED;
}

void WatchdogTask()
{
    // If the watchdog is enabled
    if (WatchdogTimeoutPeriodUs != WATCHDOG_TIMEOUT_DISABLED)
    {
        uint32_t currentTimeUs = BoardGetElapsedTimeUs();

        // If the watchdog timed out, disable the watchdog timer and open all
        // the relays
        if (currentTimeUs - LastKickTimeUs > WatchdogTimeoutPeriodUs)
        {
            WatchdogTimeoutPeriodUs = WATCHDOG_TIMEOUT_DISABLED;
            BoardWriteRelays(0);

            BoardDebugPrint("%s: Watchdog timed out!\r\n", __func__);
        }
    }
}

void WatchdogKick()
{
    LastKickTimeUs = BoardGetElapsedTimeUs();
}

void WatchdogTimeoutSet(uint32_t timeoutUs)
{
    WatchdogTimeoutPeriodUs = timeoutUs;
}

uint32_t WatchdogTimeoutGet()
{
    return WatchdogTimeoutPeriodUs;
}