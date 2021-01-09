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

#include "tusb.h"

#define STRING_MANUFACTURER "Frank Jenner"
#define STRING_PRODUCT      "Relacon Relay Controller"

#define LANGID_ENGLISH      0x0409

/**
 * The device descriptor
 */
static tusb_desc_device_t const DEVICE_DESCRIPTOR =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_DESCRIPTORS_VENDOR_ID,
    .idProduct          = USB_DESCRIPTORS_PRODUCT_ID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

/**
 * Called from TinyUSB to retrieve the device descriptor
 *
 * @return Returns a pointer to the buffer containing the device descriptor
 */
const uint8_t * tud_descriptor_device_cb(void)
{
    return (const uint8_t*)&DEVICE_DESCRIPTOR;
}

/**
 * The HID report descriptor
 */
static const uint8_t HID_REPORT_DESCRIPTOR[] =
{
    HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2   ),

    // Collection for command/response reports (report ID 1)
    HID_USAGE        ( 0x01                       ),
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),

        // Input report
        HID_USAGE         ( 0xa6                                   ),
        HID_REPORT_ID     ( 1                                      )
        HID_USAGE         ( 0xa7                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_INPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

        // Output report
        HID_USAGE         ( 0xa9                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

    HID_COLLECTION_END,

    // Collection for RS232 reports (report ID 2; unused on this device)
    HID_USAGE        ( 0x02                       ),
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),

        // Input report
        HID_USAGE         ( 0xa5                                   ),
        HID_REPORT_ID     ( 2                                      )
        HID_USAGE         ( 0xaa                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_INPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

        // Output report
        HID_USAGE         ( 0xab                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

    HID_COLLECTION_END,
    
    // Collection for "streaming" reports (report ID 3; unused on this device)
    HID_USAGE        ( 0x03                       ),
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),

        // Input report
        HID_USAGE         ( 0xae                                   ),
        HID_REPORT_ID     ( 3                                      )
        HID_USAGE         ( 0xac                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_INPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

        // Output report
        HID_USAGE         ( 0xad                                   ),
        HID_LOGICAL_MIN   ( 0x00                                   ),
        HID_LOGICAL_MAX   ( 0x7f                                   ),
        HID_REPORT_SIZE   ( 8                                      ),
        HID_REPORT_COUNT  ( CFG_TUD_HID_EP_BUFSIZE - 1             ),
        HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

    HID_COLLECTION_END
};

/**
 * Called from TinyUSB to retrieve the HID report descriptor
 *
 * @return Returns a pointer to a buffer containing the HID report descriptor
 */
const uint8_t * tud_hid_descriptor_report_cb(void)
{
    return HID_REPORT_DESCRIPTOR;
}

/**
 * Total length of configuration descriptor, including subordinate descriptors
 */
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

/**
 * The configuration descriptor, including subordinate descriptors
 */
static const uint8_t CONFIGURATION_DESCRIPTOR[] =
{
    // Main configuration descriptor
    TUD_CONFIG_DESCRIPTOR(
        1,                  // Configuration index
        1,                  // Number of interfaces in this configuration
        0,                  // String descriptor index for configuration (none)
        CONFIG_TOTAL_LEN,   // Total length of this descriptor and all subordinate descriptors
        0,                  // Attributes (neither self-powered nor supporting remote wakeup)
        100                 // Maximum power in mA
    ),

    // Subordinate interface and endpoint descriptors for HID interface
    TUD_HID_INOUT_DESCRIPTOR(
        0,                  // Interface number of HID interface
        0,                  // String descriptor index for interface (none)
        HID_PROTOCOL_NONE,  // Protocol (HID)
        sizeof(HID_REPORT_DESCRIPTOR),
        0x01,               // HID interrupt OUT endpoint address
        0x01 | TUSB_DIR_IN_MASK, // HID interrupt IN endpoint address
        CFG_TUD_HID_EP_BUFSIZE, // HID interrupt endpoint packet size
        10                  // Interrupt endpoint service interval in ms
    )
};

/**
 * Called from TinyUSB to retrieve the configuration descriptor (including)
 * any subordinate descriptors
 *
 * @param[in] index The index of the configuration to retrieve
 *
 * @return Returns a pointer to the configuration descriptor
 */
const uint8_t * tud_descriptor_configuration_cb(uint8_t index)
{
    // We only support a single configuration, configuration 1. Strangely,
    // TinyUSB subtracts 1 from the requested configuration number before
    // passing it to this function, so "configuration 1" is actually "index 0".
    if (index != 0)
        return NULL;
    return CONFIGURATION_DESCRIPTOR;
}

/**
 * Array of string descriptors. Index 0 is a special case in USB, and should
 * contain a list of 16-bit language IDs supported by this device. We only
 * support the English language ID.
 */
static const char *STRING_DESCRIPTORS[] =
{
    (const char[]) { U16_TO_U8S_LE(LANGID_ENGLISH) },
    STRING_MANUFACTURER,
    STRING_PRODUCT,
    USB_DESCRIPTORS_STRING_SERIAL_NUM,
};

/**
 * Scratch buffer for converting between ASCII strings and the UTF-16LE encoded
 * strings used for string descriptors.
 */
static uint16_t stringDescriptorScratchBuffer[32];

/**
 * Called from TinyUSB to retrieve the requested string descriptor. USB strings
 * must be presented as UTF-16LE encoded strings, so this function must handle
 * the conversion from the ASCII strings stored in the table to unicode. It
 * also handles the special case of string index zero, which returns a list of
 * language IDs supported by this device rather than returning a string. The
 * returned buffer must have a sufficient lifetime to be valid until the
 * GET_DESCRIPTOR transfer is complete.
 *
 * @param[in] index The index of the string descriptor to return
 * @param[in] langid The language identified for the requested string
 *
 * @return Returns a pointer to the requested string descriptor
 */
const uint16_t * tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    // Bail out if requested index is out of range
    if (index >= TU_ARRAY_SIZE(STRING_DESCRIPTORS))
        return NULL;

    // Bail out if requested language is not supported
    if (index != 0 && langid != LANGID_ENGLISH)
        return NULL;

    uint8_t bDescriptorType = TUSB_DESC_STRING;
    uint8_t bLength = sizeof(bDescriptorType) + sizeof(bLength);

    if (index == 0)
    {
        // Copy the language ID value and update the length appropriate
        memcpy(&stringDescriptorScratchBuffer[1], STRING_DESCRIPTORS[0], sizeof(uint16_t));
        bLength += sizeof(uint16_t);
    }
    else
    {
        const char* str = STRING_DESCRIPTORS[index];

        // Truncate the string if it's too long for the scratch buffer
        unsigned numChars = strlen(str);
        if (numChars > TU_ARRAY_SIZE(stringDescriptorScratchBuffer) - 1)
            numChars = TU_ARRAY_SIZE(stringDescriptorScratchBuffer) - 1;

        // Convert ASCII string into UTF-16
        for (unsigned i = 0; i < numChars; i++)
        {
            stringDescriptorScratchBuffer[1 + i] = str[i];
        }

        bLength += (2 * numChars);
    }

    // The first 16-bit word of the string descriptor contains the bLength
    // field and the bDescriptor type field, so pack those in
    stringDescriptorScratchBuffer[0] = (bDescriptorType << 8) | (bLength << 0);

    return stringDescriptorScratchBuffer;
}
