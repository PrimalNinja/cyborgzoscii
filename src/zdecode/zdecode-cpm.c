/* Cyborg ZOSCII v20250805
   (c) 2025 Cyborg Unicorn Pty Ltd.
   This software is released under MIT License. */

/* Small-C for CP/M - ZOSCII ROM Decoder - Streaming Version */
#include "stdio.h"

/* Simple memory layout for decoder:
   program
   16kb ROM (loaded at top of TPA)
   I/O buffers only - no lookup tables needed! */

/* Memory pointers */
char *tpa_start;
char *tpa_end;
char *rom_start;
int available_space;
int actual_rom_size;

/* Streaming buffers */
char input_chunk[2048];    /* 2KB input chunk buffer (address pairs) */
char output_buffer[1024];  /* 1KB output buffer (decoded bytes) */

/* Initialize and grab all TPA */
int init_memory()
{
    int *bdos_ptr;
    
    /* Get TPA size from BDOS */
    bdos_ptr = 0x0006;  /* BDOS warm boot vector */
    tpa_end = *bdos_ptr - 1;
    
    /* TPA starts after our program */
    tpa_start = &tpa_end;  /* Simple - no lookup tables needed */
    
    available_space = tpa_end - tpa_start;
    
    printf("TPA: %u bytes available\n", available_space);
    return available_space;
}

/* Load ROM at highest possible address */
char *load_rom(char *filename)
{
    FILE *rom_file;
    int bytes_read;
    
    /* ROM goes at top of TPA - 16KB */
    rom_start = tpa_end - 16383;
    
    rom_file = fopen(filename, "rb");
    if (!rom_file) {
        printf("Cannot open ROM file: %s\n", filename);
        return 0;
    }
    
    bytes_read = fread(rom_start, 1, 16384, rom_file);
    fclose(rom_file);
    
    actual_rom_size = bytes_read;  /* ADD THIS LINE - store actual size */
    
    printf("Loaded %d bytes of ROM from %s at %04X\n", bytes_read, filename, rom_start);
    return rom_start;
}

/* Stream encoded file and decode to output */
void decode_input_streaming(char *input_filename, char *output_filename)
{
    FILE *input_file, *output_file;
    int bytes_read, input_offset, output_pos;
    int address_low, address_high, rom_address;
    char decoded_byte;
    long total_input = 0, total_output = 0;
    int chunk_count = 0;
    
    input_file = fopen(input_filename, "rb");  /* Use parameter */
    if (!input_file) {
        printf("Cannot open input file: %s\n", input_filename);
        return;
    }
    
    output_file = fopen(output_filename, "wb");  /* Use parameter */
    if (!output_file) {
        printf("Cannot create output file: %s\n", output_filename);
        fclose(input_file);
        return;
    }
    
    printf("Streaming encoded file decoding...\n");
    output_pos = 0;
    
    /* Stream encoded file in 2KB chunks (1K address pairs = 1K decoded bytes) */
    while ((bytes_read = fread(input_chunk, 1, 2048, input_file)) > 0) {
        total_input += bytes_read;
        chunk_count++;
        
        /* Process each address pair in this input chunk */
        for (input_offset = 0; input_offset < bytes_read; input_offset += 2) {
            /* Check we have a complete address pair */
            if (input_offset + 1 < bytes_read) {
                /* Read 16-bit address (little endian) */
                address_low = input_chunk[input_offset] & 0xFF;
                address_high = input_chunk[input_offset + 1] & 0xFF;
                rom_address = address_low | (address_high << 8);
                
                /* Bounds check ROM address - USE ACTUAL SIZE */
                if (rom_address >= 0 && rom_address < actual_rom_size) {
                    /* Direct lookup in ROM - that's it! */
                    decoded_byte = rom_start[rom_address];
                    
                    /* Add decoded byte to output buffer */
                    output_buffer[output_pos++] = decoded_byte;
                    
                    /* Flush output buffer when full */
                    if (output_pos >= 1024) {
                        fwrite(output_buffer, 1, output_pos, output_file);
                        total_output += output_pos;
                        output_pos = 0;
                    }
                } else {
                    printf("WARNING: ROM address %04X out of range (ROM size: %d)\n", 
                           rom_address, actual_rom_size);
                    /* Write placeholder for bad address */
                    output_buffer[output_pos++] = '?';
                    
                    /* Flush output buffer when full */
                    if (output_pos >= 1024) {
                        fwrite(output_buffer, 1, output_pos, output_file);
                        total_output += output_pos;
                        output_pos = 0;
                    }
                }
            }
        }
        
        /* Progress indicator */
        if (chunk_count % 50 == 0) {
            printf("Processed %d chunks (%ld bytes)...\n", chunk_count, total_input);
        }
    }
    
    /* Flush any remaining output */
    if (output_pos > 0) {
        fwrite(output_buffer, 1, output_pos, output_file);
        total_output += output_pos;
    }
    
    fclose(input_file);
    fclose(output_file);
    
    printf("\nDecoding complete:\n");
    printf("Input file:  %ld bytes\n", total_input);
    printf("Output file: %ld bytes\n", total_output);
    printf("Compression ratio: %.1f:1\n", (float)total_input / (float)total_output);
}

/* Main program */
main(argc, argv)
int argc;
char *argv[];
{
    char *rom_filename;
    char *input_filename;
    char *output_filename;
    
    printf("Small-C ZOSCII Decoder for CP/M - Streaming Version\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    /* Check command line arguments */
    if (argc == 3) {
        /* Two arguments - input and output files, use default ROM.BIN */
        rom_filename = "ROM.BIN";
        input_filename = argv[1];
        output_filename = argv[2];
        printf("Using default ROM: ROM.BIN\n");
        printf("Input: %s -> Output: %s\n\n", input_filename, output_filename);
    }
    else if (argc == 4) {
        /* Three arguments - ROM, input, and output files */
        rom_filename = argv[1];
        input_filename = argv[2];
        output_filename = argv[3];
        printf("ROM: %s\n", rom_filename);
        printf("Input: %s -> Output: %s\n\n", input_filename, output_filename);
    }
    else {
        printf("Usage: ZDECODE <inputfile> <outputfile>\n");
        printf("   or: ZDECODE <romfile> <inputfile> <outputfile>\n");
        printf("\nExamples:\n");
        printf("  ZDECODE ENCODED.BIN DECODED.TXT        (uses ROM.BIN)\n");
        printf("  ZDECODE MYROM.BIN ENCODED.BIN DECODED.TXT\n");
        return 1;
    }
    
    /* Initialize memory management */
    if (!init_memory()) {
        printf("ERROR: Insufficient memory\n");
        return 1;
    }
    
    /* Load ROM into top of TPA */
    if (!load_rom(rom_filename)) {
        printf("ERROR: ROM load failed\n");
        return 1;
    }
    
    printf("ROM loaded - ready to decode\n");
    printf("ROM starts at: %04X (%d bytes)\n", rom_start, actual_rom_size);
    printf("Available TPA: %d bytes\n", rom_start - tpa_start);
    
    /* Stream decode input file */
    printf("\nStarting file decoding...\n");
    decode_input_streaming(input_filename, output_filename);
    
    printf("\nZOSCII decoding complete!\n");
    printf("Files: %s -> %s\n", input_filename, output_filename);
    printf("ROM: %s (loaded at %04X)\n", rom_filename, rom_start);
    
    return 0;
}