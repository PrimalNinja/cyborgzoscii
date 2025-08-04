# Cyborg ZOSCII v20250805
# (c) 2025 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import struct
import random
from typing import List, Optional, Callable, Dict, Any

class ZOSCIIEncoder:
    def __init__(self, rom_data: bytes, memory_blocks: List[Dict[str, int]]):
        """
        Initialize ZOSCII encoder with ROM data and memory blocks.
        
        Args:
            rom_data: Binary ROM data
            memory_blocks: List of dicts with 'start' and 'size' keys
        """
        self.rom_data = rom_data
        self.memory_blocks = memory_blocks
        self.byte_addresses = self._build_address_arrays()
    
    def _is_valid_address(self, address: int) -> bool:
        """Check if address is within valid memory blocks."""
        for block in self.memory_blocks:
            if block['start'] <= address < (block['start'] + block['size']):
                return True
        return False
    
    def _build_address_arrays(self) -> Dict[int, List[int]]:
        """Build arrays of addresses for each byte value."""
        byte_addresses = {i: [] for i in range(256)}
        
        for address in range(len(self.rom_data)):
            if self._is_valid_address(address):
                byte_value = self.rom_data[address]
                byte_addresses[byte_value].append(address)
        
        return byte_addresses
    
    def encode(self, message: str, converter: Optional[Callable[[int, int], int]] = None, 
               unmappable_char: int = 42) -> List[int]:
        """
        Encode message to ZOSCII address sequence.
        
        Args:
            message: Text message to encode
            converter: Optional character conversion function
            unmappable_char: Fallback character for unmappable chars
            
        Returns:
            List of memory addresses
        """
        result = []
        
        for char in message:
            char_code = ord(char)
            
            # Apply converter if provided
            if converter:
                char_code = converter(char_code, unmappable_char)
            
            # Find addresses for this character
            addresses = self.byte_addresses.get(char_code, [])
            if addresses:
                # Randomly select an address
                selected_address = random.choice(addresses)
                result.append(selected_address)
        
        return result
    
    def decode(self, addresses: List[int]) -> str:
        """
        Decode ZOSCII address sequence back to text.
        
        Args:
            addresses: List of memory addresses
            
        Returns:
            Decoded message string
        """
        message = ""
        for address in addresses:
            if 0 <= address < len(self.rom_data):
                byte_value = self.rom_data[address]
                message += chr(byte_value)
            else:
                message += "?"
        return message
    
    def save_addresses(self, addresses: List[int], filename: str):
        """Save addresses to binary file (little-endian format)."""
        with open(filename, 'wb') as f:
            for addr in addresses:
                f.write(struct.pack('<I', addr))
    
    @staticmethod
    def load_addresses(filename: str) -> List[int]:
        """Load addresses from binary file."""
        addresses = []
        with open(filename, 'rb') as f:
            while True:
                data = f.read(4)
                if len(data) < 4:
                    break
                address = struct.unpack('<I', data)[0]
                addresses.append(address)
        return addresses

# Character conversion functions
def petscii_to_ascii(petscii_char: int, unmappable_char: int) -> int:
    """Convert PETSCII character to ASCII."""
    # Direct mapping for printable ASCII range
    if 32 <= petscii_char <= 95:
        return petscii_char
    return unmappable_char

def ebcdic_to_ascii(ebcdic_char: int, unmappable_char: int) -> int:
    """Convert EBCDIC character to ASCII."""
    ebcdic_map = {
        # Space and punctuation
        64: 32,   # Space
        75: 46,   # .
        76: 60,   # <
        77: 40,   # (
        78: 43,   # +
        79: 124,  # |
        80: 38,   # &
        90: 33,   # !
        91: 36,   # $
        93: 41,   # )
        94: 59,   # ;
        96: 45,   # -
        97: 47,   # /
        107: 44,  # ,
        108: 37,  # %
        109: 95,  # _
        110: 62,  # >
        111: 63,  # ?
        121: 96,  # `
        122: 58,  # :
        123: 35,  # #
        124: 64,  # @
        125: 39,  # '
        126: 61,  # =
        127: 34,  # "
    }
    
    # Lowercase letters a-i (129-137)
    if 129 <= ebcdic_char <= 137:
        return 97 + (ebcdic_char - 129)
    
    # Lowercase letters j-r (145-153)
    if 145 <= ebcdic_char <= 153:
        return 106 + (ebcdic_char - 145)
    
    # Lowercase letters s-z (162-169)
    if 162 <= ebcdic_char <= 169:
        return 115 + (ebcdic_char - 162)
    
    # Uppercase letters A-I (193-201)
    if 193 <= ebcdic_char <= 201:
        return 65 + (ebcdic_char - 193)
    
    # Uppercase letters J-R (209-217)
    if 209 <= ebcdic_char <= 217:
        return 74 + (ebcdic_char - 209)
    
    # Uppercase letters S-Z (226-233)
    if 226 <= ebcdic_char <= 233:
        return 83 + (ebcdic_char - 226)
    
    # Digits 0-9 (240-249)
    if 240 <= ebcdic_char <= 249:
        return 48 + (ebcdic_char - 240)
    
    return ebcdic_map.get(ebcdic_char, unmappable_char)

# Example usage
if __name__ == "__main__":
    # Load ROM data
    with open('rom_file.bin', 'rb') as f:
        rom_data = f.read()
    
    # Define memory blocks
    memory_blocks = [
        {'start': 0x0000, 'size': len(rom_data)}
    ]
    
    # Create encoder
    encoder = ZOSCIIEncoder(rom_data, memory_blocks)
    
    # Encode message
    message = "Hello, World!"
    addresses = encoder.encode(message, ebcdic_to_ascii, 42)
    print(f"Encoded '{message}' to {len(addresses)} addresses")
    
    # Save to file
    encoder.save_addresses(addresses, 'message.bin')
    
    # Decode back
    decoded = encoder.decode(addresses)
    print(f"Decoded: '{decoded}'")
    
    # Verify roundtrip
    if decoded == message:
        print("✓ Encoding/decoding successful!")
    else:
        print("✗ Encoding/decoding failed!")