// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Memory block structure
typedef struct {
    int start;
    int size;
} MemoryBlock;

// Dynamic array structure for addresses
typedef struct {
    int* addresses;
    int count;
    int capacity;
} AddressList;

// Function pointer type for character converters
typedef int (*CharConverter)(int character, int unmappable_char);

// Helper function to add address to dynamic array
void addAddress(AddressList* list, int address) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 10 : list->capacity * 2;
        list->addresses = realloc(list->addresses, list->capacity * sizeof(int));
    }
    list->addresses[list->count++] = address;
}

// Helper function to check if address is in valid memory blocks
int isValidAddress(int address, MemoryBlock* memory_blocks, int block_count) {
    for (int i = 0; i < block_count; i++) {
        if (address >= memory_blocks[i].start && 
            address < (memory_blocks[i].start + memory_blocks[i].size)) {
            return 1;
        }
    }
    return 0;
}

// Function to convert PETSCII character codes to ASCII character codes
int petsciiToAscii(int petscii_char, int unmappable_char) {
    int petscii_to_ascii_map[256];
    int i;
    
    // Initialize all positions with unmappable_char
    for (i = 0; i < 256; i++) {
        petscii_to_ascii_map[i] = unmappable_char;
    }
    
    // 32-63: Space, digits, punctuation (direct ASCII mapping)
    for (i = 32; i <= 63; i++) {
        petscii_to_ascii_map[i] = i;
    }
    
    // 64-95: @A-Z[\]^_ (direct ASCII mapping)
    for (i = 64; i <= 95; i++) {
        petscii_to_ascii_map[i] = i;
    }
    
    return petscii_to_ascii_map[petscii_char];
}

// Function to convert EBCDIC character codes to ASCII character codes
int ebcdicToAscii(int ebcdic_char, int unmappable_char) {
    int ebcdic_to_ascii_map[256];
    int i;
    
    // Initialize all positions with unmappable_char
    for (i = 0; i < 256; i++) {
        ebcdic_to_ascii_map[i] = unmappable_char;
    }
    
    // Specific EBCDIC to ASCII mappings
    ebcdic_to_ascii_map[64] = 32;   // Space
    ebcdic_to_ascii_map[75] = 46;   // .
    ebcdic_to_ascii_map[76] = 60;   // <
    ebcdic_to_ascii_map[77] = 40;   // (
    ebcdic_to_ascii_map[78] = 43;   // +
    ebcdic_to_ascii_map[79] = 124;  // |
    ebcdic_to_ascii_map[80] = 38;   // &
    ebcdic_to_ascii_map[90] = 33;   // !
    ebcdic_to_ascii_map[91] = 36;   // $
    ebcdic_to_ascii_map[93] = 41;   // )
    ebcdic_to_ascii_map[94] = 59;   // ;
    ebcdic_to_ascii_map[96] = 45;   // -
    ebcdic_to_ascii_map[97] = 47;   // /
    ebcdic_to_ascii_map[107] = 44;  // ,
    ebcdic_to_ascii_map[108] = 37;  // %
    ebcdic_to_ascii_map[109] = 95;  // _
    ebcdic_to_ascii_map[110] = 62;  // >
    ebcdic_to_ascii_map[111] = 63;  // ?
    ebcdic_to_ascii_map[121] = 96;  // `
    ebcdic_to_ascii_map[122] = 58;  // :
    ebcdic_to_ascii_map[123] = 35;  // #
    ebcdic_to_ascii_map[124] = 64;  // @
    ebcdic_to_ascii_map[125] = 39;  // '
    ebcdic_to_ascii_map[126] = 61;  // =
    ebcdic_to_ascii_map[127] = 34;  // "
    
    // a-i (129-137)
    for (i = 129; i <= 137; i++) {
        ebcdic_to_ascii_map[i] = 97 + (i - 129);
    }
    
    // j-r (145-153)
    for (i = 145; i <= 153; i++) {
        ebcdic_to_ascii_map[i] = 106 + (i - 145);
    }
    
    // s-z (162-169)
    for (i = 162; i <= 169; i++) {
        ebcdic_to_ascii_map[i] = 115 + (i - 162);
    }
    
    // A-I (193-201)
    for (i = 193; i <= 201; i++) {
        ebcdic_to_ascii_map[i] = 65 + (i - 193);
    }
    
    // J-R (209-217)
    for (i = 209; i <= 217; i++) {
        ebcdic_to_ascii_map[i] = 74 + (i - 209);
    }
    
    // S-Z (226-233)
    for (i = 226; i <= 233; i++) {
        ebcdic_to_ascii_map[i] = 83 + (i - 226);
    }
    
    // 0-9 (240-249)
    for (i = 240; i <= 249; i++) {
        ebcdic_to_ascii_map[i] = 48 + (i - 240);
    }
    
    return ebcdic_to_ascii_map[ebcdic_char];
}

// Function to convert string to ZOSCII address sequence
int* toZOSCII(unsigned char* binary_data, int data_length, const char* input_string, 
              MemoryBlock* memory_blocks, int block_count, CharConverter converter, 
              int unmappable_char, int* result_count) {
    
    AddressList byte_addresses[256];
    int i, address, byte_value, index, random_pick;
    int string_length = strlen(input_string);
    int* result = malloc(string_length * sizeof(int));
    *result_count = 0;
    
    // Initialize byte address arrays
    for (i = 0; i < 256; i++) {
        byte_addresses[i].addresses = NULL;
        byte_addresses[i].count = 0;
        byte_addresses[i].capacity = 0;
    }
    
    // Parse binary data and populate address arrays
    for (address = 0; address < data_length; address++) {
        if (isValidAddress(address, memory_blocks, block_count)) {
            byte_value = binary_data[address];
            addAddress(&byte_addresses[byte_value], address);
        }
    }
    // Build result array with random addresses
    for (i = 0; i < string_length; i++) {
        index = (unsigned char)input_string[i];
        
        if (converter != NULL) {
            index = converter(index, unmappable_char);
        }
        
        if (index >= 0 && index < 256 && byte_addresses[index].count > 0) {
            // Pick random address from this character's array
            random_pick = rand() % byte_addresses[index].count;
            address = byte_addresses[index].addresses[random_pick];
            result[*result_count] = address;
            (*result_count)++;
        }
    }
    
    // Clean up dynamic arrays
    for (i = 0; i < 256; i++) {
        if (byte_addresses[i].addresses != NULL) {
            free(byte_addresses[i].addresses);
        }
    }
    
    // Resize result array to actual size
    result = realloc(result, (*result_count) * sizeof(int));
    return result;
}

// Example usage
int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Create memory blocks
    MemoryBlock memory_blocks[] = {
        {0xC000, 0x1000},  // ROM at C000-CFFF
        {0xE000, 0x0800}   // Additional ROM at E000-E7FF
    };
    int block_count = sizeof(memory_blocks) / sizeof(MemoryBlock);
    
    // Create dummy ROM data (64KB address space)
    unsigned char* rom_data = malloc(65536);
    for (int i = 0; i < 65536; i++) {
        rom_data[i] = rand() % 256;  // Fill with random data for example
    }
    
    // Convert string to ZOSCII addresses
    const char* message = "Hello, World!";
    int result_count;
    int* addresses = toZOSCII(rom_data, 65536, message, memory_blocks, 
                              block_count, ebcdicToAscii, 42, &result_count);
    
    // Print results
    printf("ZOSCII addresses for '%s':\n", message);
    for (int i = 0; i < result_count; i++) {
        printf("'%c' -> 0x%04X (%d)\n", message[i], addresses[i], addresses[i]);
    }
    
    // Clean up
    free(addresses);
    free(rom_data);
    
    return 0;
}