// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define ZOSCII_ROM_LOAD_MAX 131072L

typedef struct 
{
    uint32_t* ptrAddresses;
    uint32_t intCount;
} ByteAddresses;

typedef struct 
{
    uint8_t* ptrROMData;
    long lngROMSize;
    ByteAddresses arrLookup[256];
} RomData;

static void leWrite(const void* ptrData_a, size_t intSize_a, size_t intCount_a, FILE* ptrFile_a)
{
    size_t lngI = 0;
    const uint8_t* ptrSrc = (const uint8_t*)ptrData_a;
    
    for (lngI = 0; lngI < intCount_a; lngI++)
    {
        if (intSize_a == 2)
        {
            uint16_t intValue = *(const uint16_t*)(ptrSrc + (lngI * intSize_a));
            uint8_t arrBytes[2];
            arrBytes[0] = (uint8_t)(intValue & 0xFF);
            arrBytes[1] = (uint8_t)(intValue >> 8);
            fwrite(arrBytes, 1, 2, ptrFile_a);
        }
        else
        {
            fwrite(ptrSrc + (lngI * intSize_a), intSize_a, 1, ptrFile_a);
        }
    }
}

static void buildLookupTable(RomData* ptrRom_a)
{
    uint32_t arrCounts[256] = {0};
    long lngROMSize = 0;
    long lngI = 0;
    int intI = 0;
    
    // Initialize lookup array
    for (intI = 0; intI < 256; intI++) 
    {
        ptrRom_a->arrLookup[intI].ptrAddresses = NULL;
        ptrRom_a->arrLookup[intI].intCount = 0;
    }
    
    // ROM addresses are 16-bit, so only use first 64KB
    lngROMSize = ptrRom_a->lngROMSize;
    if (lngROMSize > 65536L) 
    {
        lngROMSize = 65536L;
    }
    
    // Count occurrences
    for (lngI = 0; lngI < lngROMSize; lngI++) 
    {
        arrCounts[ptrRom_a->ptrROMData[lngI]]++;
    }
    
    // Allocate memory for each byte value
    for (intI = 0; intI < 256; intI++) 
    {
        if (arrCounts[intI] > 0) 
        {
            ptrRom_a->arrLookup[intI].ptrAddresses = (uint32_t*)malloc(arrCounts[intI] * sizeof(uint32_t));
            ptrRom_a->arrLookup[intI].intCount = 0;
        }
    }
    
    // Fill addresses
    for (lngI = 0; lngI < lngROMSize; lngI++) 
    {
        uint8_t by = ptrRom_a->ptrROMData[lngI];
        ptrRom_a->arrLookup[by].ptrAddresses[ptrRom_a->arrLookup[by].intCount++] = (uint32_t)lngI;
    }

	// seed rand based on ROM
    uint32_t intRomHash = 0;
    for (lngI = 0; lngI < ptrRom_a->lngROMSize; lngI++) 
    {
        intRomHash = (intRomHash * 33) + ptrRom_a->ptrROMData[lngI];
    }
    
    intRomHash ^= (uint32_t)time(NULL);
    
    srand(intRomHash);
}

static RomData* loadRom(const char* strFilename_a)
{
    RomData* ptrRom = NULL;
    FILE* ptrROM = NULL;
    
    ptrROM = fopen(strFilename_a, "rb");
    if (ptrROM) 
    {
        ptrRom = (RomData*)malloc(sizeof(RomData));
        if (ptrRom) 
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
                
                // Pre-build lookup table for reuse across multiple encodes
                buildLookupTable(ptrRom);
            } 
            else 
            {
                free(ptrRom);
                ptrRom = NULL;
            }
        }
        fclose(ptrROM);
    }
    
    return ptrRom;
}

static void unloadRom(RomData* ptrRom_a)
{
    int intI = 0;
    
    if (ptrRom_a) 
    {
        if (ptrRom_a->ptrROMData) 
        {
            free(ptrRom_a->ptrROMData);
        }
        
        for (intI = 0; intI < 256; intI++) 
        {
            if (ptrRom_a->arrLookup[intI].ptrAddresses) 
            {
                free(ptrRom_a->arrLookup[intI].ptrAddresses);
            }
        }
        
        free(ptrRom_a);
    }
}

static bool encodeFile(const RomData* ptrRom_a, const char* strInputFile_a, const char* strOutputFile_a)
{
    bool blnSuccess = false;
    FILE* ptrInput = NULL;
    FILE* ptrOutput = NULL;
    int intCh = 0;
    
    ptrInput = fopen(strInputFile_a, "rb");
    if (ptrInput) 
    {
        ptrOutput = fopen(strOutputFile_a, "wb");
        if (ptrOutput) 
        {
            // Stream-encode input
            while ((intCh = fgetc(ptrInput)) != EOF) 
            {
                uint8_t by = (uint8_t)intCh;
                if (ptrRom_a->arrLookup[by].intCount > 0) 
                {
                    uint32_t intRandomIdx = rand() % ptrRom_a->arrLookup[by].intCount;
                    uint16_t intAddress = (uint16_t)ptrRom_a->arrLookup[by].ptrAddresses[intRandomIdx];
                    leWrite(&intAddress, sizeof(uint16_t), 1, ptrOutput);
                }
            }
            
            blnSuccess = true;
            
            fclose(ptrOutput);
        }
        fclose(ptrInput);
    }
    
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    int intResult = 1;
    RomData* ptrRom = NULL;
    bool blnEncodeOk = false;
    
    printf("ZOSCII Encoder v20260303\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (intArgC_a == 4) 
    {
        ptrRom = loadRom(strArgv_a[1]);
        if (ptrRom) 
        {
            blnEncodeOk = encodeFile(ptrRom, strArgv_a[2], strArgv_a[3]);
            unloadRom(ptrRom);
            
            if (blnEncodeOk) 
            {
                intResult = 0;
            } 
            else 
            {
                fprintf(stderr, "Encode failed\n");
            }
        } 
        else 
        {
            perror("Failed to load ROM");
        }
    } 
    else 
    {
        fprintf(stderr, "Usage: %s <romfile> <inputdatafile> <encodedoutput>\n", strArgv_a[0]);
    }
    
    return intResult;
}