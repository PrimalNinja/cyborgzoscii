// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
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
} RomData;

static RomData* loadRom(const char* strFilename_a)
{
    RomData* ptrRom = NULL;
    FILE* ptrROM = NULL;
    
    ptrROM = fopen(strFilename_a, "rb");
    if (ptrROM) 
    {
        ptrRom = malloc(sizeof(RomData));
        if (ptrRom) 
        {
            fseek(ptrROM, 0, SEEK_END);
            ptrRom->lngROMSize = ftell(ptrROM);
            fseek(ptrROM, 0, SEEK_SET);
            
            if (ptrRom->lngROMSize > ZOSCII_ROM_LOAD_MAX) 
            {
                ptrRom->lngROMSize = ZOSCII_ROM_LOAD_MAX;
            }
            
            ptrRom->ptrROMData = malloc(ptrRom->lngROMSize);
            if (ptrRom->ptrROMData) 
            {
                fread(ptrRom->ptrROMData, 1, ptrRom->lngROMSize, ptrROM);
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

static void freeRom(RomData* ptrRom_a)
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

static bool decodeFile(const RomData* ptrRom_a, const char* strInputFile_a, const char* strOutputFile_a)
{
    bool blnSuccess = false;
    FILE* ptrInput = NULL;
    FILE* ptrOutput = NULL;
    uint8_t arrBuf[2];
    long lngInputSize = 0;
    long lngSlots = 0;
    long lngI = 0;
    
    ptrInput = fopen(strInputFile_a, "rb");
    if (ptrInput) 
    {
        // Get input file size to calculate number of slots
        fseek(ptrInput, 0, SEEK_END);
        lngInputSize = ftell(ptrInput);
        fseek(ptrInput, 0, SEEK_SET);
        
        // Each slot is 2 bytes (16-bit address)
        lngSlots = lngInputSize / 2;
        
        if (lngSlots >= 0) 
        {
            ptrOutput = fopen(strOutputFile_a, "wb");
            if (ptrOutput) 
            {
                // Decode each slot
                for (lngI = 0; lngI < lngSlots; lngI++) 
                {
                    if (fread(arrBuf, 2, 1, ptrInput) != 1) 
                    {
                        break;
                    }
                    
                    uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);
                    if (intAddr < ptrRom_a->lngROMSize) 
                    {
                        if (fputc(ptrRom_a->ptrROMData[intAddr], ptrOutput) == EOF) 
                        {
                            break;
                        }
                    }
                }
                
                if (lngI == lngSlots) 
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
    int intResult = 1;
    RomData* ptrRom = NULL;
    bool blnDecodeOk = false;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Decoder v20260303\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc_a == 4) 
    {
        ptrRom = loadRom(strArgv_a[1]);
        if (ptrRom) 
        {
            blnDecodeOk = decodeFile(ptrRom, strArgv_a[2], strArgv_a[3]);
            freeRom(ptrRom);
            
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