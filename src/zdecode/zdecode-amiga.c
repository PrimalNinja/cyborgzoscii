// Cyborg ZOSCII Decoder v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int intArgC_a, char* parrArgs_a[]) 
{
    int intBittage = 16;  // Default to 16-bit for Amiga compatibility
    int intOffset = 0;
    
	printf("ZOSCII Decoder\n");
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
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", parrArgs_a[0]);
        return 1;
    }
    
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
    
    // Open encoded input file
    FILE* fInput = fopen(parrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening encoded input file");
        free(pROMData);
        return 1;
    }
    
    // Open output file
    FILE* fOutput = fopen(parrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        free(pROMData);
        fclose(fInput);
        return 1;
    }
    
    // Decode input based on bit width
    if (intBittage == 16) 
	{
        uint16_t intAddress;
        while (fread(&intAddress, sizeof(uint16_t), 1, fInput) == 1) 
		{
            if (intAddress < lngROMSize) 
			{
                fputc(pROMData[intAddress], fOutput);
            }
        }
    } 
	else 
	{
        uint32_t intAddress;
        while (fread(&intAddress, sizeof(uint32_t), 1, fInput) == 1) 
		{
            if (intAddress < lngROMSize) 
			{
                fputc(pROMData[intAddress], fOutput);
            }
        }
    }
    
    // Clean up
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    
    return 0;
}
