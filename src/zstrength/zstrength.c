// Cyborg ZOSCII v20260418
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
    uint32_t arrROMCountsHigh[256];
} ROMData;

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

static ROMData* loadROM(const char* strFilename_a)
{
    ROMData* ptrROMData = NULL;
    FILE* ptrROMFile = NULL;
    
    ptrROMData = (ROMData*)malloc(sizeof(ROMData));
    if (ptrROMData)
    {
        // Initialize
        memset(ptrROMData, 0, sizeof(ROMData));
        
        ptrROMFile = fopen(strFilename_a, "rb");
        if (ptrROMFile)
        {
            fseek(ptrROMFile, 0, SEEK_END);
            ptrROMData->lngROMSize = ftell(ptrROMFile);
            fseek(ptrROMFile, 0, SEEK_SET);
            
            if (ptrROMData->lngROMSize > ZOSCII_ROM_LOAD_MAX)
            {
                ptrROMData->lngROMSize = ZOSCII_ROM_LOAD_MAX;
            }
            
            ptrROMData->ptrROMData = (uint8_t*)malloc(ptrROMData->lngROMSize);
            if (ptrROMData->ptrROMData)
            {
                fread(ptrROMData->ptrROMData, 1, ptrROMData->lngROMSize, ptrROMFile);
                
                // Count ROM byte occurrences - first 64KB (encoding range)
                long lngLowSize = (ptrROMData->lngROMSize > 65536L) ? 65536L : ptrROMData->lngROMSize;
                for (long lngI = 0; lngI < lngLowSize; lngI++)
                {
                    ptrROMData->arrROMCounts[ptrROMData->ptrROMData[lngI]]++;
                }
                
                // Count ROM byte occurrences - second 64KB (if present)
                for (long lngI = 65536L; lngI < ptrROMData->lngROMSize; lngI++)
                {
                    ptrROMData->arrROMCountsHigh[ptrROMData->ptrROMData[lngI]]++;
                }
            }
            else
            {
                free(ptrROMData);
                ptrROMData = NULL;
            }
            
            fclose(ptrROMFile);
        }
        else
        {
            free(ptrROMData);
            ptrROMData = NULL;
        }
    }
    
    return ptrROMData;
}

static void unloadROM(ROMData* ptrROMData_a)
{
    if (ptrROMData_a)
    {
        if (ptrROMData_a->ptrROMData)
        {
            free(ptrROMData_a->ptrROMData);
        }
        
        free(ptrROMData_a);
    }
}

static bool analyzeFile(const ROMData* ptrROMData_a, const char* strInputFile_a)
{
    uint32_t arrInputCounts[256] = {0};
    bool blnSuccess = false;
    double dblFileStrength = 0.0;
    double dblGeneralStrength = 0.0;
    double dblUtilisation = 0.0;
    int intCh = 0;
    int intCharsUsed = 0;
    int intInputLength = 0;
    FILE* ptrInput = NULL;
    
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
            if (ptrROMData_a->arrROMCounts[intI] > 0)
            {
                dblGeneralStrength += log10(ptrROMData_a->arrROMCounts[intI]);
            }
            if (arrInputCounts[intI] > 0 && ptrROMData_a->arrROMCounts[intI] > 0)
            {
                dblFileStrength += arrInputCounts[intI] * log10(ptrROMData_a->arrROMCounts[intI]);
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
        printf("Byte  Dec  ROM Lo 64K  ROM Hi 64K  Input Count  Char\n");
        printf("----  ---  ----------  ----------  -----------  ----\n");
        
        for (int intI = 0; intI < 256; intI++)
        {
            if (ptrROMData_a->arrROMCounts[intI] > 0 || ptrROMData_a->arrROMCountsHigh[intI] > 0 || arrInputCounts[intI] > 0)
            {
                char chDisplay = (intI >= 32 && intI <= 126) ? (char)intI : ' ';
                printf("0x%02X  %3d  %10u  %10u  %11u    %c\n", 
                       intI, intI, ptrROMData_a->arrROMCounts[intI], ptrROMData_a->arrROMCountsHigh[intI], arrInputCounts[intI], chDisplay);
            }
        }
        
        blnSuccess = true;
    }
    
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    bool blnAnalyzeOk = false;
    int intResult = 1;
    ROMData* ptrROMData = NULL;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII ROM Strength Analyzer v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (intArgC_a == 3)
    {
        ptrROMData = loadROM(strArgv_a[1]);
        if (ptrROMData)
        {
            blnAnalyzeOk = analyzeFile(ptrROMData, strArgv_a[2]);
            
            if (blnAnalyzeOk)
            {
                intResult = 0;
            }
            else
            {
                fprintf(stderr, "Analysis failed\n");
            }
            
            unloadROM(ptrROMData);
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