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

int main(int intArgC_a, char* arrArgs_a[]) 
{
#ifdef _WIN32
    // Set binary mode for stdin/stdout if needed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Decoder\n");
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
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", arrArgs_a[0]);
        return 1;
    }
    
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
    
    FILE* fInput = fopen(arrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening encoded input file");
        return 1;
    }
    
    FILE* fOutput = fopen(arrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        return 1;
    }
    
    if (intBittage == 16) 
	{
        uint16_t intAddress16;
        while (fread(&intAddress16, sizeof(uint16_t), 1, fInput) == 1) 
		{
            if (intAddress16 < lngROMSize) 
			{
                fputc(pROMData[intAddress16], fOutput);
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
    
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    
    return 0;
}
