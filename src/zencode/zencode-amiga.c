// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    uint32_t *addresses;
    uint32_t count;
} ByteAddresses;

int main(int argc, char *argv[]) {
    int bit_width = 16;  // Default to 16-bit for Amiga compatibility
    int arg_offset = 0;
    
    // Parse command-line arguments for bit width
    if (argc >= 2 && strcmp(argv[1], "-32") == 0) {
        bit_width = 32;
        arg_offset = 1;
    } else if (argc >= 2 && strcmp(argv[1], "-16") == 0) {
        bit_width = 16;
        arg_offset = 1;
    }
    
    // Validate arguments
    if (argc != 4 + arg_offset) {
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile> <encodedoutput>\n", argv[0]);
        return 1;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Open ROM file
    FILE *rom_file = fopen(argv[1 + arg_offset], "rb");
    if (!rom_file) {
        perror("Error opening ROM file");
        return 1;
    }
    
    // Get ROM file size
    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);
    
    // Limit ROM size based on bit width
    long max_size = (bit_width == 16) ? 65536 : 4294967296L;
    if (rom_size > max_size) {
        rom_size = max_size;
    }
    
    // Allocate memory for ROM data
    uint8_t *rom_data = malloc(rom_size);
    if (!rom_data) {
        fprintf(stderr, "Memory allocation failed for ROM data\n");
        fclose(rom_file);
        return 1;
    }
    
    fread(rom_data, 1, rom_size, rom_file);
    fclose(rom_file);
    
    // Build address lookup tables
    ByteAddresses lookup[256];
    uint32_t counts[256] = {0};
    
    // Count occurrences
    for (long i = 0; i < rom_size; i++) {
        counts[rom_data[i]]++;
    }
    
    // Allocate address arrays
    for (int i = 0; i < 256; i++) {
        lookup[i].addresses = malloc(counts[i] * sizeof(uint32_t));
        if (!lookup[i].addresses) {
            fprintf(stderr, "Memory allocation failed for lookup table %d\n", i);
            free(rom_data);
            for (int j = 0; j < i; j++) {
                free(lookup[j].addresses);
            }
            return 1;
        }
        lookup[i].count = 0;
    }
    
    // Populate address arrays
    for (long i = 0; i < rom_size; i++) {
        uint8_t byte = rom_data[i];
        lookup[byte].addresses[lookup[byte].count++] = i;
    }
    
    // Open input file
    FILE *input_file = fopen(argv[2 + arg_offset], "rb");
    if (!input_file) {
        perror("Error opening input file");
        free(rom_data);
        for (int i = 0; i < 256; i++) {
            free(lookup[i].addresses);
        }
        return 1;
    }
    
    // Open output file
    FILE *output_file = fopen(argv[3 + arg_offset], "wb");
    if (!output_file) {
        perror("Error opening output file");
        free(rom_data);
        for (int i = 0; i < 256; i++) {
            free(lookup[i].addresses);
        }
        fclose(input_file);
        return 1;
    }
    
    // Process input and write output
    int c;
    while ((c = fgetc(input_file)) != EOF) {
        uint8_t byte = (uint8_t)c;
        if (lookup[byte].count > 0) {
            uint32_t random_idx = rand() % lookup[byte].count;
            uint32_t address = lookup[byte].addresses[random_idx];
            
            if (bit_width == 16) {
                uint16_t addr16 = (uint16_t)address;
                fwrite(&addr16, sizeof(uint16_t), 1, output_file);
            } else {
                fwrite(&address, sizeof(uint32_t), 1, output_file);
            }
        }
    }
    
    // Clean up
    fclose(input_file);
    fclose(output_file);
    free(rom_data);
    for (int i = 0; i < 256; i++) {
        free(lookup[i].addresses);
    }
    
    return 0;
}