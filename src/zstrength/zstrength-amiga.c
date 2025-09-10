// Cyborg ZOSCII Strength Analyzer v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

void print_large_number(double dblExponent_a) 
{
    if (dblExponent_a < 3) 
	{
        printf("~%.0f permutations", pow(10, dblExponent_a));
    } 
	else if (dblExponent_a < 6) 
	{
        printf("~%.1f thousand permutations", pow(10, dblExponent_a) / 1000.0);
    } 
	else if (dblExponent_a < 9) 
	{
        printf("~%.1f million permutations", pow(10, dblExponent_a) / 1000000.0);
    } 
	else if (dblExponent_a < 12) 
	{
        printf("~%.1f billion permutations", pow(10, dblExponent_a) / 1000000000.0);
    } 
	else if (dblExponent_a < 15) 
	{
        printf("~%.1f trillion permutations", pow(10, dblExponent_a) / 1000000000000.0);
    } 
	else if (dblExponent_a < 82) 
	{
        printf("More than all atoms in the observable universe (10^%.0f permutations)", dblExponent_a);
    } 
	else if (dblExponent_a < 1000) 
	{
        printf("Incomprehensibly massive (10^%.0f permutations)", dblExponent_a);
    } 
	else 
	{
        printf("Astronomically secure (10^%.1fM permutations)", dblExponent_a / 1000000.0);
    }
}

int main(int intArgC_a, char* parrArgs_a[]) 
{
    int intBittage = 16;  // Default to 16-bit for Amiga compatibility
    int intOffset = 0;
    
	printf("ZOSCII ROM Strength Analyzer\n");
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
    if (intArgC_a != 3 + intOffset) 
	{
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile>\n", parrArgs_a[0]);
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
    
    // Count ROM byte occurrences
    uint32_t arrROMCounts[256] = {0};
    uint32_t arrInputCounts[256] = {0};
    
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        arrROMCounts[pROMData[lngI]]++;
    }
    
    // Open input file
    FILE* fInput = fopen(parrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening input file");
        free(pROMData);
        return 1;
    }
    
    // Count input character occurrences
    int ch;
    int intInputLength = 0;
    int intCharsUsed = 0;
    
    while ((ch = fgetc(fInput)) != EOF) 
	{
        uint8_t by = (uint8_t)ch;
        arrInputCounts[by]++;
        intInputLength++;
    }
    fclose(fInput);
    
    // Count characters utilized
    for (int intI = 0; intI < 256; intI++) 
	{
        if (arrInputCounts[intI] > 0) 
		{
            intCharsUsed++;
        }
    }
    
    // Calculate ROM strength metrics
    double dblGeneralStrength = 0.0;
    double dblFileStrength = 0.0;
    
    for (int intI = 0; intI < 256; intI++) 
	{
        if (arrROMCounts[intI] > 0) 
		{
            dblGeneralStrength += log10(arrROMCounts[intI]);
        }
        if (arrInputCounts[intI] > 0 && arrROMCounts[intI] > 0) 
		{
            dblFileStrength += arrInputCounts[intI] * log10(arrROMCounts[intI]);
        }
    }
    
    double dblUtilisation = (intCharsUsed / 256.0) * 100.0;
    
    // Output results
    printf("ROM Strength Analysis (%d-bit)\n", intBittage);
    printf("===============================\n\n");
    
    printf("Input Information:\n");
    printf("- Text Length: %d characters\n", intInputLength);
    printf("- Characters Utilized: %d of 256 (%.1f%%)\n", intCharsUsed, dblUtilisation);
    printf("\n");
    
    printf("General ROM Capacity: ~10^%.0f (", dblGeneralStrength);
    print_large_number(dblGeneralStrength);
    printf(")\n");
    
    printf("This File Security: ~10^%.0f (", dblFileStrength);
    print_large_number(dblFileStrength);
    printf(")\n\n");
    
    printf("Byte Analysis:\n");
    printf("Byte  Dec  ROM Count  Input Count  Char\n");
    printf("----  ---  ---------  -----------  ----\n");
    
    for (int intI = 0; intI < 256; intI++) 
	{
        if (arrROMCounts[intI] > 0 || arrInputCounts[intI] > 0) 
		{
            char chDisplay = (intI >= 32 && intI <= 126) ? intI : ' ';
            printf("0x%02X  %3d  %9u  %11u    %c\n", 
                   intI, intI, arrROMCounts[intI], arrInputCounts[intI], chDisplay);
        }
    }
    
    free(pROMData);
    return 0;
}
