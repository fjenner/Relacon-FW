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

#ifndef EVENT_COUNTER_H
#define EVENT_COUNTER_H

#include <stdint.h>
#include <stdbool.h>

/** There is an event counter for each of the digital inputs */
#define EVENT_COUNTER_NUM_COUNTERS 8

/**
 * Initializes the event counter code, including resetting the event counters
 * and setting the default debounce configuration
 */
void EventCounterInit();

/**
 * Performs debouncing and records event counts
 */
void EventCounterTask();

/**
 * Gets the current count of the selected event counter, optionally clearing
 * the count atomically. The count is a 16-bit value that wraps back to zero
 * on overflow.
 *
 * @param[in] index The index of the event counter whose count to return
 * @param[in] resetAfterRead If true, resets the counter after reading
 *
 * @return Returns the count for this event
 */
uint16_t EventCounterRead(uint8_t index, bool resetAfterRead);

/**
 * Sets the debounce time used for the event counters. When a rising edge is
 * detected from an idle state, the debounce timer starts. Any additional
 * rising edges that occur before the debounce time elapses do not contribute
 * to the event count.
 *
 * @param[in] debounceTimeUs The debounce time, in microseconds
 */
void EventCounterDebounceTimeSet(uint32_t debounceTimeUs);

/**
 * Gets the debounce time used for the event counters
 *
 * @return The debounce time, in microseconds
 */
uint32_t EventCounterDebounceTimeGet();

#endif