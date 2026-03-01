// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

typedef struct 
{
    uint32_t* ptrAddresses;
    uint32_t intCount;
} ByteAddresses;

typedef struct 
{
    uint8_t* ptrROMData;
    long lngROMSize;
    uint32_t arrROMCounts[256];
} RomData;

#define ZOSCII_ROM_LOAD_MAX 131072L

static void printLargeNumber(double dblExponent_a) 
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

static RomData* loadRom(const char* strFilename_a)
{
    RomData* ptrRom = NULL;
    FILE* ptrROM = NULL;
    
    ptrRom = (RomData*)malloc(sizeof(RomData));
    if (ptrRom)
    {
        // Initialize
        memset(ptrRom, 0, sizeof(RomData));
        
        ptrROM = fopen(strFilename_a, "rb");
        if (ptrROM)
        {
            fseek(ptrROM, 0, SEEK_END);
            ptrRom->lngROMSize = ftell(ptrROM);
            fseek(ptrROM, 0, SEEK_SET);
            
            if (ptrRom->lngROMSize > ZOSCII_ROM_LOAD_MAX)
            {
                ptrRom->lngROMSize = ZOSCII_ROM_LOAD_MAX;
            }
            
            ptrRom->ptrROMData = (uint8_t*)malloc(ptrRom->lngROMSize);
            if (ptrRom->ptrROMData)
            {
                fread(ptrRom->ptrROMData, 1, ptrRom->lngROMSize, ptrROM);
                
                // Count ROM byte occurrences
                for (long lngI = 0; lngI < ptrRom->lngROMSize; lngI++)
                {
                    ptrRom->arrROMCounts[ptrRom->ptrROMData[lngI]]++;
                }
            }
            else
            {
                free(ptrRom);
                ptrRom = NULL;
            }
            
            fclose(ptrROM);
        }
        else
        {
            free(ptrRom);
            ptrRom = NULL;
        }
    }
    
    return ptrRom;
}

static void unloadRom(RomData* ptrRom_a)
{
    if (ptrRom_a)
    {
        if (ptrRom_a->ptrROMData)
        {
            free(ptrRom_a->ptrROMData);
        }
        free(ptrRom_a);
    }
}

static bool analyzeFile(const RomData* ptrRom_a, const char* strInputFile_a)
{
    bool blnSuccess = false;
    FILE* ptrInput = NULL;
    uint32_t arrInputCounts[256] = {0};
    int intInputLength = 0;
    int intCharsUsed = 0;
    int intCh = 0;
    double dblGeneralStrength = 0.0;
    double dblFileStrength = 0.0;
    double dblUtilisation = 0.0;
    
    ptrInput = fopen(strInputFile_a, "rb");
    if (ptrInput)
    {
        // Count input character occurrences
        while ((intCh = fgetc(ptrInput)) != EOF)
        {
            uint8_t by = (uint8_t)intCh;
            arrInputCounts[by]++;
            intInputLength++;
        }
        fclose(ptrInput);
        
        // Count characters utilized
        for (int intI = 0; intI < 256; intI++)
        {
            if (arrInputCounts[intI] > 0)
            {
                intCharsUsed++;
            }
        }
        
        // Calculate ROM strength metrics
        for (int intI = 0; intI < 256; intI++)
        {
            if (ptrRom_a->arrROMCounts[intI] > 0)
            {
                dblGeneralStrength += log10(ptrRom_a->arrROMCounts[intI]);
            }
            if (arrInputCounts[intI] > 0 && ptrRom_a->arrROMCounts[intI] > 0)
            {
                dblFileStrength += arrInputCounts[intI] * log10(ptrRom_a->arrROMCounts[intI]);
            }
        }
        
        dblUtilisation = (intCharsUsed / 256.0) * 100.0;
        
        printf("ROM Strength Analysis\n");
        printf("=====================\n\n");
        
        printf("Input Information:\n");
        printf("- Text Length: %d characters\n", intInputLength);
        printf("- Characters Utilized: %d of 256 (%.1f%%)\n", intCharsUsed, dblUtilisation);
        printf("\n");
        
        printf("General ROM Capacity: ~10^%.0f (", dblGeneralStrength);
        printLargeNumber(dblGeneralStrength);
        printf(")\n");
        
        printf("This File Security: ~10^%.0f (", dblFileStrength);
        printLargeNumber(dblFileStrength);
        printf(")\n\n");
        
        printf("Byte Analysis:\n");
        printf("Byte  Dec  ROM Count  Input Count  Char\n");
        printf("----  ---  ---------  -----------  ----\n");
        
        for (int intI = 0; intI < 256; intI++)
        {
            if (ptrRom_a->arrROMCounts[intI] > 0 || arrInputCounts[intI] > 0)
            {
                char chDisplay = (intI >= 32 && intI <= 126) ? (char)intI : ' ';
                printf("0x%02X  %3d  %9u  %11u    %c\n", 
                       intI, intI, ptrRom_a->arrROMCounts[intI], arrInputCounts[intI], chDisplay);
            }
        }
        
        blnSuccess = true;
    }
    
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    int intResult = 1;
    RomData* ptrRom = NULL;
    bool blnAnalyzeOk = false;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII ROM Strength Analyzer\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n\n");

    if (intArgC_a == 3)
    {
        ptrRom = loadRom(strArgv_a[1]);
        if (ptrRom)
        {
            blnAnalyzeOk = analyzeFile(ptrRom, strArgv_a[2]);
            
            if (blnAnalyzeOk)
            {
                intResult = 0;
            }
            else
            {
                fprintf(stderr, "Analysis failed\n");
            }
            
            unloadRom(ptrRom);
        }
        else
        {
            perror("Failed to load ROM");
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <romfile> <inputdatafile>\n", strArgv_a[0]);
    }
    
    return intResult;
}