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
char* intTPAStart;
char* intTPAEnd;
char* intROMStart;
int intAvailableTPA;
int intROMSize;

/* Streaming buffers */
char arrInputBuffer[2048];    /* 2KB input chunk buffer (address pairs) */
char arrOutputBuffer[1024];  /* 1KB output buffer (decoded bytes) */

/* Initialize and grab all TPA */
int init_memory()
{
    int* pBDOS;
    
    /* Get TPA size from BDOS */
    pBDOS = 0x0006;  /* BDOS warm boot vector */
    intTPAEnd = *pBDOS - 1;
    
    /* TPA starts after our program */
    intTPAStart = &intTPAEnd;  /* Simple - no lookup tables needed */
    
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

/* Stream encoded file and decode to output */
void decode_input_streaming(char* strInputFilename_a, char* strOutputFilename_a)
{
    FILE* fInput;
    FILE* fOutput;
    char chDecodedByte;
    int intAddressHigh;
    int intAddressLow;
    int intBytesRead;
    int intChunkCount = 0;
    int intInputOffset;
    int intOutputPos;
    int intROMAddress;
    long lngTotalInput = 0;
    long lngTotalOutput = 0;
    
    fInput = fopen(strInputFilename_a, "rb");  /* Use parameter */
    if (!fInput) 
	{
        printf("Cannot open input file: %s\n", strInputFilename_a);
        return;
    }
    
    fOutput = fopen(strOutputFilename_a, "wb");  /* Use parameter */
    if (!fOutput) 
	{
        printf("Cannot create output file: %s\n", strOutputFilename_a);
        fclose(fInput);
        return;
    }
    
    printf("Streaming encoded file decoding...\n");
    intOutputPos = 0;
    
    /* Stream encoded file in 2KB chunks (1K address pairs = 1K decoded bytes) */
    while ((intBytesRead = fread(arrInputBuffer, 1, 2048, fInput)) > 0) 
	{
        lngTotalInput += intBytesRead;
        intChunkCount++;
        
        /* Process each address pair in this input chunk */
        for (intInputOffset = 0; intInputOffset < intBytesRead; intInputOffset += 2) 
		{
            /* Check we have a complete address pair */
            if (intInputOffset + 1 < intBytesRead) 
			{
                /* Read 16-bit address (little endian) */
                intAddressLow = arrInputBuffer[intInputOffset] & 0xFF;
                intAddressHigh = arrInputBuffer[intInputOffset + 1] & 0xFF;
                intROMAddress = intAddressLow | (intAddressHigh << 8);
                
                /* Bounds check ROM address - USE ACTUAL SIZE */
                if (intROMAddress >= 0 && intROMAddress < intROMSize) 
				{
                    /* Direct lookup in ROM - that's it! */
                    chDecodedByte = intROMStart[intROMAddress];
                    
                    /* Add decoded byte to output buffer */
                    arrOutputBuffer[intOutputPos++] = chDecodedByte;
                    
                    /* Flush output buffer when full */
                    if (intOutputPos >= 1024) 
					{
                        fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
                        lngTotalOutput += intOutputPos;
                        intOutputPos = 0;
                    }
                } 
				else 
				{
                    printf("WARNING: ROM address %04X out of range (ROM size: %d)\n", 
                           intROMAddress, intROMSize);
                    /* Write placeholder for bad address */
                    arrOutputBuffer[intOutputPos++] = '?';
                    
                    /* Flush output buffer when full */
                    if (intOutputPos >= 1024) 
					{
                        fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
                        lngTotalOutput += intOutputPos;
                        intOutputPos = 0;
                    }
                }
            }
        }
        
        /* Progress indicator */
        if (intChunkCount % 50 == 0) 
		{
            printf("Processed %d chunks (%ld bytes)...\n", intChunkCount, lngTotalInput);
        }
    }
    
    /* Flush any remaining output */
    if (intOutputPos > 0) 
	{
        fwrite(arrOutputBuffer, 1, intOutputPos, fOutput);
        lngTotalOutput += intOutputPos;
    }
    
    fclose(fInput);
    fclose(fOutput);
    
    printf("\nDecoding complete:\n");
    printf("Input file:  %ld bytes\n", lngTotalInput);
    printf("Output file: %ld bytes\n", lngTotalOutput);
    printf("Compression ratio: %.1f:1\n", (float)lngTotalInput / (float)lngTotalOutput);
}

/* Main program */
main(int intArgC_a, *arrArgs_a[])
{
    char* strROMFilename;
    char* strInputFilename;
    char* strOutputFilename;
    
    printf("Small-C ZOSCII Decoder for CP/M - Streaming Version\n");
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
        printf("Usage: ZDECODE <inputfile> <outputfile>\n");
        printf("   or: ZDECODE <romfile> <inputfile> <outputfile>\n");
        printf("\nExamples:\n");
        printf("  ZDECODE ENCODED.BIN DECODED.TXT        (uses ROM.BIN)\n");
        printf("  ZDECODE MYROM.BIN ENCODED.BIN DECODED.TXT\n");
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
    
    printf("ROM loaded - ready to decode\n");
    printf("ROM starts at: %04X (%d bytes)\n", intROMStart, intROMSize);
    printf("Available TPA: %d bytes\n", intROMStart - intTPAStart);
    
    /* Stream decode input file */
    printf("\nStarting file decoding...\n");
    decode_input_streaming(strInputFilename, strOutputFilename);
    
    printf("\nZOSCII decoding complete!\n");
    printf("Files: %s -> %s\n", strInputFilename, strOutputFilename);
    printf("ROM: %s (loaded at %04X)\n", strROMFilename, intROMStart);
    
    return 0;
}
