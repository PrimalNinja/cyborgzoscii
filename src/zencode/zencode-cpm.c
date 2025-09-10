/* Cyborg ZOSCII v20250805
   (c) 2025 Cyborg Unicorn Pty Ltd.
   This software is released under MIT License. */

/* Small-C for CP/M - ZOSCII ROM Encoder - Input Streaming Version */
#include "stdio.h"

/* Lookup table structure - 1536 bytes total */
/* 256 entries x 6 bytes each = 3 words per entry */
int arrLookup[768];  /* 256 * 3 = 768 words */

/* Memory pointers */
char* intTPAStart;
char* intTPAEnd;
char* intROMStart;
int intAvailableTPA;
int intROMSize;

/* Streaming buffers */
char arrInputBuffer[1024];    /* 1KB input chunk buffer */
char arrOutputBuffer[2048];  /* 2KB output buffer */

/* Initialize and grab all TPA */
int init_memory()
{
    int* pBDOS;
    
    /* Get TPA size from BDOS */
    pBDOS = 0x0006;  /* BDOS warm boot vector */
    intTPAEnd = *pBDOS - 1;
    
    /* TPA starts after our program + lookup table */
    intTPAStart = &arrLookup[768]; /* End of lookup table */
    
    intAvailableTPA = intTPAEnd - intTPAStart;
    
    printf("TPA: %u bytes available\n", intAvailableTPA);
    return intAvailableTPA;
}

/* Load ROM at highest possible address */
char* load_rom(char* strFilename_a)
{
    FILE* fROM;
    int intBytesRead;
    
    /* ROM goes at top of TPA - 16KB */
    intROMStart = intTPAEnd - 16383;
    
    fROM = fopen(strFilename_a, "rb");
    if (!fROM) 
	{
        printf("Cannot open ROM file: %s\n", strFilename_a);
        return 0;
    }
    
    intBytesRead = fread(intROMStart, 1, 16384, fROM);
    fclose(fROM);
    
    intROMSize = intBytesRead;  /* ADD THIS LINE - store actual size */
    
    printf("Loaded %d bytes of ROM from %s at %04X\n", intBytesRead, strFilename_a, intROMStart);
    return intROMStart;
}

/* Count byte occurrences in ROM */
void count_bytes()
{
    char* pROM;
    int by;
    int intI;
    
    /* Initialize lookup table */
    for (intI = 0; intI < 768; intI++) 
	{
        arrLookup[intI] = 0;
    }
    
    /* Scan entire ROM - USE ACTUAL SIZE */
    pROM = intROMStart;
    for (intI = 0; intI < intROMSize; intI++) 
	{
        by = *pROM & 0xFF;
        arrLookup[by * 3]++;  /* Increment occurrence count */
        pROM++;
    }
    
    printf("Byte counting complete\n");
}

/* Allocate address list blocks */
void allocate_blocks()
{
    char* pAlloc;
    int by;
    int intBlockSize;
    int intOccuranceCount;
    
    /* Start allocating right after lookup table, before ROM */
    pAlloc = intTPAStart;
    
    for (by = 0; by < 256; by++) 
	{
        intOccuranceCount = arrLookup[by * 3];
        
        if (intOccuranceCount > 0) 
		{
            /* Set block start pointer */
            arrLookup[by * 3 + 1] = (int)pAlloc;
            
            /* Calculate block size (2 bytes per address) */
            intBlockSize = intOccuranceCount * 2;
            
            /* Move to next allocation point */
            pAlloc += intBlockSize;
        }
        
        /* Initialize fill counter to 0 */
        arrLookup[by * 3 + 2] = 0;
    }
    
    printf("Block allocation complete, used %d bytes\n", pAlloc - intTPAStart);
    
    /* Check if we have enough space before ROM */
    if (pAlloc >= intROMStart) 
	{
        printf("ERROR: Address tables overflow into ROM space!\n");
        printf("Tables need %d bytes, only %d available\n", 
               pAlloc - intTPAStart, intROMStart - intTPAStart);
    }
}

/* Populate address lists with ROM addresses */
void populate_address_lists()
{
    char* pROM;
    int by;
    int* intFillCounter;
    int intI;
    char* pWrite;
    
    /* Scan ROM and populate address lists - USE ACTUAL SIZE */
    pROM = intROMStart;
    for (intI = 0; intI < intROMSize; intI++) 
	{
        by = *pROM & 0xFF;
        
        /* Get pointers from lookup table */
        if (arrLookup[by * 3] > 0) 
		{  /* Has occurrences */
            intFillCounter = &arrLookup[by * 3 + 2];
            pWrite = arrLookup[by * 3 + 1] + (*intFillCounter * 2);
            
            /* Store address as 2 bytes */
            *pWrite++ = intI & 0xFF;      /* Low byte */
            *pWrite = (intI >> 8) & 0xFF; /* High byte */
            
            /* Increment fill counter */
            (*intFillCounter)++;
        }
        pROM++;
    }
    
    printf("Address lists populated\n");
}

/* Stream input file and encode to output */
void encode_input_streaming(char *strInputFilename_a, char *strOutputFilename_a)
{
    FILE* fInput;
    FILE* fOutput;
    int by;
    int intBytesRead;
    int intChunkCount = 0;
    int intInputOffset;
    int intOccuranceCount;
    int intOutputPos;
    int intRandomIdx;
    long intTotalInput = 0;
    long intTotalOutput = 0;
    static int intPseudoSeed = 12345;  /* Simple PRNG seed */
    char* pBlockStart;
    char* pRandomAddr;
    
    fInput = fopen(strInputFilename_a, "rb");
    if (!fInput) 
	{
        printf("Cannot open input file: %s\n", strInputFilename_a);
        return;
    }
    
    fOutput = fopen(strOutputFilename_a, "wb");
    if (!fOutput) 
	{
        printf("Cannot create output file: %s\n", strOutputFilename_a);
        fclose(fInput);
        return;
    }
    
    printf("Streaming input file encoding...\n");
    intOutputPos = 0;
    
    /* Stream input file in 1KB chunks */
    while ((intBytesRead = fread(arrInputBuffer, 1, 1024, fInput)) > 0) 
	{
        intTotalInput += intBytesRead;
        intChunkCount++;
        
        /* Process each byte in this input chunk */
        for (intInputOffset = 0; intInputOffset < intBytesRead; intInputOffset++) 
		{
            by = arrInputBuffer[intInputOffset] & 0xFF;
            
            /* Get this byte's address list info */
            intOccuranceCount = arrLookup[by * 3];
            pBlockStart = arrLookup[by * 3 + 1];
            
            if (intOccuranceCount > 0 && pBlockStart) 
			{
                /* Simple pseudo-random number generator */
                intPseudoSeed = (intPseudoSeed * 1103515245 + 12345) & 0x7FFFFFFF;
                intRandomIdx = intPseudoSeed % intOccuranceCount;
                
                /* Calculate pointer to random address in the list */
                pRandomAddr = pBlockStart + (intRandomIdx * 2);
                
                /* Add address to output buffer */
                arrOutputBuffer[intOutputPos++] = pRandomAddr[0]; /* Low byte */
                arrOutputBuffer[intOutputPos++] = pRandomAddr[1]; /* High byte */

                /* Flush output buffer when full */
                if (intOutputPos >= 2048) 
				{
                    fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
                    intTotalOutput += intOutputPos;
                    intOutputPos = 0;
                }
            } 
			else 
			{
                printf("WARNING: No ROM addresses for byte 0x%02X (char '%c')\n", 
                       by, (by >= 32 && by <= 126) ? by : '?');
                /* Write zeros as fallback */
                arrOutputBuffer[intOutputPos++] = 0;
                arrOutputBuffer[intOutputPos++] = 0;
                
                /* Flush output buffer when full */
                if (intOutputPos >= 2048) 
				{
                    fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
                    intTotalOutput += intOutputPos;
                    intOutputPos = 0;
                }
            }
        }
        
        /* Progress indicator */
        if (intChunkCount % 10 == 0) 
		{
            printf("Processed %d chunks (%ld bytes)...\n", intChunkCount, intTotalInput);
        }
    }
    
    /* Flush any remaining output */
    if (intOutputPos > 0) 
	{
        fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
        intTotalOutput += intOutputPos;
    }
    
    fclose(fInput);
    fclose(fOutput);
    
    printf("\nEncoding complete:\n");
    printf("Input file:  %ld bytes\n", intTotalInput);
    printf("Output file: %ld bytes\n", intTotalOutput);
    printf("Expansion ratio: %.1f:1\n", (float)intTotalOutput / (float)intTotalInput);
}

/* Print memory usage statistics */
void print_memory_stats()
{
    char* pLastAlloc = intTPAStart;
    int by;
    int intMaxCount = 0;
    int intMinCount = 999;
    int intTotalAddresses = 0;
    int intUsedBytes = 0;
    
    printf("\nMemory Usage Statistics:\n");
    printf("TPA Start: %04X\n", intTPAStart);
    printf("ROM Start: %04X\n", intROMStart);
    printf("TPA End:   %04X\n", intTPAEnd);
    
    /* Calculate address table usage */
    for (by = 0; by < 256; by++) 
	{
        int intCount = arrLookup[by * 3];
        if (intCount > 0) 
		{
            intTotalAddresses += intCount;
            intUsedBytes += intCount * 2;
            if (intCount < intMinCount) intMinCount = intCount;
            if (intCount > intMaxCount) intMaxCount = intCount;
            
            char* pBlock = arrLookup[by * 3 + 1];
            if (pBlock > pLastAlloc) pLastAlloc = pBlock + (intCount * 2);
        }
    }
    
    printf("Address tables: %d bytes (%04X to %04X)\n", 
           intUsedBytes, intTPAStart, pLastAlloc);
    printf("Free space: %d bytes\n", intROMStart - pLastAlloc);
    printf("Total addresses: %d\n", intTotalAddresses);
    printf("Address range: %d to %d per byte\n", intMinCount, intMaxCount);
}

/* Main program */
main(int intArgC_a, *arrArgs_a[])
{
    char* strROMFilename;
    char* strInputFilename;
    char* strOutputFilename;
    
    printf("Small-C ZOSCII Encoder for CP/M - Streaming Version\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    /* Check command line arguments */
    if (intArgC_a == 3) 
	{
        /* Two arguments - input and output files, use default ROM.BIN */
        strROMFilename = "ROM.BIN";
        strInputFilename = arrArgs_a[1];
        strOutputFilename = arrArgs_a[2];
        printf("Using default ROM: ROM.BIN\n");
        printf("Input: %s -> Output: %s\n\n", strInputFilename, strOutputFilename);
    }
    else if (intArgC_a == 4) 
	{
        /* Three arguments - ROM, input, and output files */
        strROMFilename = arrArgs_a[1];
        strInputFilename = arrArgs_a[2];
        strOutputFilename = arrArgs_a[3];
        printf("ROM: %s\n", strROMFilename);
        printf("Input: %s -> Output: %s\n\n", strInputFilename, strOutputFilename);
    }
    else 
	{
        printf("Usage: ZENCODE <inputfile> <outputfile>\n");
        printf("   or: ZENCODE <romfile> <inputfile> <outputfile>\n");
        printf("\nExamples:\n");
        printf("  ZENCODE INPUT.TXT ENCODED.BIN        (uses ROM.BIN)\n");
        printf("  ZENCODE MYROM.BIN INPUT.TXT ENCODED.BIN\n");
        return 1;
    }
    
    /* Initialize memory management */
    if (!init_memory()) 
	{
        printf("ERROR: Insufficient memory\n");
        return 1;
    }
    
    /* Load ROM into top of TPA */
    if (!load_rom(strROMFilename)) 
	{
        printf("ERROR: ROM load failed\n");
        return 1;
    }
    
    /* Analyze ROM and build lookup structures */
    printf("Analyzing ROM...\n");
    count_bytes();          /* Count byte occurrences */
    allocate_blocks();      /* Allocate address list blocks */
    populate_address_lists(); /* Fill address lists with ROM positions */
    
    /* Show memory usage */
    print_memory_stats();
    
    /* Stream encode input file */
    printf("\nStarting input file encoding...\n");
    encode_input_streaming(strInputFilename, strOutputFilename);
    
    printf("\nZOSCII encoding complete!\n");
    printf("Files: %s -> %s\n", strInputFilename, strOutputFilename);
    printf("ROM: %s (loaded at %04X)\n", strROMFilename, intROMStart);
    
    return 0;
}
