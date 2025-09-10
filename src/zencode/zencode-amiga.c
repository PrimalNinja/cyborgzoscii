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

int main(int intArgC_a, char* parrArgs_a[]) 
{
    int intBittage = 16;  // Default to 16-bit for Amiga compatibility
    int intOffset = 0;
    
	printf("ZOSCII Encoder\n");
	printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    // Parse command-line arguments for bit width
    if (intArgC_a >= 2 && strcmp(parrArgs_a[1], "-32") == 0) 
	{
        intBittage = 32;
        intOffset = 1;
    } 
	else if (intArgC_a >= 2 && strcmp(parrArgs_a[1], "-16") == 0) 
	{
        intBittage = 16;
        intOffset = 1;
    }
    
    // Validate arguments
    if (intArgC_a != 4 + intOffset) 
	{
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile> <encodedoutput>\n", parrArgs_a[0]);
        return 1;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Open ROM file
    FILE* fROM = fopen(parrArgs_a[1 + intOffset], "rb");
    if (!fROM) 
	{
        perror("Error opening ROM file");
        return 1;
    }
    
    // Get ROM file size
    fseek(fROM, 0, SEEK_END);
    long lngROMSize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    
    // Limit ROM size based on bit width
    long lngMaxSize = (intBittage == 16) ? 65536 : 4294967296L;
    if (lngROMSize > lngMaxSize) 
	{
        lngROMSize = lngMaxSize;
    }
    
    // Allocate memory for ROM data
    uint8_t* pROMData = malloc(lngROMSize);
    if (!pROMData) 
	{
        fprintf(stderr, "Memory allocation failed for ROM data\n");
        fclose(fROM);
        return 1;
    }
    
    fread(pROMData, 1, lngROMSize, fROM);
    fclose(fROM);
    
    // Build address lookup tables
    ByteAddresses arrLookup[256];
    uint32_t arrROMCounts[256] = {0};
    
    // Count occurrences
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        arrROMCounts[pROMData[lngI]]++;
    }
    
    // Allocate address arrays
    for (int intI = 0; intI < 256; intI++) 
	{
        arrLookup[intI].addresses = malloc(arrROMCounts[intI] * sizeof(uint32_t));
        if (!arrLookup[intI].addresses) 
		{
            fprintf(stderr, "Memory allocation failed for lookup table %d\n", intI);
            free(pROMData);
            for (int intJ = 0; intJ < intI; intJ++) 
			{
                free(arrLookup[intJ].addresses);
            }
            return 1;
        }
        arrLookup[intI].count = 0;
    }
    
    // Populate address arrays
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        uint8_t by = pROMData[lngI];
        arrLookup[by].addresses[arrLookup[by].count++] = lngI;
    }
    
    // Open input file
    FILE* fInput = fopen(parrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening input file");
        free(pROMData);
        for (int intI = 0; intI < 256; intI++) 
		{
            free(arrLookup[intI].addresses);
        }
        return 1;
    }
    
    // Open output file
    FILE* fOutput = fopen(parrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        free(pROMData);
        for (int intI = 0; intI < 256; intI++) 
		{
            free(arrLookup[intI].addresses);
        }
        fclose(fInput);
        return 1;
    }
    
    // Process input and write output
    int c;
    while ((c = fgetc(fInput)) != EOF) 
	{
        uint8_t by = (uint8_t)c;
        if (arrLookup[by].count > 0) 
		{
            uint32_t intRandomIdx = rand() % arrLookup[by].count;
            uint32_t intAddress = arrLookup[by].addresses[intRandomIdx];
            
            if (intBittage == 16) 
			{
                uint16_t intAddress16 = (uint16_t)intAddress;
                fwrite(&intAddress16, sizeof(uint16_t), 1, fOutput);
            } 
			else 
			{
                fwrite(&intAddress, sizeof(uint32_t), 1, fOutput);
            }
        }
    }
    
    // Clean up
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    for (int intI = 0; intI < 256; intI++) 
	{
        free(arrLookup[intI].addresses);
    }
    
    return 0;
}
