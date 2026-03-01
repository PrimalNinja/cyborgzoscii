// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Amiga Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
    if (ptrROM != NULL) 
    {
        ptrRom = (RomData*)malloc(sizeof(RomData));
        if (ptrRom != NULL) 
        {
            fseek(ptrROM, 0, SEEK_END);
            ptrRom->lngROMSize = ftell(ptrROM);
            fseek(ptrROM, 0, SEEK_SET);
            
            if (ptrRom->lngROMSize > ZOSCII_ROM_LOAD_MAX) 
            {
                ptrRom->lngROMSize = ZOSCII_ROM_LOAD_MAX;
            }
            
            ptrRom->ptrROMData = (uint8_t*)malloc(ptrRom->lngROMSize);
            if (ptrRom->ptrROMData != NULL) 
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
    if (ptrRom_a != NULL) 
    {
        if (ptrRom_a->ptrROMData != NULL) 
        {
            free(ptrRom_a->ptrROMData);
        }
        free(ptrRom_a);
    }
}

static int decodeFile(const RomData* ptrRom_a, const char* strInputFile_a, const char* strOutputFile_a)
{
    int intResult = 0;
    FILE* ptrInput = NULL;
    FILE* ptrOutput = NULL;
    uint8_t arrBuf[2];
    long lngInputSize = 0;
    long lngSlots = 0;
    int intI = 0;
    
    ptrInput = fopen(strInputFile_a, "rb");
    if (ptrInput != NULL) 
    {
        fseek(ptrInput, 0, SEEK_END);
        lngInputSize = ftell(ptrInput);
        fseek(ptrInput, 0, SEEK_SET);
        
        lngSlots = lngInputSize / 2;
        
        if (lngSlots >= 0) 
        {
            ptrOutput = fopen(strOutputFile_a, "wb");
            if (ptrOutput != NULL) 
            {
                // Decode each slot
                for (intI = 0; intI < (int)lngSlots; intI++) 
                {
                    if (fread(arrBuf, 2, 1, ptrInput) != 1) 
                    {
                        break;
                    }
                    
                    uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);
                    if ((long)intAddr < ptrRom_a->lngROMSize) 
                    {
                        if (fputc(ptrRom_a->ptrROMData[(long)intAddr], ptrOutput) == EOF) 
                        {
                            break;
                        }
                    }
                }
                
                if (intI == (int)lngSlots) 
                {
                    intResult = 1;
                }
                
                fclose(ptrOutput);
            }
        }
        fclose(ptrInput);
    }
    
    return intResult;
}

int main(int intArgC_a, char* strArgv_a[])
{
    int intResult = 1;
    RomData* ptrRom = NULL;
    int intDecodeOk = 0;
    
    printf("ZOSCII Decoder\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n\n");

    if (intArgC_a == 4) 
    {
        ptrRom = loadRom(strArgv_a[1]);
        if (ptrRom != NULL) 
        {
            intDecodeOk = decodeFile(ptrRom, strArgv_a[2], strArgv_a[3]);
            freeRom(ptrRom);
            
            if (intDecodeOk != 0) 
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