"""
Cyborg ZOSCII v20250805
(c) 2025 Cyborg Unicorn Pty Ltd.
This software is released under MIT License.

ZOSCII encoding library for Python
"""

import time
import random
from typing import List, Dict, Optional, Callable, Tuple
from dataclasses import dataclass


@dataclass
class MemoryBlock:
    """Represents a block of memory in the ROM"""
    start: int
    size: int


@dataclass
class ZosciiResult:
    """Contains the encoded addresses and statistics"""
    addresses: List[int]
    input_counts: List[int]
    rom_counts: List[int]


# Type alias for converter functions
ConverterFunc = Callable[[int, int], int]


def to_zoscii(
    binary_data: bytes,
    input_string: str,
    memory_blocks: List[MemoryBlock],
    converter: Optional[ConverterFunc] = None,
    unmappable_char: int = 42
) -> ZosciiResult:
    """
    Converts a string to ZOSCII address sequence
    
    Args:
        binary_data: ROM/binary data as bytes
        input_string: Message to encode
        memory_blocks: List of MemoryBlock objects defining valid ROM regions
        converter: Optional character conversion function (e.g., petscii_to_ascii)
        unmappable_char: Character code for unmappable characters (default: 42)
    
    Returns:
        ZosciiResult containing addresses and statistics
    """
    start_time = time.time()
    
    byte_counts = [0] * 256
    byte_addresses = [[] for _ in range(256)]
    input_counts = [0] * 256
    result_count = 0
    debug_missing = 0
    
    # Pass 1: Count occurrences by iterating through blocks
    for block in memory_blocks:
        end = min(block.start + block.size, len(binary_data))
        for address in range(block.start, end):
            byte_val = binary_data[address]
            byte_counts[byte_val] += 1
    
    # Pass 2: Pre-allocate exact-sized lists
    for i in range(256):
        if byte_counts[i] > 0:
            byte_addresses[i] = [0] * byte_counts[i]
    
    # Pass 3: Populate arrays by iterating through blocks
    offsets = [0] * 256
    for block in memory_blocks:
        end = min(block.start + block.size, len(binary_data))
        for address in range(block.start, end):
            byte_val = binary_data[address]
            byte_addresses[byte_val][offsets[byte_val]] = address
            offsets[byte_val] += 1
    
    # Count valid characters for result array size
    for i, ch in enumerate(input_string):
        index = ord(ch)
        if converter:
            index = converter(index, unmappable_char)
        
        if 0 <= index < 256 and len(byte_addresses[index]) > 0:
            result_count += 1
        else:
            debug_missing += 1
            if debug_missing <= 10:
                print(f"Missing character: '{ch}' (code {ord(ch)} -> {index})")
    
    print(f"Characters found in ROM: {result_count}")
    print(f"Characters missing from ROM: {debug_missing}")
    
    # Build result array with random addresses
    addresses = []
    
    for ch in input_string:
        index = ord(ch)
        if converter:
            index = converter(index, unmappable_char)
        
        if 0 <= index < 256 and len(byte_addresses[index]) > 0:
            input_counts[index] += 1
            random_pick = random.randint(0, len(byte_addresses[index]) - 1)
            addresses.append(byte_addresses[index][random_pick])
    
    elapsed_ms = (time.time() - start_time) * 1000
    
    print("ZOSCII Performance:")
    print(f"- Input length: {len(input_string)} chars")
    print(f"- Memory blocks: {len(memory_blocks)}")
    print(f"- Execution time: {elapsed_ms:.2f}ms")
    print(f"- Output addresses: {len(addresses)}")
    
    return ZosciiResult(
        addresses=addresses,
        input_counts=input_counts,
        rom_counts=byte_counts
    )


def petscii_to_ascii(petscii_char: int, unmappable_char: int) -> int:
    """
    Converts PETSCII character codes to ASCII
    
    Args:
        petscii_char: PETSCII character code (0-255)
        unmappable_char: Character code to use for unmappable characters
    
    Returns:
        ASCII character code or unmappable_char
    """
    petscii_map = [
        # 0-31: Control characters
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        # 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        # 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        
        # 96-255: Everything else mapped to unmappable
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    ]
    
    if petscii_char < 0 or petscii_char > 255:
        return unmappable_char
    
    result = petscii_map[petscii_char]
    return unmappable_char if result == -1 else result


def ebcdic_to_ascii(ebcdic_char: int, unmappable_char: int) -> int:
    """
    Converts EBCDIC character codes to ASCII
    
    Args:
        ebcdic_char: EBCDIC character code (0-255)
        unmappable_char: Character code to use for unmappable characters
    
    Returns:
        ASCII character code or unmappable_char
    """
    ebcdic_map = [
        # 0-63: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        # 64-79: Space and some punctuation
        32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 46, 60, 40, 43, 124,
        
        # 80-95: & and punctuation
        38, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, 36, -1, 41, 59, -1,
        
        # 96-111: - and punctuation
        45, 47, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 37, 95, 62, 63,
        
        # 112-127: More punctuation
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 96, 58, 35, 64, 39, 61, 34,
        
        # 128: Control
        -1,
        
        # 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        # 138-144: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        # 145-153: j-r
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        # 154-161: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        # 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        # 170-192: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1,
        
        # 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        # 202-208: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        # 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        # 218-225: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        # 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        # 234-239: Control/special
        -1, -1, -1, -1, -1, -1,
        
        # 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        # 250-255: Control/special
        -1, -1, -1, -1, -1, -1,
    ]
    
    if ebcdic_char < 0 or ebcdic_char > 255:
        return unmappable_char
    
    result = ebcdic_map[ebcdic_char]
    return unmappable_char if result == -1 else result