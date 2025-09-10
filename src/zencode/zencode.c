// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

typedef struct 
{
    uint32_t* addresses;
    uint32_t count;
} ByteAddresses;

int main(int intArgC_a, char *arrArgs_a[]) 
{
#ifdef _WIN32
    // Set binary mode for stdin/stdout if needed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Encoder\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    int intBittage = 16;  // default
    int intOffset = 0;
    
    if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-32") == 0) 
	{
        intBittage = 32;
        intOffset = 1;
    } 
	else if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-16") == 0) 
	{
        intBittage = 16;
        intOffset = 1;
    }
    
    if (intArgC_a != 4 + intOffset) 
	{
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile> <encodedoutput>\n", arrArgs_a[0]);
        return 1;
    }
    
    srand(time(NULL));
    
    FILE* fROM = fopen(arrArgs_a[1 + intOffset], "rb");
    if (!fROM) 
	{
        perror("Error opening ROM file");
        return 1;
    }
    
    fseek(fROM, 0, SEEK_END);
    long lngROMSize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    
    // Check ROM size limit based on bit width
    long lngMaxSize = (intBittage == 16) ? 65536 : 4294967296L;
    if (lngROMSize > lngMaxSize) 
	{
        lngROMSize = lngMaxSize;
    }
    
    uint8_t* pROMData = malloc(lngROMSize);
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
        arrLookup[intI].count = 0;
    }
    
    // Populate address arrays
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        uint8_t by = pROMData[lngI];
        arrLookup[by].addresses[arrLookup[by].count++] = lngI;
    }
    
    FILE* fInput = fopen(arrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening input file");
        return 1;
    }
    
    FILE* fOutput = fopen(arrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        return 1;
    }
    
    int ch;
    while ((ch = fgetc(fInput)) != EOF) 
	{
        uint8_t by = (uint8_t)ch;
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
    
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    for (int intI = 0; intI < 256; intI++) 
	{
        free(arrLookup[intI].addresses);
    }
    
    return 0;
}
