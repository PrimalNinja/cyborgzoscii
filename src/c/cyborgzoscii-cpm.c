/* Cyborg ZOSCII v20250805
   (c) 2025 Cyborg Unicorn Pty Ltd.
   This software is released under MIT License. */

/* Small-C for CP/M - ZOSCII ROM Encoder */
#include "stdio.h"

/* caters for a memory map as follows:
   program
   1.5kb table header (occurance count+block pointer+memory pointer)
   32kb occurances tables
   1kb reconstruction area
   1kb reconstruction area+bottom of rom shared
   15kb rest of rom /*

/* Lookup table structure - 1536 bytes total */
/* 256 entries x 6 bytes each = 3 words per entry */
int lookup_table[768];  /* 256 * 3 = 768 words */

/* Memory pointers */
char *tpa_start;
char *tpa_end;
char *rom_start;
int table_size;
int available_space;

/* Initialize and grab all TPA */
int init_memory()
{
    int *bdos_ptr;
    
    /* Get TPA size from BDOS */
    bdos_ptr = 0x0006;  /* BDOS warm boot vector */
    tpa_end = *bdos_ptr - 1;
    
    /* TPA starts after our program + lookup table */
    tpa_start = &lookup_table[768]; /* End of lookup table */
    
    available_space = tpa_end - tpa_start;
    
    printf("TPA: %u bytes available\n", available_space);
    return available_space;
}

/* Load ROM at highest possible address */
char *load_rom()
{
    FILE *rom_file;
    int bytes_read;
    
    /* ROM goes at top of TPA - 16KB */
    rom_start = tpa_end - 16383;
    
    rom_file = fopen("ROM.BIN", "rb");
    if (!rom_file) 
	{
        printf("Cannot open ROM.BIN\n");
        return 0;
    }
    
    bytes_read = fread(rom_start, 1, 16384, rom_file);
    fclose(rom_file);
    
    printf("Loaded %d bytes of ROM at %04X\n", bytes_read, rom_start);
    return rom_start;
}

/* Step 4: Count byte occurrences in ROM */
void count_bytes()
{
    char *rom_ptr;
    int i, byte_val;
    
    /* Initialize lookup table */
    for (i = 0; i < 768; i++) 
	{
        lookup_table[i] = 0;
    }
    
    /* Scan entire ROM */
    rom_ptr = rom_start;
    for (i = 0; i < 16384; i++) 
	{
        byte_val = *rom_ptr & 0xFF;
        lookup_table[byte_val * 3]++;  /* Increment occurrence count */
        rom_ptr++;
    }
    
    printf("Byte counting complete\n");
}

/* Step 5: Allocate blocks and set pointers */
void allocate_blocks()
{
    int i, byte_val;
    char *alloc_ptr;
    int occurrence_count;
    int block_size;
    
    /* Start allocating right after lookup table */
    alloc_ptr = tpa_start;
    
    for (byte_val = 0; byte_val < 256; byte_val++) 
	{
        occurrence_count = lookup_table[byte_val * 3];
        
        if (occurrence_count > 0) 
		{
            /* Set block start pointer */
            lookup_table[byte_val * 3 + 1] = alloc_ptr;
            
            /* Calculate block size (2 bytes per address) */
            block_size = occurrence_count * 2;
            
            /* Move to next allocation point */
            alloc_ptr += block_size;
        }
        
        /* Initialize fill counter to 0 */
        lookup_table[byte_val * 3 + 2] = 0;
    }
    
    printf("Block allocation complete, used %d bytes\n", alloc_ptr - tpa_start);
}

void process_and_write()
{
    FILE *out_file;
    char write_buffer[2048];     /* 2KB output buffer */
    char rom_chunk[1024];        /* Copy of 1KB ROM chunk */
    int buffer_pos;
    int chunk, rom_addr, chunk_offset;
    int byte_val, address;
    int *block_start, *fill_counter;
    char *write_ptr;
    
    /* Randomization variables */
    int occurrence_count;
    int random_index;
    char *random_addr_ptr;
    static int pseudo_seed = 12345;  /* Simple PRNG seed */
    
    out_file = fopen("ENCODED.BIN", "wb");
    if (!out_file) 
	{
        printf("Cannot create output file\n");
        return;
    }
    
    /* Process 16 chunks of 1KB ROM each → 2KB output each */
    for (chunk = 0; chunk < 16; chunk++) 
	{
        printf("Processing chunk %d\n", chunk);
        
        /* Step 6a: Expand ROM chunk into working area (1KB → 2KB) */
        rom_addr = chunk * 1024;
        work_area = rom_start - 2048;  /* Working area just below ROM */
        
        /* Expand forward: consume ROM before we overwrite it */
        for (chunk_offset = 0; chunk_offset < 1024; chunk_offset++) {
            work_area[chunk_offset * 2] = rom_start[rom_addr + chunk_offset];  /* Byte value */
            work_area[chunk_offset * 2 + 1] = 0;                              /* Zero padding */
        }
        
        /* Step 6b: Process this chunk - populate address lists */
        for (chunk_offset = 0; chunk_offset < 1024; chunk_offset++) {
            byte_val = work_area[chunk_offset * 2] & 0xFF;  /* Get byte from expanded area */
            address = rom_addr + chunk_offset;              /* Absolute ROM address */
            
            /* Get pointers from lookup table */
            block_start = lookup_table[byte_val * 3 + 1];
            fill_counter = &lookup_table[byte_val * 3 + 2];
            
            if (block_start) {
                /* Calculate write position in address list */
                write_ptr = block_start + (*fill_counter * 2);
                
                /* Store address as 2 bytes */
                *write_ptr++ = address & 0xFF;      /* Low byte */
                *write_ptr = (address >> 8) & 0xFF; /* High byte */
                
                /* Increment fill counter */
                (*fill_counter)++;
            }
        }
        
        /* Step 7: Reconstruct output with randomization */
        buffer_pos = 0;
        for (chunk_offset = 0; chunk_offset < 1024; chunk_offset++) 
		{
            byte_val = rom_chunk[chunk_offset] & 0xFF;
            
            /* Get this byte's address list info */
            occurrence_count = lookup_table[byte_val * 3];
            block_start = lookup_table[byte_val * 3 + 1];
            
            if (occurrence_count > 0 && block_start) 
			{
                /* Simple pseudo-random number generator */
                pseudo_seed = (pseudo_seed * 1103515245 + 12345) & 0x7FFFFFFF;
                random_index = pseudo_seed % occurrence_count;
                
                /* Calculate pointer to random address in the list */
                random_addr_ptr = block_start + (random_index * 2);
                
                /* Write the random address to output */
                write_buffer[buffer_pos++] = random_addr_ptr[0]; /* Low byte */
                write_buffer[buffer_pos++] = random_addr_ptr[1]; /* High byte */
            } else {
                printf("ERROR: No addresses for byte %02X\n", byte_val);
                /* Write zeros as fallback */
                write_buffer[buffer_pos++] = 0;
                write_buffer[buffer_pos++] = 0;
            }
        }
        
        /* Write 2KB chunk to disk */
        if (buffer_pos == 2048) 
		{
            fwrite(write_buffer, 1, 2048, out_file);
            printf("Wrote chunk %d (2KB)\n", chunk);
        } else {
            printf("ERROR: Chunk %d size: %d bytes\n", chunk, buffer_pos);
        }
    }
    
    fclose(out_file);
    printf("32KB encoded file complete\n");
}

/* Updated main program */
main()
{
    printf("Small-C ROM Encoder for CP/M\n");
    
    if (!init_memory()) 
	{
        printf("Insufficient memory\n");
        return 1;
    }
    
    if (!load_rom()) 
	{
        printf("ROM load failed\n");
        return 1;
    }
    
    count_bytes();          /* Step 4 */
    allocate_blocks();      /* Step 5 */
    process_and_write();    /* Combined Step 6+7 */
    
    printf("Encoding complete\n");
    return 0;
}