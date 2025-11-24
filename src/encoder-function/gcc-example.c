// Example program demonstrating ZOSCII library usage
// Compile: gcc example.c zoscii.c -o example

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "zoscii-encoder.h"

int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Example: Create a simple ROM with ASCII characters
    unsigned char romData[1000];
    for (int i = 0; i < 1000; i++) {
        romData[i] = 65 + (i % 26); // Fill with A-Z pattern
    }
    
    // Define memory blocks (entire ROM is one block)
    MemoryBlock blocks[] = {
        {0, 1000}
    };
    int blockCount = 1;
    
    // Test message
    const char* message = "HELLO WORLD";
    
    printf("Encoding message: %s\n\n", message);
    
    // Encode using ZOSCII
    ZOSCIIResult result = toZOSCII(romData, message, blocks, blockCount, 
                                   NULL, 42); // No converter, '*' for unmappable
    
    // Display results
    printf("\nEncoded addresses:\n");
    for (int i = 0; i < result.address_count; i++) {
        char ch = (i < strlen(message)) ? message[i] : '?';
        printf("  '%c' -> address %4d (ROM byte: 0x%02X)\n", 
               ch, result.addresses[i], romData[result.addresses[i]]);
    }
    
    // Verify by decoding
    printf("\nDecoding verification:\n");
    printf("Decoded: ");
    for (int i = 0; i < result.address_count; i++) {
        printf("%c", romData[result.addresses[i]]);
    }
    printf("\n\n");
    
    // Show character usage statistics
    printf("Character usage from input:\n");
    for (int i = 0; i < 256; i++) {
        if (result.input_counts[i] > 0) {
            printf("  '%c' (0x%02X): used %d times, ROM has %d occurrences\n", 
                   (i >= 32 && i <= 126) ? i : '?', i,
                   result.input_counts[i], result.rom_counts[i]);
        }
    }
    
    // Clean up
    freeZOSCIIResult(&result);
    
    printf("\nExample complete!\n");
    return 0;
}