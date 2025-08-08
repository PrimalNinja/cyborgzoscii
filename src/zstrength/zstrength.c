// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

void print_large_number(double exponent) {
    if (exponent < 3) {
        printf("~%.0f permutations", pow(10, exponent));
    } else if (exponent < 6) {
        printf("~%.1f thousand permutations", pow(10, exponent) / 1000.0);
    } else if (exponent < 9) {
        printf("~%.1f million permutations", pow(10, exponent) / 1000000.0);
    } else if (exponent < 12) {
        printf("~%.1f billion permutations", pow(10, exponent) / 1000000000.0);
    } else if (exponent < 15) {
        printf("~%.1f trillion permutations", pow(10, exponent) / 1000000000000.0);
    } else if (exponent < 82) {
        printf("More than all atoms in the observable universe (10^%.0f permutations)", exponent);
    } else if (exponent < 1000) {
        printf("Incomprehensibly massive (10^%.0f permutations)", exponent);
    } else {
        printf("Astronomically secure (10^%.1fM permutations)", exponent / 1000000.0);
    }
}

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
    
    if (argc != 3 + arg_offset) {
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile>\n", argv[0]);
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
    
    // Count ROM byte occurrences
    uint32_t rom_counts[256] = {0};
    uint32_t input_counts[256] = {0};
    
    for (long i = 0; i < rom_size; i++) {
        rom_counts[rom_data[i]]++;
    }
    
    FILE *input_file = fopen(argv[2 + arg_offset], "rb");
    if (!input_file) {
        perror("Error opening input file");
        return 1;
    }
    
    // Count input character occurrences
    int c;
    int input_length = 0;
    int characters_used = 0;
    
    while ((c = fgetc(input_file)) != EOF) {
        uint8_t byte = (uint8_t)c;
        input_counts[byte]++;
        input_length++;
    }
    fclose(input_file);
    
    // Count characters utilized
    for (int i = 0; i < 256; i++) {
        if (input_counts[i] > 0) {
            characters_used++;
        }
    }
    
    // Calculate ROM strength metrics
    double general_strength = 0.0;
    double file_strength = 0.0;
    
    for (int i = 0; i < 256; i++) {
        if (rom_counts[i] > 0) {
            general_strength += log10(rom_counts[i]);
        }
        if (input_counts[i] > 0 && rom_counts[i] > 0) {
            file_strength += input_counts[i] * log10(rom_counts[i]);
        }
    }
    
    double utilization = (characters_used / 256.0) * 100.0;
    
    printf("ROM Strength Analysis (%d-bit)\n", bit_width);
    printf("===============================\n\n");
    
    printf("Input Information:\n");
    printf("- Text Length: %d characters\n", input_length);
    printf("- Characters Utilized: %d of 256 (%.1f%%)\n", characters_used, utilization);
    printf("\n");
    
    printf("General ROM Capacity: ~10^%.0f (", general_strength);
    print_large_number(general_strength);
    printf(")\n");
    
    printf("This File Security: ~10^%.0f (", file_strength);
    print_large_number(file_strength);
    printf(")\n\n");
    
    printf("Byte Analysis:\n");
    printf("Byte  Dec  ROM Count  Input Count  Char\n");
    printf("----  ---  ---------  -----------  ----\n");
    
    for (int i = 0; i < 256; i++) {
        if (rom_counts[i] > 0 || input_counts[i] > 0) {
            char display_char = (i >= 32 && i <= 126) ? i : ' ';
            printf("0x%02X  %3d  %9u  %11u    %c\n", 
                   i, i, rom_counts[i], input_counts[i], display_char);
        }
    }
    
    free(rom_data);
    return 0;
}