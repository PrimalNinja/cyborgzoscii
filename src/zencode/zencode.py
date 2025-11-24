#!/usr/bin/env python3
# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import random
import struct

def main():
    print("ZOSCII Encoder")
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
        print(f"Usage: {sys.argv[0]} [-16|-32] <romfile> <inputdatafile> <encodedoutput>", file=sys.stderr)
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
    
    # Build address lookup tables
    lookup = [[] for _ in range(256)]
    rom_counts = [0] * 256
    
    # Count occurrences
    for i in range(rom_size):
        rom_counts[rom_data[i]] += 1
    
    # Allocate address arrays
    for i in range(256):
        lookup[i] = [0] * rom_counts[i]
    
    # Populate address arrays
    counts = [0] * 256
    for i in range(rom_size):
        byte = rom_data[i]
        lookup[byte][counts[byte]] = i
        counts[byte] += 1
    
    # Read input file
    try:
        with open(sys.argv[2 + offset], 'rb') as f:
            input_data = f.read()
    except IOError as e:
        print(f"Error opening input file: {e}", file=sys.stderr)
        return 1
    
    # Open output file
    try:
        f_output = open(sys.argv[3 + offset], 'wb')
    except IOError as e:
        print(f"Error opening output file: {e}", file=sys.stderr)
        return 1
    
    # Encode data
    for byte in input_data:
        if len(lookup[byte]) > 0:
            random_idx = random.randint(0, len(lookup[byte]) - 1)
            address = lookup[byte][random_idx]
            
            if bittage == 16:
                address16 = address & 0xFFFF
                f_output.write(struct.pack('<H', address16))
            else:
                f_output.write(struct.pack('<I', address))
    
    f_output.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())