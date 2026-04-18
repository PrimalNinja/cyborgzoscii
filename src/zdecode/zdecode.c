// Cyborg ZOSCII v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

#define ZOSCII_ROM_LOAD_MAX 131072L

typedef struct 
{
    uint8_t* ptrROMData;
    long lngROMSize;
} ROMData;

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

static bool decodeFile(const ROMData* ptrROMData_a, const char* strInputFile_a, const char* strOutputFile_a)
{
    uint8_t arrBuf[2];
    bool blnSuccess = false;
    long intI = 0;
    long intInputSize = 0;
    long intSlots = 0;
    FILE* ptrInput = NULL;
    FILE* ptrOutput = NULL;
    
    ptrInput = fopen(strInputFile_a, "rb");
    if (ptrInput) 
    {
        // Get input file size to calculate number of slots
        fseek(ptrInput, 0, SEEK_END);
        intInputSize = ftell(ptrInput);
        fseek(ptrInput, 0, SEEK_SET);
        
        // Each slot is 2 bytes (16-bit address)
        intSlots = intInputSize / 2;
        
        if (intSlots >= 0) 
        {
            ptrOutput = fopen(strOutputFile_a, "wb");
            if (ptrOutput) 
            {
                // Decode each slot
                for (intI = 0; intI < intSlots; intI++) 
                {
                    if (fread(arrBuf, 2, 1, ptrInput) != 1) 
                    {
                        break;
                    }
                    
                    uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);
                    if (intAddr < ptrROMData_a->lngROMSize) 
                    {
                        if (fputc(ptrROMData_a->ptrROMData[intAddr], ptrOutput) == EOF) 
                        {
                            break;
                        }
                    }
                }
                
                if (intI == intSlots) 
                {
                    blnSuccess = true;
                }
                
                fclose(ptrOutput);
            }
        }
        fclose(ptrInput);
    }
    
    return blnSuccess;
}

int main(int argc_a, char* strArgv_a[])
{
    bool blnDecodeOk = false;
    int intResult = 1;
    ROMData* ptrROMData = NULL;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Decoder v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc_a == 4) 
    {
        ptrROMData = loadROM(strArgv_a[1]);
        if (ptrROMData) 
        {
            blnDecodeOk = decodeFile(ptrROMData, strArgv_a[2], strArgv_a[3]);
            unloadROM(ptrROMData);
            
            if (blnDecodeOk) 
            {
                intResult = 0;
            } 
            else 
            {
                fprintf(stderr, "Decode failed\n");
            }
        } 
        else 
        {
            perror("Failed to load ROM");
        }
    } 
    else 
    {
        fprintf(stderr, "Usage: %s <romfile> <encoded> <output>\n", strArgv_a[0]);
    }
    
    return intResult;
}