// Cyborg ZOSCII v20260301
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
                    fwrite(&intAddress, sizeof(uint16_t), 1, ptrOutput);
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
    
    printf("ZOSCII Encoder\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n\n");
    
    srand((unsigned int)time(NULL));

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