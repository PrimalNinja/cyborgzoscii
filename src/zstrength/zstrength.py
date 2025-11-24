#!/usr/bin/env python3
# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import math

def print_large_number(exponent):
    if exponent < 3:
        print(f"~{10**exponent:.0f} permutations", end='')
    elif exponent < 6:
        print(f"~{10**exponent / 1000.0:.1f} thousand permutations", end='')
    elif exponent < 9:
        print(f"~{10**exponent / 1000000.0:.1f} million permutations", end='')
    elif exponent < 12:
        print(f"~{10**exponent / 1000000000.0:.1f} billion permutations", end='')
    elif exponent < 15:
        print(f"~{10**exponent / 1000000000000.0:.1f} trillion permutations", end='')
    elif exponent < 82:
        print(f"More than all atoms in the observable universe (10^{exponent:.0f} permutations)", end='')
    elif exponent < 1000:
        print(f"Incomprehensibly massive (10^{exponent:.0f} permutations)", end='')
    else:
        print(f"Astronomically secure (10^{exponent / 1000000.0:.1f}M permutations)", end='')

def main():
    print("ZOSCII ROM Strength Analyzer")
    print("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n")
    
    bittage = 16  # default
    offset = 0
    
    if len(sys.argv) >= 2 and sys.argv[1] == "-32":
        bittage = 32
        offset = 1
    elif len(sys.argv) >= 2 and sys.argv[1] == "-16":
        bittage = 16
        offset = 1
    
    if len(sys.argv) != 3 + offset:
        print(f"Usage: {sys.argv[0]} [-16|-32] <romfile> <inputdatafile>", file=sys.stderr)
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
    
    # Count ROM byte occurrences
    rom_counts = [0] * 256
    input_counts = [0] * 256
    
    for i in range(rom_size):
        rom_counts[rom_data[i]] += 1
    
    # Read input file
    try:
        with open(sys.argv[2 + offset], 'rb') as f:
            input_data = f.read()
    except IOError as e:
        print(f"Error opening input file: {e}", file=sys.stderr)
        return 1
    
    # Count input character occurrences
    input_length = len(input_data)
    chars_used = 0
    
    for byte in input_data:
        input_counts[byte] += 1
    
    # Count characters utilized
    for i in range(256):
        if input_counts[i] > 0:
            chars_used += 1
    
    # Calculate ROM strength metrics
    general_strength = 0.0
    file_strength = 0.0
    
    for i in range(256):
        if rom_counts[i] > 0:
            general_strength += math.log10(rom_counts[i])
        if input_counts[i] > 0 and rom_counts[i] > 0:
            file_strength += input_counts[i] * math.log10(rom_counts[i])
    
    utilisation = (chars_used / 256.0) * 100.0
    
    print(f"ROM Strength Analysis ({bittage}-bit)")
    print("===============================\n")
    
    print("Input Information:")
    print(f"- Text Length: {input_length} characters")
    print(f"- Characters Utilized: {chars_used} of 256 ({utilisation:.1f}%)")
    print()
    
    print(f"General ROM Capacity: ~10^{general_strength:.0f} (", end='')
    print_large_number(general_strength)
    print(")")
    
    print(f"This File Security: ~10^{file_strength:.0f} (", end='')
    print_large_number(file_strength)
    print(")\n")
    
    print("Byte Analysis:")
    print("Byte  Dec  ROM Count  Input Count  Char")
    print("----  ---  ---------  -----------  ----")
    
    for i in range(256):
        if rom_counts[i] > 0 or input_counts[i] > 0:
            display = chr(i) if 32 <= i <= 126 else ' '
            print(f"0x{i:02X}  {i:3d}  {rom_counts[i]:9d}  {input_counts[i]:11d}    {display}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())