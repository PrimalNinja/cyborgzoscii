// Cyborg ZOSCII Decoder v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", argv[0]);
        return 1;
    }
    
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
    
    // Open encoded input file
    FILE *input_file = fopen(argv[2 + arg_offset], "rb");
    if (!input_file) {
        perror("Error opening encoded input file");
        free(rom_data);
        return 1;
    }
    
    // Open output file
    FILE *output_file = fopen(argv[3 + arg_offset], "wb");
    if (!output_file) {
        perror("Error opening output file");
        free(rom_data);
        fclose(input_file);
        return 1;
    }
    
    // Decode input based on bit width
    if (bit_width == 16) {
        uint16_t address;
        while (fread(&address, sizeof(uint16_t), 1, input_file) == 1) {
            if (address < rom_size) {
                fputc(rom_data[address], output_file);
            }
        }
    } else {
        uint32_t address;
        while (fread(&address, sizeof(uint32_t), 1, input_file) == 1) {
            if (address < rom_size) {
                fputc(rom_data[address], output_file);
            }
        }
    }
    
    // Clean up
    fclose(input_file);
    fclose(output_file);
    free(rom_data);
    
    return 0;
}