/* Cyborg ZOSCII v20250805
   (c) 2025 Cyborg Unicorn Pty Ltd.
   This software is released under MIT License. */

/* Small-C for CP/M - ZOSCII ROM Strength Analyzer */
#include "stdio.h"

/* Same memory layout as encoder:
   program
   1.5kb table header (occurrence count+block pointer+memory pointer)
   32kb address tables
   16kb of ROM
   some I/O buffers */

/* Lookup table structure - 1536 bytes total */
/* 256 entries x 6 bytes each = 3 words per entry */
int arrLookup[768];  /* 256 * 3 = 768 words */

/* Memory pointers */
char* intTPAStart;
char* intTPAEnd;
char* intROMStart;
int intAvailableTPA;
int intROMSize;

/* Analysis data */
int arrInputCounts[256];     /* Character frequency in input file */
char arrInputBuffer[1024];    /* 1KB input chunk buffer */

/* Same memory initialization as encoder */
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

/* Same ROM loading as encoder */
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

/* PASS 1 - Same as encoder: Count byte occurrences in ROM */
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
    
    /* Scan entire ROM */
    pROM = intROMStart;
	for (intI = 0; intI < intROMSize; intI++) 
	{
		by = *pROM & 0xFF;
		arrLookup[by * 3]++;
		pROM++;
	}
    
    printf("ROM byte counting complete\n");
}

/* PASS 2 - Same as encoder: Allocate address list blocks */
void allocate_blocks()
{
    char* pAlloc;
    int by;
    int intOccuranceCount;
    int intBlockSize;
    
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
    
    printf("Address block allocation complete, used %d bytes\n", pAlloc - intTPAStart);
    
    /* Check if we have enough space before ROM */
    if (pAlloc >= intROMStart) 
	{
        printf("ERROR: Address tables overflow into ROM space!\n");
        exit(1);
    }
}

/* PASS 3 - Same as encoder: Populate address lists with ROM addresses */
void populate_address_lists()
{
    char* pROM;
    int by;
    int* intFillCounter;
    int intI;
    char* pWrite;
    
    /* Scan ROM and populate address lists */
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
    
    printf("ROM address lists populated\n");
}

/* PASS 4 - NEW: Analyze input file character frequency */
void analyze_input_file(char* strFilename_a)
{
    FILE* fInput;
    int by;
    int intBytesRead;
    int intI;
    int intUniqueChars = 0;
    long lngTotalChars = 0;
    
    /* Initialize input character counts */
    for (intI = 0; intI < 256; intI++) 
	{
        arrInputCounts[intI] = 0;
    }
    
    fInput = fopen(strFilename_a, "rb");
    if (!fInput) 
	{
        printf("Cannot open input file: %s\n", strFilename_a);
        return;
    }
    
    printf("Analyzing input file: %s\n", strFilename_a);
    
    /* Stream input file in 1KB chunks */
    while ((intBytesRead = fread(arrInputBuffer, 1, 1024, fInput)) > 0) 
	{
        lngTotalChars += intBytesRead;
        
        /* Count each byte in this chunk */
        for (intI = 0; intI < intBytesRead; intI++) 
		{
            by = arrInputBuffer[intI] & 0xFF;
            arrInputCounts[by]++;
        }
    }
    
    fclose(fInput);
    
    /* Count unique characters */
    for (intI = 0; intI < 256; intI++) 
	{
        if (arrInputCounts[intI] > 0) 
		{
            intUniqueChars++;
        }
    }
    
    printf("Input analysis complete: %ld characters, %d unique\n", 
           lngTotalChars, intUniqueChars);
}

/* Simple integer logarithm approximation (base 10) */
int log10_approx(int intValue_a)
{
    int intResult = 0;
    
    if (intValue_a < 1) return 0;
    
    while (intValue_a >= 10) 
	{
        intValue_a /= 10;
        intResult++;
    }
    
    return intResult;
}

/* PASS 5 - NEW: Calculate and display ROM strength */
void calculate_strength()
{
	int intByteLog;
    int intCoveredBytes = 0;       /* Input bytes that have ROM coverage */
    int intFileStrength = 0;       /* Sum of intInputCount * log10(intROMCount) */
    int intGeneralStrength = 0;    /* Sum of log10(arrROMCounts) */
    int intI;
    int intInputBytesUsed = 0;    /* How many different bytes in input */
	int intInputCount;
    int intROMBytesUsed = 0;      /* How many different bytes appear in ROM */
    int intROMCount;
    
    printf("\nROM Strength Analysis\n");
    printf("=====================\n");
    
    /* Calculate strength metrics */
    for (intI = 0; intI < 256; intI++) 
	{
        intROMCount = arrLookup[intI * 3];      /* ROM occurrences */
        intInputCount = arrInputCounts[intI];        /* Input occurrences */
        
        if (intROMCount > 0) 
		{
            intROMBytesUsed++;
            intByteLog = log10_approx(intROMCount);
            intGeneralStrength += intByteLog;
        }
        
        if (intInputCount > 0) 
		{
            intInputBytesUsed++;
            
            if (intROMCount > 0) 
			{
                intCoveredBytes++;
                intByteLog = log10_approx(intROMCount);
                intFileStrength += intInputCount * intByteLog / 100; /* Scale down */
            }
        }
    }
    
    printf("ROM Coverage:\n");
    printf("- ROM bytes used: %d of 256 (%d%%)\n", 
           intROMBytesUsed, (intROMBytesUsed * 100) / 256);
    printf("- Input bytes used: %d of 256 (%d%%)\n", 
           intInputBytesUsed, (intInputBytesUsed * 100) / 256);
    printf("- Input bytes covered by ROM: %d of %d (%d%%)\n", 
           intCoveredBytes, intInputBytesUsed, 
           intInputBytesUsed > 0 ? (intCoveredBytes * 100) / intInputBytesUsed : 0);
    
    printf("\nStrength Estimates:\n");
    printf("- General ROM capacity: ~10^%d\n", intGeneralStrength);
    printf("- This file security: ~10^%d\n", intFileStrength);
    
    if (intCoveredBytes < intInputBytesUsed) 
	{
        printf("WARNING: %d input characters have no ROM coverage!\n", 
               intInputBytesUsed - intCoveredBytes);
    }
}

/* Display detailed byte analysis */
void print_byte_analysis()
{
    char chDisplay;
    int intI;
    int intInputCount;
    int intROMCount;
    
    printf("\nDetailed Byte Analysis:\n");
    printf("Byte Dec ROM_Count Input_Count Addresses  Char\n");
    printf("---- --- --------- ----------- ---------  ----\n");
    
    for (intI = 0; intI < 256; intI++) 
	{
        intROMCount = arrLookup[intI * 3];
        intInputCount = arrInputCounts[intI];
        
        /* Only show bytes that appear in ROM or input */
        if (intROMCount > 0 || intInputCount > 0) 
		{
            chDisplay = (intI >= 32 && i <= 126) ? intI : ' ';
            
            printf("%02X   %3d   %7d     %7d", intI, intI, intROMCount, intInputCount);
            
            /* Show first few ROM addresses for this byte */
            if (intROMCount > 0) 
			{
                char *pAddr = arrLookup[intI * 3 + 1];
                int intAddress1 = pAddr[0] | (pAddr[1] << 8);
                printf("   %04X", intAddress1);
                if (intROMCount > 1) 
				{
                    int intAddress2 = pAddr[2] | (pAddr[3] << 8);
                    printf(",%04X", intAddress2);
                    if (intROMCount > 2) 
					{
                        printf("...");
                    }
                }
            } 
			else 
			{
                printf("   NONE");
            }
            
            printf("      %c\n", chDisplay);
        }
    }
}

/* Print memory usage statistics - same as encoder */
void print_memory_stats()
{
    int by;
    int intTotalAddresses = 0;
    int intUsedBytes = 0;
    int intMinCount = 999;
    int intMaxCount = 0;
    char* pLastAlloc = intTPAStart;
    
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
            
            char* block_ptr = arrLookup[by * 3 + 1];
            if (block_ptr > pLastAlloc) pLastAlloc = block_ptr + (intCount * 2);
        }
    }
    
    printf("Address tables: %d bytes (%04X to %04X)\n", 
           intUsedBytes, intTPAStart, pLastAlloc);
    printf("Free space: %d bytes\n", intROMStart - pLastAlloc);
    printf("Total ROM addresses catalogued: %d\n", intTotalAddresses);
    printf("Address count range: %d to %d per byte\n", intMinCount, intMaxCount);
}

/* Main program */
main(int intArgC_a, *arrArgs_a[])
{
    char* strInputFilename;
    char* strROMFilename;
    
    printf("Small-C ZOSCII ROM Strength Analyzer for CP/M\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    /* Check command line arguments */
    if (intArgC_a == 2) 
	{
        /* Single argument - input file, use default ROM.BIN */
        strROMFilename = "ROM.BIN";
        strInputFilename = arrArgs_a[1];
        printf("Using default ROM: ROM.BIN\n");
        printf("Input file: %s\n\n", strInputFilename);
    }
    else if (intArgC_a == 3) 
	{
        /* Two arguments - ROM file and input file */
        strROMFilename = arrArgs_a[1];
        strInputFilename = arrArgs_a[2];
        printf("ROM file: %s\n", strROMFilename);
        printf("Input file: %s\n\n", strInputFilename);
    }
    else 
	{
        printf("Usage: ZSTRENGTH <inputfile>\n");
        printf("   or: ZSTRENGTH <romfile> <inputfile>\n");
        printf("\nExamples:\n");
        printf("  ZSTRENGTH MYTEXT.TXT        (uses ROM.BIN)\n");
        printf("  ZSTRENGTH MYROM.BIN DATA.TXT\n");
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
    
    /* PASS 1: Analyze ROM and build lookup structures */
    printf("PASS 1: Counting ROM bytes...\n");
    count_bytes();
    
    /* PASS 2: Allocate address list blocks */
    printf("PASS 2: Allocating address blocks...\n");
    allocate_blocks();
    
    /* PASS 3: Fill address lists with ROM positions */
    printf("PASS 3: Populating ROM address lists...\n");
    populate_address_lists();
    
    /* Show memory usage */
    print_memory_stats();
    
    /* PASS 4: Analyze input file */
    printf("\nPASS 4: Analyzing input file...\n");
    analyze_input_file(strInputFilename);
    
    /* PASS 5: Calculate and display strength */
    printf("\nPASS 5: Calculating ROM strength...\n");
    calculate_strength();
    
    /* Show detailed analysis */
    print_byte_analysis();
    
    printf("\nZOSCII ROM strength analysis complete!\n");
    printf("ROM: %s, Input: %s\n", strROMFilename, strInputFilename);
    
    return 0;
}
