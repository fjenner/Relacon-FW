#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/**
 * Performs board-specific initialization. This generally includes setting up
 * clocks trees, enabling peripheral clocks, configuring pins, installing
 * interrupt handlers, etc.
 */
void BoardInit();

/**
 * Returns the elapsed time, in microseconds, since board initialization. This
 * should be provided as a free-running 32-bit value that will roll over to
 * zero upon overflow.
 *
 * @return The elapsed time since board initialization, in microseconds
 */
uint32_t BoardGetElapsedTimeUs();

/**
 * Sets the state of the 8 relays, where the provided value represents that of
 * the protocol-level "PORTK" abstraction.
 *
 * @param relayState The desired state of the relays
 */
void BoardWriteRelays(uint8_t relayState);

/**
 * Reads the state of the 8 relays, corresponding to the protocol-level "PORTK"
 * abstraction.
 *
 * @return Returns the "PORTK" relays state
 */
uint8_t BoardReadRelays();

/**
 * Gets the state of the 8 digital input lines. Bits 7:4 should correspond to
 * "PORTB" bits 3:0 and bits 3:0 should correspond to "PORTA" bits 3:0, where
 * "PORTA" and "PORTB" are the *protocol-level* port abstraction, and don't
 * necessarily correspond to the actual port designations on the MCU.
 *
 * @return Returns the state of the 8 digital input lines
 */
uint8_t BoardReadDigitalInputs();

/**
 * Print debug logging output in a board-specific manner
 *
 * @param format Format string compatible with printf
 *
 * @return The number of characters written, or a negative value on error
 */
#ifdef ENABLE_UART_DEBUG
int BoardDebugPrint(const char *format, ...);
#else
#define BoardDebugPrint(...)
#endif

#endif