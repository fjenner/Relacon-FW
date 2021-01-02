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

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

/** Treat a watchdog timeout of zero as "watchdog disabled" */
#define WATCHDOG_TIMEOUT_DISABLED 0

/**
 * Initialize the watchdog with default values
 */
void WatchdogInit();

/**
 * Should be called periodically to check for and process a watchdog timeout
 */
void WatchdogTask();

/**
 * Kicks the watchdog to restart the timeout counter
 */
void WatchdogKick();

/**
 * Sets the watchdog timeout period
 *
 * @param[in] timeoutUs The timeout period, in microseconds
 */
void WatchdogTimeoutSet(uint32_t timeoutUs);

/**
 * Gets the watchdog timeout period
 *
 * @return The timeout period in microseconds
 */
uint32_t WatchdogTimeoutGet();

#endif