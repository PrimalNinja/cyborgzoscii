// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Set binary mode for stdin/stdout if needed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    int bit_width = 16;  // default
    int arg_offset = 0;
    
    if (argc >= 2 && strcmp(argv[1], "-32") == 0) {
        bit_width = 32;
        arg_offset = 1;
    } else if (argc >= 2 && strcmp(argv[1], "-16") == 0) {
        bit_width = 16;
        arg_offset = 1;
    }
    
    if (argc != 4 + arg_offset) {
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", argv[0]);
        return 1;
    }
    
    FILE *rom_file = fopen(argv[1 + arg_offset], "rb");
    if (!rom_file) {
        perror("Error opening ROM file");
        return 1;
    }
    
    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);
    
    // Check ROM size limit based on bit width
    long max_size = (bit_width == 16) ? 65536 : 4294967296L;
    if (rom_size > max_size) {
        rom_size = max_size;
    }
    
    uint8_t *rom_data = malloc(rom_size);
    fread(rom_data, 1, rom_size, rom_file);
    fclose(rom_file);
    
    FILE *input_file = fopen(argv[2 + arg_offset], "rb");
    if (!input_file) {
        perror("Error opening encoded input file");
        return 1;
    }
    
    FILE *output_file = fopen(argv[3 + arg_offset], "wb");
    if (!output_file) {
        perror("Error opening output file");
        return 1;
    }
    
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
    
    fclose(input_file);
    fclose(output_file);
    free(rom_data);
    
    return 0;
}