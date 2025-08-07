# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import time
import random

def to_zoscii(arr_binary_data_a, str_input_string_a, arr_memory_blocks_a, cb_converter_a, int_unmappable_char_a):
    """
    Function to convert string to ZOSCII address sequence
    :param arr_binary_data_a: list/bytes containing the ROM/binary data  
    :param str_input_string_a: message to convert
    :param arr_memory_blocks_a: list of {'start': startAddress, 'size': blockSize} objects
    :param cb_converter_a: encoding conversion function or None
    :param int_unmappable_char_a: the native character code to be used if it cannot be mapped to ASCII
    :return: dict with 'addresses', 'input_counts', 'rom_counts' keys
    """
    
    int_start_time = int(time.time() * 1000)
    
    int_result_index = 0
    int_result_count = 0
    int_debug_missing = 0
    
    arr_byte_counts = [0] * 256
    arr_byte_addresses = [[] for _ in range(256)]
    arr_offsets = [0] * 256
    arr_input_counts = [0] * 256
    
    # Pass 1: Count occurrences by iterating through blocks
    for int_block in range(len(arr_memory_blocks_a)):
        obj_block = arr_memory_blocks_a[int_block]
        for int_address in range(obj_block['start'], obj_block['start'] + obj_block['size']):
            int_byte = arr_binary_data_a[int_address]
            arr_byte_counts[int_byte] += 1
    
    # Pass 2: Pre-allocate exact-sized arrays
    for int_i in range(256):
        arr_byte_addresses[int_i] = [0] * arr_byte_counts[int_i]
        arr_offsets[int_i] = 0
    
    # Pass 3: Populate arrays by iterating through blocks
    for int_block in range(len(arr_memory_blocks_a)):
        obj_block = arr_memory_blocks_a[int_block]
        for int_address in range(obj_block['start'], obj_block['start'] + obj_block['size']):
            int_byte = arr_binary_data_a[int_address]
            arr_byte_addresses[int_byte][arr_offsets[int_byte]] = int_address
            arr_offsets[int_byte] += 1
    
    # Build result array with random addresses - pre-allocate and avoid append()
    for int_i in range(len(str_input_string_a)):
        int_index = ord(str_input_string_a[int_i])
        if cb_converter_a:
            int_index = cb_converter_a(int_index, int_unmappable_char_a)
        
        if (int_index >= 0 and int_index < 256 and 
            arr_byte_addresses[int_index] and len(arr_byte_addresses[int_index]) > 0):
            int_result_count += 1
        else:
            int_debug_missing += 1
            if int_debug_missing <= 10:
                print(f"Missing character: '{str_input_string_a[int_i]}' (code {ord(str_input_string_a[int_i])} -> {int_index})")
    
    print(f"Characters found in ROM: {int_result_count}")
    print(f"Characters missing from ROM: {int_debug_missing}")
    
    arr_result = [0] * int_result_count
    
    for int_i in range(len(str_input_string_a)):
        int_index = ord(str_input_string_a[int_i])
        if cb_converter_a:
            int_index = cb_converter_a(int_index, int_unmappable_char_a)
        
        if (int_index >= 0 and int_index < 256 and 
            arr_byte_addresses[int_index] and len(arr_byte_addresses[int_index]) > 0):
            arr_input_counts[int_index] += 1
            int_random_pick = random.randint(0, len(arr_byte_addresses[int_index]) - 1)
            arr_result[int_result_index] = arr_byte_addresses[int_index][int_random_pick]
            int_result_index += 1
    
    int_end_time = int(time.time() * 1000)
    int_elapsed_ms = int_end_time - int_start_time
    
    print("ZOSCII Performance:")
    print(f"- Binary size: {len(arr_binary_data_a)} bytes")
    print(f"- Input length: {len(str_input_string_a)} chars")
    print(f"- Memory blocks: {len(arr_memory_blocks_a)}")
    print(f"- Execution time: {int_elapsed_ms}ms")
    print(f"- Output addresses: {len(arr_result)}")
    
    return {
        'addresses': arr_result,
        'input_counts': arr_input_counts,
        'rom_counts': arr_byte_counts
    }


def petscii_to_ascii(int_petscii_char_a, int_unmappable_char_a):
    """
    Function to convert PETSCII character codes to ASCII character codes
    :param int_petscii_char_a: PETSCII character code (0-255)
    :param int_unmappable_char_a: the native character code to be used if it cannot be mapped to ASCII
    :return: ASCII character code or int_unmappable_char_a for unmappable characters
    """
    
    arr_petscii_to_ascii_map = [
        # 0-31: Control characters
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        # 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        
        # 96-255: Everything else
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a
    ]
    
    return arr_petscii_to_ascii_map[int_petscii_char_a]


def ebcdic_to_ascii(int_ebcdic_char_a, int_unmappable_char_a):
    """
    Function to convert EBCDIC character codes to ASCII character codes
    :param int_ebcdic_char_a: EBCDIC character code (0-255)
    :param int_unmappable_char_a: the native character code to be used if it cannot be mapped to ASCII
    :return: ASCII character code or int_unmappable_char_a for unmappable characters
    """

    arr_ebcdic_to_ascii_map = [
        # 0-63: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 64-79: Space and some punctuation
        32, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, 46, 60, 40, 43, 124,
        
        # 80-95: & and punctuation
        38, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, 33, 36, int_unmappable_char_a, 41, 59, int_unmappable_char_a,
        
        # 96-111: - and punctuation
        45, 47, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, 44, 37, 95, 62, 63,
        
        # 112-127: More punctuation
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, 96, 58, 35, 64, 39, 61, 34,
        
        # 128: Control
        int_unmappable_char_a,
        
        # 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        # 138-144: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 145-153: j-r
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        # 154-161: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        # 170-192: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        # 202-208: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        # 218-225: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        # 234-239: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a,
        
        # 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        # 250-255: Control/special
        int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a, int_unmappable_char_a
    ]
    
    return arr_ebcdic_to_ascii_map[int_ebcdic_char_a]


# Example usage
if __name__ == "__main__":
    # Example binary data
    binary_data = [65, 66, 67, 68, 69, 70] * 100  # Sample data with A-F pattern
    
    # Example memory blocks
    memory_blocks = [
        {'start': 0, 'size': 300},
        {'start': 300, 'size': 300}
    ]
    
    # Test string
    test_string = "HELLO WORLD"
    
    # Call the function with PETSCII converter
    result = to_zoscii(binary_data, test_string, memory_blocks, petscii_to_ascii, 42)
    
    print("\nResult addresses:")
    for i, addr in enumerate(result['addresses']):
        print(f"Address {i}: {addr}")
    
    print("\nInput character counts:")
    for i in range(256):
        if result['input_counts'][i] > 0:
            char = chr(i) if 32 <= i <= 126 else '?'
            print(f"Character {i} ('{char}'): {result['input_counts'][i]} occurrences")
