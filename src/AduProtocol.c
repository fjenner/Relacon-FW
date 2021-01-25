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

#include "AduProtocol.h"
#include "Watchdog.h"
#include "EventCounter.h"
#include "boards/Board.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// The largest ADU command currently defined is the "MKddd" command
#define MAX_CMD_STR_SIZE    5
#define MAX_RSP_BUF_SIZE    8

#define NUM_RELAYS          8

#define INPUT_PORT_NUM_PINS 4
#define INPUT_PORT_A_SHIFT  0
#define INPUT_PORT_B_SHIFT  4
#define INPUT_PORT_A_MASK   0x0f
#define INPUT_PORT_B_MASK   0xf0

#define DEC_DIGITS_1_BIT    1
#define DEC_DIGITS_4_BIT    2
#define DEC_DIGITS_8_BIT    3
#define DEC_DIGITS_16_BIT   5

/** Buffer for storing the response to the latest command */
static uint8_t ResponseBuf[MAX_RSP_BUF_SIZE];

/** The size of the response data in the buffer */
static size_t ResponseBufLen;

/** Represents one of the digital input ports */
enum InputPort
{
    INPUT_PORT_UNDEFINED,
    INPUT_PORT_A,
    INPUT_PORT_B
};

/** Valid event counter debounce command settings */
enum DebounceSetting
{
    DEBOUNCE_SETTING_10MS,
    DEBOUNCE_SETTING_1MS,
    DEBOUNCE_SETTING_100US,
    DEBOUNCE_SETTING_NUM_SETTINGS
};

/** Mapping of debounce settings to debounce times */
static const uint32_t DEBOUNCE_TIMES_US[] =
{
    [DEBOUNCE_SETTING_10MS] = 10000,
    [DEBOUNCE_SETTING_1MS] = 1000,
    [DEBOUNCE_SETTING_100US] = 100
};

/** Valid watchdog command settings */
enum WatchdogSetting
{
    WATCHDOG_SETTING_DISABLED,
    WATCHDOG_SETTING_1SEC,
    WATCHDOG_SETTING_10SEC,
    WATCHDOG_SETTING_1MIN,
    WATCHDOG_SETTING_NUM_SETTINGS
};

/** Mapping of watchdog settings to watchdog timer periods */
static const uint32_t WATCHDOG_TIMES_US[] =
{
    [WATCHDOG_SETTING_DISABLED] = WATCHDOG_TIMEOUT_DISABLED,
    [WATCHDOG_SETTING_1SEC] = 1000000,
    [WATCHDOG_SETTING_10SEC] = 10000000,
    [WATCHDOG_SETTING_1MIN] = 60000000
};

/**
 * Converts a port designator character (case-insensitive) to a corresponding
 * enumerator
 *
 * @param[in] inputPortChar The port designation character
 *
 * @return The corresponding port enumerator
 */
static enum InputPort CharToInputPort(char inputPortChar)
{
    inputPortChar = toupper(inputPortChar);
    enum InputPort inputPort;

    switch (inputPortChar)
    {
        case 'A': inputPort = INPUT_PORT_A; break;
        case 'B': inputPort = INPUT_PORT_B; break;
        default: inputPort = INPUT_PORT_UNDEFINED; break;
    }

    return inputPort;
}

/**
 * Gets the value of the specified port. Since the input ports are only 4 bits
 * (and hence have a range of 0 to 0x0f), a return value of 0xff can be safely
 * used to indicate a failure.
 *
 * @param[in] inputPortChar The character of the port designation
 *
 * @return Returns the value of the port on success, or 0xff on failure
 */
static uint8_t GetInputPortValue(char inputPortChar)
{
    uint8_t ret = 0xff;

    // Determine the input port being requested
    enum InputPort inputPort = CharToInputPort(inputPortChar);

    if (inputPort == INPUT_PORT_UNDEFINED)
    {
        BoardDebugPrint("%s: Undefined port: %c\r\n", __func__, inputPortChar);
    }
    else
    {
        uint8_t combinedPortValue = BoardReadDigitalInputs();

        // Isolate the value of the requested port
        unsigned portShift = INPUT_PORT_A_SHIFT;
        unsigned portMask = INPUT_PORT_A_MASK;

        if (inputPort == INPUT_PORT_B)
        {
            portShift = INPUT_PORT_B_SHIFT;
            portMask = INPUT_PORT_B_MASK;
        }

        ret = (combinedPortValue & portMask) >> portShift;
    }

    return ret;
}

/**
 * Writes the specified value into the response buffer (and updates the
 * response buffer length accordingly) by formatting the value as a binary
 * string. The string will contain exactly @p numDigits binary digits, even if
 * the value could be represented with fewer digits or requires more digits.
 *
 * @pre The response buffer has been zeroed out prior to calling
 * @post The response buffer will be populated with the string value and the
 *       response buffer length updated accordingly
 *
 * @param[in] value The numeric value to write to the response buffer
 * @param[in] numDigits The number of binary digits to write into the response
 */
static void WriteResponseBinary(uint8_t value, uint8_t numDigits)
{
    for (unsigned i = 0; i < numDigits; i++)
    {
        ResponseBuf[i] = (value & (1 << (numDigits - i - 1))) ? '1' : '0';
    }
    
    ResponseBufLen = numDigits;
}

/**
 * Writes the specified value into the response buffer (and updates the
 * response buffer length accordingly) by formatting the value as a decimal
 * string. The string will contain exactly @p numDigits decimal digits, even if
 * the value could be represented with fewer digits or requires more digits.
 *
 * @pre The response buffer has been zeroed out prior to calling
 * @post The response buffer will be populated with the string value and the
 *       response buffer length updated accordingly
 *
 * @param[in] value The numeric value to write to the response buffer
 * @param[in] numDigits The number of decimal digits to write into the response
 */
static void WriteResponseDecimal(uint16_t value, uint8_t numDigits)
{
    for (int i = numDigits - 1; i >= 0; i--)
    {
        unsigned digit = value % 10;
        value /= 10;
        ResponseBuf[i] = '0' + digit;
    }

    ResponseBufLen = numDigits;
}

/**
 * Handler for the "RPy" or "RPyn" command, which responds with the status of
 * input line n (where n is '0', '1', '2', or '3') on port y (where y is 'A' or
 * 'B'). If n is not provided, the response contains the 4-bit binary value of
 * port y. If n is provided, the response contains a single binary value of the
 * requested line.
 *
 * @post On success, populates the response buffer with the port status
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadSinglePort(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;
    size_t len = strlen(args);

    // We only accept the formats "RPy" or "RPyn", where y is the port to read
    // ('A' or 'B') and n is the line to read ('0', '1', '2', or '3')
    if (len >= 1 && len <= 2)
    {
        uint8_t portValue = GetInputPortValue(args[0]);

        if (portValue != 0xff)
        {
            // If requesting a specific input line
            if (len == 2)
            {
                char *endptr;
                unsigned long line = strtoul(&args[1], &endptr, 10);

                // Check that the conversion was valid and in range
                if (*endptr == '\0' && line < INPUT_PORT_NUM_PINS)
                {
                    WriteResponseBinary((portValue & (1 << line)) != 0, 1);
                    success = true;
                }
                else
                {
                    BoardDebugPrint("%s: Invalid pin: %s\r\n", __func__, &args[1]);
                }

            }
            // If requesting all four input lines on the port
            else
            {
                WriteResponseBinary(portValue, INPUT_PORT_NUM_PINS);
                success = true;
            }
        }
    }
    else
    {
        BoardDebugPrint("%s: Invalid length %u\r\n", __func__, (unsigned)len);
    }

    return success;
}

/**
 * Handler for the "PAy" command, which responds with the status of port y
 * (where y is 'A' or 'B') as a decimal value.
 *
 * @post On success, populates the response buffer with the port status
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadSinglePortDecimal(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // We expect exactly one character argument
    if (strlen(args) == 1)
    {
        uint8_t portValue = GetInputPortValue(args[0]);
        if (portValue != 0xff)
        {
            WriteResponseDecimal(portValue, DEC_DIGITS_4_BIT);
            success = true;
        }
    }
    else
    {
        BoardDebugPrint("%s: Invalid command arguments: %s\r\n", __func__, args);
    }

    return success;
}

/**
 * Handler for the "PI" command, which responds with the concatenated status of
 * PORT A and PORT B (where PORT A forms bits 3:0 and PORT B forms bits 7:4) as
 * a decimal value.
 *
 * @post On success, populates the response buffer with the port status
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadCombinedPortsDecimal(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // There should be no character arguments to this command
    if (strlen(args) == 0)
    {
        WriteResponseDecimal(BoardReadDigitalInputs(), DEC_DIGITS_8_BIT);
        success = true;
    }

    return success;
}

/**
 * Handler for the "SKn" command, which closes the relay specified by n (where
 * n is a value from '0' to '7'). This command does not have a response.
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerSetRelay(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // There should be exactly one character argument to this command
    if (strlen(args) == 1)
    {
        char *endptr;
        unsigned long relay = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && relay < NUM_RELAYS)
        {
            // Set the bit corresponding to the specified relay
            uint8_t relayPort = BoardReadRelays();
            relayPort |= (1 << relay);
            BoardWriteRelays(relayPort);

            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "RKn" command, which opens the relay specified by n (where
 * n is a value from '0' to '7'). This command does not have a response.
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerClearRelay(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);
    bool success = false;

    // There should be exactly one character argument to this command
    if (strlen(args) == 1)
    {
        char *endptr;
        unsigned long relay = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && relay < NUM_RELAYS)
        {
            // Clear the bit corresponding to the specified relay
            uint8_t relayPort = BoardReadRelays();
            relayPort &= ~(1 << relay);
            BoardWriteRelays(relayPort);

            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "MKddd" command, which sets the 8-bit relay port to the
 * value indicated by the decimal string ddd. This command does not have a
 * response.
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerWriteRelayPort(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // There should be no more than 3 decimal characters/digital representing
    // the 8-bit value to write to the relay port
    if (strlen(args) <= 3)
    {
        char *endptr;
        unsigned long portValue = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && portValue <= UINT8_MAX)
        {
            BoardWriteRelays(portValue);
            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "RPKn" command, which responds with the current status of
 * relay n (where n ranges from '0' to '7').
 *
 * @post On success, the response buffer is populated
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadSingleRelay(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // There should be exactly one character argument to this command
    if (strlen(args) == 1)
    {
        char *endptr;
        unsigned long relay = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && relay < NUM_RELAYS)
        {
            uint8_t relayPort = BoardReadRelays();
            WriteResponseBinary((relayPort & (1 << relay)) != 0, 1);

            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "PK" command, which responds with the current status of
 * the 8-bit relay port as a decimal value.
 *
 * @post On success, the response buffer is populated
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadRelayPortDecimal(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    // There should be no arguments to this command
    if (strlen(args) == 0)
    {
        WriteResponseDecimal(BoardReadRelays(), DEC_DIGITS_8_BIT);
        success = true;
    }

    return success;
}

/**
 * Handler for the "REx" command, which responds with the current count, as a
 * decimal value of event counter n in (where n ranges from '0' to '7').
 *
 * @post On success, the response buffer is populated
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadEventCounter(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    if (strlen(args) == 1)
    {
        char *endptr;
        unsigned long index = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && index < EVENT_COUNTER_NUM_COUNTERS)
        {
            uint16_t count = EventCounterRead(index, false);
            WriteResponseDecimal(count, DEC_DIGITS_16_BIT);
            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "RCx" command, which responds with the current count, as a
 * decimal value of event counter n in (where n ranges from '0' to '7'). This
 * is the same as the "REx" command, except that the event count is also reset.
 *
 * @post On success, the response buffer is populated
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerReadAndResetEventCounter(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;

    if (strlen(args) == 1)
    {
        char *endptr;
        unsigned long index = strtoul(args, &endptr, 10);

        if (*endptr == '\0' && index < EVENT_COUNTER_NUM_COUNTERS)
        {
            uint16_t count = EventCounterRead(index, true);
            WriteResponseDecimal(count, DEC_DIGITS_16_BIT);
            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "DB" or "DBn" command, which either gets ("DB" command) or
 * sets ("DBn" command), the debounce setting for the event counters. The value
 * of n ranges from '0' to '2' as follows: '0' = 10ms, '1' = 1ms, '2' = 100us.
 *
 * @post On success, the response buffer is populated for the "DB" command
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerDebounceSetting(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;
    size_t len = strlen(args);

    if (len == 0)
    {
        // Query the debounce setting and populate the response
        uint32_t debounceTime = EventCounterDebounceTimeGet();
        for (unsigned i = 0; i < DEBOUNCE_SETTING_NUM_SETTINGS; i++)
        {
            if (DEBOUNCE_TIMES_US[i] == debounceTime)
            {
                WriteResponseDecimal(i, 1);
                success = true;
                break;
            }
        }
    }
    else if (len == 1)
    {
        // Set the debounce time based on the command argument
        char *endptr;
        unsigned long setting = strtoul(args, &endptr, 10);

        if (setting < DEBOUNCE_SETTING_NUM_SETTINGS)
        {
            EventCounterDebounceTimeSet(DEBOUNCE_TIMES_US[setting]);
            success = true;
        }
    }

    return success;
}

/**
 * Handler for the "WD" or "WDn" command, which either gets ("WD" command) or
 * sets ("WDn" command), the watchdog configuration. The value of n ranges
 * from '0' to '3' as follows: '0' = disabled, '1' = 1s, '2' = 10s, '3' = 1min.
 *
 * @post On success, the response buffer is populated for the "WD" command
 *
 * @param[in] args The non-fixed portion of the command string (if any)
 *
 * @return Returns true on success or false on failure
 */
static bool HandlerWatchdogSetting(const char *args)
{
    BoardDebugPrint("Hit %s\r\n", __func__);

    bool success = false;
    size_t len = strlen(args);

    if (len == 0)
    {
        // Fetch the current watchdog setting and populate the response
        uint32_t watchdogTimeout = WatchdogTimeoutGet();
        for (unsigned i = 0; i < WATCHDOG_SETTING_NUM_SETTINGS; i++)
        {
            if (WATCHDOG_TIMES_US[i] == watchdogTimeout)
            {
                WriteResponseDecimal(i, 1);
                success = true;
                break;
            }
        }
    }
    else if (len == 1)
    {
        // Update the current watchdog setting
        char *endptr;
        unsigned long setting = strtoul(args, &endptr, 10);

        if (setting < WATCHDOG_SETTING_NUM_SETTINGS)
        {
            WatchdogTimeoutSet(WATCHDOG_TIMES_US[setting]);
        }
    }

    return success;
}

/**
 * Command processor table entry. Associates a command handler function with
 * a command prefix string
 */
struct CommandProcessorEntry
{
    /** The command prefix string handled by this handler */
    const char *CommandPrefix;

    /**
     * The length of the command prefix (we could compute this dynamically with
     * strlen() each time we search the command table, but why waste the
     * cycles?)
     */
    size_t CommandPrefixLen;

    /**
     * The function to handle commands matching the specified prefix
     *
     * @param[in] args The portion of the command string containing argument
     *                 characters (if any)
     *
     * @return Returns true on success or false on failure
     */
    bool (*Handler)(const char *args);
};

/**
 * Simple macro to simplify the creation of entries in the command processors
 * table.
 *
 * @param[in] prefix The command prefix string
 * @param[in] handler The handler function for processing this type of command
 */
#define CMD_PROCESSOR_ENTRY(prefix, handler) \
    { prefix, sizeof(prefix) - 1, handler }

/** Table of handlers for processing each type of command */
static const struct CommandProcessorEntry ENTRIES[] =
{
    // Commands for writing or querying relay states
    // Note: The "RPK" command must appear in this table before the "RP"
    // command since the first matching command handler is used, and "RPK" is
    // a specialization of "RP" that requires a separate command handler.
    CMD_PROCESSOR_ENTRY("SK", HandlerSetRelay),
    CMD_PROCESSOR_ENTRY("RK", HandlerClearRelay),
    CMD_PROCESSOR_ENTRY("MK", HandlerWriteRelayPort),
    CMD_PROCESSOR_ENTRY("RPK", HandlerReadSingleRelay),
    CMD_PROCESSOR_ENTRY("PK", HandlerReadRelayPortDecimal),

    // Commands for reading digital input pins
    CMD_PROCESSOR_ENTRY("RP", HandlerReadSinglePort),
    CMD_PROCESSOR_ENTRY("PA", HandlerReadSinglePortDecimal),
    CMD_PROCESSOR_ENTRY("PI", HandlerReadCombinedPortsDecimal),

    // Commands dealing with the event counters
    CMD_PROCESSOR_ENTRY("RE", HandlerReadEventCounter),
    CMD_PROCESSOR_ENTRY("RC", HandlerReadAndResetEventCounter),
    CMD_PROCESSOR_ENTRY("DB", HandlerDebounceSetting),

    // Commands dealing with the watchdog timer
    CMD_PROCESSOR_ENTRY("WD", HandlerWatchdogSetting),
};

/** The number of entries in the command processor table */
static const size_t NUM_ENTRIES = sizeof(ENTRIES) / sizeof(ENTRIES[0]);

bool AduProtocolProcessCommand(const uint8_t *buf, size_t len)
{
    bool success = false;
    bool handlerFound = false;
    const char *args = (const char*)buf;

    // All ADU commands are short, NULL terminated strings, so verify that
    if (strnlen(args, len) > MAX_CMD_STR_SIZE)
    {
        BoardDebugPrint("%s: Command length too long\r\n", __func__);
    }
    else
    {
        // Clear any previous response information
        ResponseBufLen = 0;

        // Search the command table for a command that matches
        for (unsigned i = 0; i < NUM_ENTRIES && !handlerFound; i++)
        {
            const struct CommandProcessorEntry *entry = &ENTRIES[i];
            if (strncasecmp(args, entry->CommandPrefix, entry->CommandPrefixLen) == 0)
            {
                // Strip off the prefix before passing to the command handler
                args += entry->CommandPrefixLen;
                success = entry->Handler(args);

                handlerFound = true;
            }
        }

        if (!handlerFound)
        {
            BoardDebugPrint("%s: No matching handler for %s\r\n", __func__, args);
        }
    }

    // Handling any command should reset the watchdog timer
    WatchdogKick();

    return success;
}

int AduProtocolGetResponse(uint8_t *buf, size_t len)
{
    // Copy the response for the last command into the output buffer
    size_t copyLen = len < ResponseBufLen ? len : ResponseBufLen;
    memcpy(buf, ResponseBuf, ResponseBufLen);
    return copyLen;
}