#!/usr/bin/env python3

#
# Copyright 2021 Frank Jenner
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import argparse
import struct
import binascii

# Binary structure definitions used in the DFU file format
dfu_prefix_struct = struct.Struct('< 5s B I B')
dfu_suffix_struct = struct.Struct('< H H H H 3s B I')
target_prefix_struct = struct.Struct('< 6s B I 255s I I')
image_element_struct = struct.Struct('< I I')

def generate_dfu_prefix(image_size, num_targets=1):
    return dfu_prefix_struct.pack(
            b'DfuSe',
            1,
            image_size,
            num_targets)

def generate_dfu_suffix(crc32, fwver=0xffff, vid=0xffff, pid=0xffff):
    return dfu_suffix_struct.pack(
            fwver,
            pid,
            vid,
            0x011a,
            b'UFD',
            dfu_suffix_struct.size,
            crc32)

def generate_target_prefix(target_size, num_elements=1, alternate_setting=0, name=None):
    target_named = 1 if name else 0

    # Pad name with null bytes
    if not name:
        name = b''
    name += (b'\0' * (255 - len(name)))

    return target_prefix_struct.pack(
            b'Target',
            alternate_setting,
            target_named,
            name,
            target_size,
            num_elements)

def generate_image_element(start_addr, data_size):
    return image_element_struct.pack(start_addr, data_size)

def create_dfu(addr, filename_bin, filename_out):

    # Read in the input file data
    with open(filename_bin, 'rb') as binfile:
        image_data = binfile.read()

    # Generate the various headers for the DFU file format
    image_element_data = generate_image_element(addr, len(image_data))
    target_prefix_data = generate_target_prefix(len(image_element_data) + len(image_data))
    dfu_prefix_data = generate_dfu_prefix(dfu_prefix_struct.size + len(target_prefix_data) + len(image_element_data) + len(image_data))

    #
    # CRC calcuation includes the suffix block except for the CRC itself, so
    # we'll populate the suffix block first, perform the CRC calculation, and
    # then re-write the suffix using the correct CRC.
    #
    crc32 = 0
    dfu_suffix_data = generate_dfu_suffix(crc32)

    full_data = bytearray(
            dfu_prefix_data +
            target_prefix_data +
            image_element_data +
            image_data +
            dfu_suffix_data)

    # Calculate the actual CRC value and overwrite the suffix block
    crc32 = binascii.crc32(full_data[:-4]) ^ 0xffffffff
    dfu_suffix_data = generate_dfu_suffix(crc32)
    full_data[-len(dfu_suffix_data):] = dfu_suffix_data

    # Write the full contents to the output file
    with open(filename_out, 'wb') as outfile:
        outfile.write(full_data)

# Configure command line argument processing
parser = argparse.ArgumentParser()
parser.add_argument("addr", type=lambda x: int(x, 0), help="the target start address at which to load the firmware binary")
parser.add_argument("binfile", help="file containing the binary firmware image")
parser.add_argument("outfile", help="output file suitable for DFU bootloading")

# Generate the DFU file based on the command line options
args = parser.parse_args()
create_dfu(args.addr, args.binfile, args.outfile)

