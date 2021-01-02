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

#ifndef ADU_PROTOCOL_H
#define ADU_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Processes the command in the provided buffer.
 *
 * @param[in] buf The buffer containing the command data
 * @param[in] len The length of the command data
 *
 * @return Returns true on success or false on failure
 */
bool AduProtocolProcessCommand(const uint8_t *buf, size_t len);

/**
 * Fetches the response from the latest command. Responses that are not read
 * by the host are discarded and overwritten each time the host sends a new
 * command. Commands that do not elicit a response will return a length of
 * zero.
 *
 * @param[out] buf The buffer to populate with the response data
 * @param[in] len The length of the provided buffer
 *
 * @return Returns the number of bytes written to the buffer on success, zero
 *         if there is no response data for the last command, or a negative
 *         value on error.
 */
int AduProtocolGetResponse(uint8_t *buf, size_t len);

#endif