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

#include "EventCounter.h"
#include "boards/Board.h"

/** Use 1ms debounce period by default */
#define DEFAULT_DEBOUNCE_TIME_US 1000

enum DebounceState
{
    DEBOUNCE_STATE_WAITING_FOR_RISING_EDGE,
    DEBOUNCE_STATE_SETTLING,
    DEBOUNCE_STATE_WAITING_FOR_FALLING_EDGE,
};

struct EventCounter
{
    enum DebounceState State;
    uint32_t RisingEdgeTime;
    uint16_t Count;
};

/** Event counters for each of the digital inputs */
static struct EventCounter Counters[EVENT_COUNTER_NUM_COUNTERS];

/** The debounce time for the input pins */
static uint32_t DebounceTimeUs;

void EventCounterInit()
{
    // Initialize all of the event counters to zero
    for (unsigned i = 0; i < EVENT_COUNTER_NUM_COUNTERS; i++)
    {
        Counters[i].Count = 0;
        Counters[i].State = DEBOUNCE_STATE_WAITING_FOR_RISING_EDGE;
    }

    DebounceTimeUs = DEFAULT_DEBOUNCE_TIME_US;
}

void EventCounterTask()
{
    uint32_t sampleTime = BoardGetElapsedTimeUs();
    uint8_t inputs = BoardReadDigitalInputs();

    for (unsigned i = 0; i < EVENT_COUNTER_NUM_COUNTERS; i++)
    {
        struct EventCounter *counter = &Counters[i];
        bool inputAsserted = (inputs & (1 << i)) != 0;

        switch (counter->State)
        {
            case DEBOUNCE_STATE_WAITING_FOR_RISING_EDGE:
                if (inputAsserted)
                {
                    counter->Count++;
                    counter->RisingEdgeTime = sampleTime;
                    counter->State = DEBOUNCE_STATE_SETTLING;
                }
                break;

            case DEBOUNCE_STATE_SETTLING:
                // Don't change state until the debounce timer has elapsed
                if (sampleTime - counter->RisingEdgeTime > DebounceTimeUs)
                {
                    if (inputAsserted)
                        counter->State = DEBOUNCE_STATE_WAITING_FOR_FALLING_EDGE;
                    else
                        counter->State = DEBOUNCE_STATE_WAITING_FOR_RISING_EDGE;
                }
                break;

            case DEBOUNCE_STATE_WAITING_FOR_FALLING_EDGE:
                if (!inputAsserted)
                    counter->State = DEBOUNCE_STATE_WAITING_FOR_RISING_EDGE;
                break;
        }
    }
}

uint16_t EventCounterRead(uint8_t index, bool resetAfterRead)
{
    uint16_t ret = 0;

    if (index < EVENT_COUNTER_NUM_COUNTERS)
    {
        ret = Counters[index].Count;
        if (resetAfterRead)
            Counters[index].Count = 0;
    }

    return ret;
}

void EventCounterDebounceTimeSet(uint32_t debounceTimeUs)
{
    DebounceTimeUs = debounceTimeUs;
}

uint32_t EventCounterDebounceTimeGet()
{
    return DebounceTimeUs;
}
