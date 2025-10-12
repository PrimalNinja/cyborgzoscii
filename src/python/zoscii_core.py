# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import time
import random
import struct
import sys

# ----------------------------------------------------------------------
# 1. CHARACTER CONVERTERS (Provided by User)
# ----------------------------------------------------------------------

def petscii_to_ascii(int_petscii_char_a, int_unmappable_char_a):
    """
    Function to convert PETSCII character codes to ASCII character codes.
    Maps PETSCII codes 32-95 (Space, digits, punctuation, A-Z) directly.
    All others are mapped to the unmappable character code.
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
        
        # 96-255: Everything else (unmappable)
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
    Function to convert EBCDIC character codes to ASCII character codes.
    Maps common printable characters and falls back to unmappable char for others.
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


# ----------------------------------------------------------------------
# 2. ZOSCIIENCODER CLASS (The missing component)
# ----------------------------------------------------------------------

class ZOSCIIEncoder:
    """
    Encapsulates the ZOSCII encoding logic and ROM/memory block data,
    performing the 3-pass optimization during initialization.
    """
    def __init__(self, rom_data, memory_blocks):
        self.rom_data = rom_data
        self.memory_blocks = memory_blocks
        self.rom_size = len(rom_data)
        
        # Pre-calculate valid address ranges for quick lookup (optimization)
        self._valid_ranges = []
        for block in self.memory_blocks:
            start = block['start']
            # Ensure end is not past ROM size
            end = min(block['start'] + block['size'], self.rom_size)
            if start < end:
                self._valid_ranges.append((start, end))

        # Pre-process the ROM immediately to build the address tables
        self._build_address_tables()
    
    def _is_valid_address(self, address):
        """Checks if a given address falls within any defined memory block."""
        if not (0 <= address < self.rom_size):
            return False
            
        for start, end in self._valid_ranges:
            # Check if address is in the range [start, end)
            if start <= address < end:
                return True
        return False

    def _build_address_tables(self):
        """Passes 1, 2, and 3: Builds the byte counts and address lists."""
        arr_byte_counts = [0] * 256
        arr_offsets = [0] * 256
        
        # Pass 1: Count occurrences within valid memory blocks
        for block in self.memory_blocks:
            start = block['start']
            end = min(block['start'] + block['size'], self.rom_size)
            for address in range(start, end):
                byte_value = self.rom_data[address]
                arr_byte_counts[byte_value] += 1
        
        # Pass 2 & 3: Allocate and Populate arrays with addresses
        arr_byte_addresses = [[] for _ in range(256)]
        for i in range(256):
            # Pre-allocate exact-sized array
            arr_byte_addresses[i] = [0] * arr_byte_counts[i]
            
        for block in self.memory_blocks:
            start = block['start']
            end = min(block['start'] + block['size'], self.rom_size)
            for address in range(start, end):
                byte_value = self.rom_data[address]
                # Populate the pre-allocated array and increment offset
                arr_byte_addresses[byte_value][arr_offsets[byte_value]] = address
                arr_offsets[byte_value] += 1
                        
        self.arr_byte_addresses = arr_byte_addresses
        self.arr_byte_counts = arr_byte_counts

    def encode(self, message, converter=None, unmappable_char=42):
        """
        Encodes a string message into a ZOSCII address sequence with
        unmappable character fallback logic.
        """
        int_start_time = int(time.time() * 1000)
        
        addresses = []
        int_debug_missing = 0
        arr_input_counts = [0] * 256

        for char in message:
            int_char_code = ord(char)
            int_target_byte = int_char_code
            
            # 1. Convert/map the input character code
            if converter:
                int_target_byte = converter(int_char_code, unmappable_char)
            
            # Store the original target byte (for logging)
            int_original_target = int_target_byte
            
            # Two attempts: once for the original target, once for the unmappable fallback
            for attempt in range(2):
                address_pool = self.arr_byte_addresses[int_target_byte]

                if len(address_pool) > 0:
                    # Found a match!
                    arr_input_counts[int_target_byte] += 1
                    # ITS Randomization (Inherent Time-Based System Randomization)
                    int_random_pick = random.randint(0, len(address_pool) - 1)
                    addresses.append(address_pool[int_random_pick])
                    break # Success, move to the next character
                
                # If pool is empty, and it was the first attempt, try the fallback
                if attempt == 0 and int_original_target != unmappable_char:
                    int_debug_missing += 1
                    # Log the fallback for debugging (only print the first 10 errors)
                    if int_debug_missing <= 10:
                        print(f"Missing character: '{char}' (code {int_original_target}). Falling back to unmappable char ({unmappable_char}).", file=sys.stderr)
                    int_target_byte = unmappable_char
                else:
                    # If even the unmappable char is missing, or we started there, skip.
                    # int_debug_missing += 1 # Already counted on the first attempt
                    break 

        int_end_time = int(time.time() * 1000)
        int_elapsed_ms = int_end_time - int_start_time
        
        print("\n--- ZOSCII Performance ---")
        print(f"Binary size: {self.rom_size} bytes")
        print(f"Input length: {len(message)} chars")
        print(f"Memory blocks: {len(self.memory_blocks)}")
        print(f"Execution time: {int_elapsed_ms}ms")
        print(f"Output addresses: {len(addresses)}")
        print(f"Characters found in ROM: {len(addresses)}")
        print(f"Characters missing/unmappable: {int_debug_missing}")

        return {
            'addresses': addresses,
            'input_counts': arr_input_counts,
            'rom_counts': self.arr_byte_counts
        }

    def decode(self, addresses):
        """Decodes a ZOSCII address sequence back into a message."""
        message_bytes = []
        for address in addresses:
            if address < self.rom_size:
                # Retrieve the byte value at the address
                message_bytes.append(self.rom_data[address])
            else:
                # Handle addresses pointing outside the ROM
                message_bytes.append(ord('?')) 
        # Decode the resulting byte sequence as ASCII (replacing non-ASCII bytes)
        return bytes(message_bytes).decode('ascii', errors='replace') 

    @staticmethod
    def load_addresses(file_path):
        """Loads ZOSCII addresses from a binary file (32-bit unsigned little-endian)."""
        addresses = []
        ADDRESS_SIZE = 4 
        ADDRESS_FORMAT = '<I' 
        
        with open(file_path, 'rb') as f:
            while True:
                data = f.read(ADDRESS_SIZE)
                if not data:
                    break
                if len(data) == ADDRESS_SIZE:
                    addresses.append(struct.unpack(ADDRESS_FORMAT, data)[0])
                else:
                    raise IOError("Address file is corrupt or truncated.")
        return addresses

    def save_addresses(self, addresses, file_path):
        """Saves ZOSCII addresses to a binary file."""
        ADDRESS_FORMAT = '<I'
        with open(file_path, 'wb') as f:
            for addr in addresses:
                f.write(struct.pack(ADDRESS_FORMAT, addr))


# ----------------------------------------------------------------------
# 3. STANDALONE EXECUTION/DEMONSTRATION BLOCK
# ----------------------------------------------------------------------

if __name__ == "__main__":
    # Example binary data (ROM)
    # 65 ('A'), 66 ('B'), 67 ('C'), 42 ('*'), 69 ('E'), 70 ('F') * 100 times
    # Convert list of ints to bytes object for proper indexing
    binary_data_list = [65, 66, 67, 42, 69, 70] * 100
    binary_data = bytes(binary_data_list)
    
    # Example memory blocks
    memory_blocks = [
        {'start': 0, 'size': 300},
        {'start': 300, 'size': 300}
    ]
    
    # Test string: 'A' (65) is present. 'Z' (90) is missing, falls back to 42 ('*'). 'B' (66) is present.
    test_string_fallback = "A Z B" 

    # Define a simple identity converter for clear testing of the fallback logic
    def identity_converter(char_code, unmappable_char):
        return char_code
        
    # Initialize the encoder (performs passes 1-3 here)
    encoder = ZOSCIIEncoder(binary_data, memory_blocks) 
    
    # Call the encode method
    result = encoder.encode(test_string_fallback, identity_converter, 42)
    
    print("\n--- Example Results ---")
    print("Result addresses:")
    # Print the encoded addresses
    decoded_chars = ""
    for i, addr in enumerate(result['addresses']):
        char = test_string_fallback[i]
        decoded_byte = binary_data[addr]
        decoded_char = chr(decoded_byte)
        decoded_chars += decoded_char
        print(f"Input Char '{char}' -> ROM Byte {decoded_byte} ('{decoded_char}') -> Address {addr}")
    
    print("\nInput character counts:")
    # Print the counts of characters successfully encoded
    for i in range(256):
        if result['input_counts'][i] > 0:
            char = chr(i) if 32 <= i <= 126 else '?'
            print(f"Byte {i} ('{char}'): {result['input_counts'][i]} occurrences")
            
    # Example of decoding (for demonstration purposes)
    decoded_message = encoder.decode(result['addresses'])
    print(f"\nDecoded (for verification): {decoded_message}")
    print(f"Expected Decoded Output (using char codes): A * B")
