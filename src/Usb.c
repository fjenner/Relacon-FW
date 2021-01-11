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

#include "Usb.h"
#include "tusb.h"
#include "boards/Board.h"
#include "AduProtocol.h"

/** The normal ADU commands/responses use HID report ID 1 */
#define REPORT_ID_ADU_CMD_RSP   1

/**
 * TinyUSB callback invoked when receiving a GET_REPORT control request. The
 * implementation should respond by either populating the buffer data and
 * returning its length, or by returning zero to cause the stack to STALL the
 * request
 *
 * @param[in] report_id The identifier of the requested report
 * @param[in] report_type The report type
 * @param[out] buffer The report buffer to populate
 * @param[in] reqlen The requested data length
 *
 * @return Returns the length of the buffer that was populated, or zero to
 *         STALL the request
 */
uint16_t tud_hid_get_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    uint16_t ret = 0;

    if (report_type == HID_REPORT_TYPE_INPUT &&
        report_id == REPORT_ID_ADU_CMD_RSP)
    {
        // Get the response to the last command
        ret = AduProtocolGetResponse(buffer, sizeof(reqlen));

        // Zero-pad the remainder of the report if not filled completely
        if (ret < reqlen)
        {
            memset(&buffer[ret], 0, reqlen - ret);
            ret = reqlen;
        }
    }

    return ret;
}

/**
 * TinyUSB callback invoked when receiving a SET_IDLE control request. If not
 * overridden, the stack will simply do nothing but still acknowledge the
 * request. The request can instead respond with a STALL if the function is
 * overridden and returns false.
 *
 * @param[in] idle_rate The duration, in 4ms increments, between reports
 *
 * @return Returns true if the request is honored or false to STALL the request
 */
bool tud_hid_set_idle_cb(uint8_t idle_rate)
{
    // Behave like the ADU218 and STALL any SET_IDLE requests
    return false;
}

/**
 * TinyUSB callback invoked when receiving a SET_REPORT request or a report on
 * the HID OUT endpoint. In the latter case, the @p report_type will be passed
 * in as HID_REPORT_TYPE_INVALID (and the @p report_id as zero), and the report
 * ID must be extracted from the first byte of the report payload.
 *
 * @param[in] report_id The report ID to set (only valid on a SET_REPORT
 *                      request on endpoint 0; otherwise, for OUT transactions
 *                      on the HID OUT endpoint, the report ID must be
 *                      extracted from the first byte of the payload)
 * @param[in] report_type The report type
 * @param[in] buffer The report payload data
 * @param[in] bufsize The length of the provided payload report data
 */
void tud_hid_set_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    BoardDebugPrint("tud_hid_set_report_cb(report_id=%u, report_type=%u, buffer[0]=0x%02x, bufsize=%u)\r\n", (unsigned)report_id, (unsigned)report_type, buffer[0], bufsize);

    // If this was really an OUT transfer on the interrupt endpoint, extract
    // the report ID and adjust the interpretation of the other parameters
    if (report_type == HID_REPORT_TYPE_INVALID)
    {
        report_id = buffer[0];
        report_type = HID_REPORT_TYPE_OUTPUT;
        buffer = &buffer[1];
        bufsize--;
    }

    // We only handle output report ID one
    if (report_type == HID_REPORT_TYPE_OUTPUT &&
        report_id == REPORT_ID_ADU_CMD_RSP)
    {
        // Send the report payload to the ADU command processor
        bool success = AduProtocolProcessCommand(buffer, bufsize);

        if (!success)
        {
            BoardDebugPrint("%s: Failed processing command %s\r\n", __func__, buffer);
        }
        else
        {
            // See whether there is a response to send back to the host
            uint8_t rspBuf[CFG_TUD_HID_EP_BUFSIZE];
            int rspLen = AduProtocolGetResponse(rspBuf, sizeof(rspBuf));

            if (rspLen > 0)
            {
                BoardDebugPrint("%s: Sending response with length %d\r\n", __func__, rspLen);
                // Pad the remainder of the report with zeros
                memset(&rspBuf[rspLen], 0, sizeof(rspBuf) - rspLen);
                tud_hid_report(REPORT_ID_ADU_CMD_RSP, rspBuf, sizeof(rspBuf));
            }
        }
    }
}

void UsbInit()
{
    tusb_init();
}

void UsbTask()
{
    tud_task();
}