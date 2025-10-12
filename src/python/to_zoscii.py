# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import time
import random
import sys

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
    
    int_result_count = 0
    int_debug_missing = 0
    
    arr_byte_counts = [0] * 256
    arr_byte_addresses = [[] for _ in range(256)]
    arr_offsets = [0] * 256
    arr_input_counts = [0] * 256
    
    int_rom_size = len(arr_binary_data_a)
    
    # Pass 1: Count occurrences by iterating through blocks
    for obj_block in arr_memory_blocks_a:
        start = obj_block['start']
        end = min(obj_block['start'] + obj_block['size'], int_rom_size)
        for int_address in range(start, end):
            int_byte = arr_binary_data_a[int_address]
            arr_byte_counts[int_byte] += 1
    
    # Pass 2: Pre-allocate exact-sized arrays
    for int_i in range(256):
        arr_byte_addresses[int_i] = [0] * arr_byte_counts[int_i]
        arr_offsets[int_i] = 0
    
    # Pass 3: Populate arrays by iterating through blocks
    for obj_block in arr_memory_blocks_a:
        start = obj_block['start']
        end = min(obj_block['start'] + obj_block['size'], int_rom_size)
        for int_address in range(start, end):
            int_byte = arr_binary_data_a[int_address]
            arr_byte_addresses[int_byte][arr_offsets[int_byte]] = int_address
            arr_offsets[int_byte] += 1
    
    # Build result array with random addresses - pre-allocate and avoid append()
    arr_addresses = []
    
    for int_i in range(len(str_input_string_a)):
        char = str_input_string_a[int_i]
        int_char_code = ord(char)
        int_target_byte = int_char_code
        
        # 1. Convert/map the input character code
        if cb_converter_a:
            int_target_byte = cb_converter_a(int_char_code, int_unmappable_char_a)

        int_original_target = int_target_byte
        
        # Inner loop for fallback logic
        for attempt in range(2):
            address_pool = arr_byte_addresses[int_target_byte]
            
            if len(address_pool) > 0:
                arr_input_counts[int_target_byte] += 1
                # ITS Randomization
                int_random_pick = random.randint(0, len(address_pool) - 1)
                arr_addresses.append(address_pool[int_random_pick])
                int_result_count += 1
                break # Found the address, move to the next character
            
            # If not found and it was the first attempt, try the fallback
            if attempt == 0 and int_original_target != int_unmappable_char_a:
                int_debug_missing += 1
                # Optional: print log to stderr
                # print(f"Missing character: '{char}' (code {int_original_target}). Falling back to unmappable char ({int_unmappable_char_a}).", file=sys.stderr)
                int_target_byte = int_unmappable_char_a
            else:
                # If even the unmappable char is missing, skip.
                # int_debug_missing += 1 # Already counted on the first attempt
                break # Character cannot be encoded, skip

    int_end_time = int(time.time() * 1000)
    int_elapsed_ms = int_end_time - int_start_time
    
    # Performance statistics (Optional, but included for completeness)
    print("\n--- ZOSCII Performance (to_zoscii function) ---", file=sys.stderr)
    print(f"Binary size: {len(arr_binary_data_a)} bytes", file=sys.stderr)
    print(f"Input length: {len(str_input_string_a)} chars", file=sys.stderr)
    print(f"Memory blocks: {len(arr_memory_blocks_a)}", file=sys.stderr)
    print(f"Execution time: {int_elapsed_ms}ms", file=sys.stderr)
    print(f"Output addresses: {len(arr_addresses)}", file=sys.stderr)
    print(f"Characters found in ROM: {int_result_count}", file=sys.stderr)
    print(f"Characters missing/unmappable: {int_debug_missing}", file=sys.stderr)

    return {
        'addresses': arr_addresses,
        'input_counts': arr_input_counts,
        'rom_counts': arr_byte_counts
    }
