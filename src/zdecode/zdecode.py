#!/usr/bin/env python3
# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import struct

def main():
    print("ZOSCII Decoder")
    print("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n")
    
    bittage = 16  # default
    offset = 0
    
    if len(sys.argv) >= 2 and sys.argv[1] == "-32":
        bittage = 32
        offset = 1
    elif len(sys.argv) >= 2 and sys.argv[1] == "-16":
        bittage = 16
        offset = 1
    
    if len(sys.argv) != 4 + offset:
        print(f"Usage: {sys.argv[0]} [-16|-32] <romfile> <encodedinput> <outputdatafile>", file=sys.stderr)
        return 1
    
    # Read ROM file
    try:
        with open(sys.argv[1 + offset], 'rb') as f:
            rom_data = f.read()
    except IOError as e:
        print(f"Error opening ROM file: {e}", file=sys.stderr)
        return 1
    
    rom_size = len(rom_data)
    
    # Check ROM size limit based on bit width
    max_size = 65536 if bittage == 16 else 4294967296
    if rom_size > max_size:
        rom_size = max_size
        rom_data = rom_data[:rom_size]
    
    # Open encoded input file
    try:
        f_input = open(sys.argv[2 + offset], 'rb')
    except IOError as e:
        print(f"Error opening encoded input file: {e}", file=sys.stderr)
        return 1
    
    # Open output file
    try:
        f_output = open(sys.argv[3 + offset], 'wb')
    except IOError as e:
        print(f"Error opening output file: {e}", file=sys.stderr)
        f_input.close()
        return 1
    
    # Decode data
    if bittage == 16:
        while True:
            data = f_input.read(2)
            if len(data) != 2:
                break
            address16 = struct.unpack('<H', data)[0]
            if address16 < rom_size:
                f_output.write(bytes([rom_data[address16]]))
    else:
        while True:
            data = f_input.read(4)
            if len(data) != 4:
                break
            address = struct.unpack('<I', data)[0]
            if address < rom_size:
                f_output.write(bytes([rom_data[address]]))
    
    f_input.close()
    f_output.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())