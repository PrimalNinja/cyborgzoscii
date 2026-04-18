// Cyborg ZOSCII v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
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
    ByteAddresses arrLookup[256];
} ROMData;

#define ZOSCII_ROM_LOAD_MAX 131072L

static void buildLookupTable(ROMData* ptrROMData_a)
{
    uint32_t arrCounts[256] = {0};
    long intHeaderSize = 0;
    int intI = 0;
    long intJ = 0;
    
    // Initialize lookup array
    for (intI = 0; intI < 256; intI++)
    {
        ptrROMData_a->arrLookup[intI].ptrAddresses = NULL;
        ptrROMData_a->arrLookup[intI].intCount = 0;
    }
    
    // Header addresses must be within the first 64KB (16-bit addresses)
    intHeaderSize = (ptrROMData_a->lngROMSize > 65536L) ? 65536L : ptrROMData_a->lngROMSize;
    
    // Count occurrences
    for (intJ = 0; intJ < intHeaderSize; intJ++)
    {
        arrCounts[ptrROMData_a->ptrROMData[intJ]]++;
    }
    
    // Allocate memory for each byte value
    for (intI = 0; intI < 256; intI++)
    {
        if (arrCounts[intI] > 0)
        {
            ptrROMData_a->arrLookup[intI].ptrAddresses = 
                (uint32_t*)malloc(arrCounts[intI] * sizeof(uint32_t));
            ptrROMData_a->arrLookup[intI].intCount = 0;
        }
    }
    
    // Fill addresses
    for (intJ = 0; intJ < intHeaderSize; intJ++)
    {
        uint8_t by = ptrROMData_a->ptrROMData[intJ];
        ptrROMData_a->arrLookup[by].ptrAddresses[ptrROMData_a->arrLookup[by].intCount++] = (uint32_t)intJ;
    }

    // Seed rand based on ROM content
    uint32_t intROMHash = 0;
    for (intJ = 0; intJ < ptrROMData_a->lngROMSize; intJ++)
    {
        intROMHash = (intROMHash * 33) + ptrROMData_a->ptrROMData[intJ];
    }
    
    intROMHash ^= (uint32_t)time(NULL);
    
    srand(intROMHash);
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

                // Pre-build lookup table for reuse across multiple encodes
                buildLookupTable(ptrROMData);
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
    int intI = 0;

    if (ptrROMData_a)
    {
        if (ptrROMData_a->ptrROMData)
        {
            free(ptrROMData_a->ptrROMData);
        }

        for (intI = 0; intI < 256; intI++)
        {
            if (ptrROMData_a->arrLookup[intI].ptrAddresses)
            {
                free(ptrROMData_a->arrLookup[intI].ptrAddresses);
            }
        }

        free(ptrROMData_a);
    }
}

static bool encodeFile(const ROMData* ptrROMData_a, const char* strInputFile_a, const char* strOutputFile_a)
{
    bool blnSuccess = false;
    int intCh = 0;
    FILE* ptrInput = NULL;
    FILE* ptrOutput = NULL;
    
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
                if (ptrROMData_a->arrLookup[by].intCount > 0)
                {
                    uint32_t intRandomIdx = rand() % ptrROMData_a->arrLookup[by].intCount;
                    uint16_t intAddress = (uint16_t)ptrROMData_a->arrLookup[by].ptrAddresses[intRandomIdx];
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
    bool blnEncodeOk = false;
    int intResult = 1;
    ROMData* ptrROMData = NULL;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Encoder v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (intArgC_a == 4)
    {
        ptrROMData = loadROM(strArgv_a[1]);
        if (ptrROMData)
        {
            blnEncodeOk = encodeFile(ptrROMData, strArgv_a[2], strArgv_a[3]);
            
            if (blnEncodeOk)
            {
                intResult = 0;
            }
            else
            {
                fprintf(stderr, "Encode failed\n");
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
        fprintf(stderr, "Usage: %s <romfile> <inputdatafile> <encodedoutput>\n", strArgv_a[0]);
    }
    
    return intResult;
}