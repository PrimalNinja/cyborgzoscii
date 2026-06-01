// Cyborg ZOSCII v20260601
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
        memset(ptrROMData, 0, sizeof(ROMData));
        ptrROMFile = fopen(strFilename_a, "rb");
        if (ptrROMFile)
        {
            fseek(ptrROMFile, 0, SEEK_END);
            ptrROMData->lngROMSize = ftell(ptrROMFile);
            fseek(ptrROMFile, 0, SEEK_SET);
            if (ptrROMData->lngROMSize > ZOSCII_ROM_LOAD_MAX) { ptrROMData->lngROMSize = ZOSCII_ROM_LOAD_MAX; }
            ptrROMData->ptrROMData = (uint8_t*)malloc(ptrROMData->lngROMSize);
            if (ptrROMData->ptrROMData) { fread(ptrROMData->ptrROMData, 1, ptrROMData->lngROMSize, ptrROMFile); }
            else { free(ptrROMData); ptrROMData = NULL; }
            fclose(ptrROMFile);
        }
        else { free(ptrROMData); ptrROMData = NULL; }
    }
    return ptrROMData;
}

static void unloadROM(ROMData* ptrROMData_a)
{
    if (ptrROMData_a)
    {
        if (ptrROMData_a->ptrROMData) { free(ptrROMData_a->ptrROMData); }
        free(ptrROMData_a);
    }
}

static void secureDelete(const char* strPath_a)
{
    FILE* ptrFile = NULL;
    long lngSize = 0;
    uint8_t byVal = 0;
    long intI = 0;

    ptrFile = fopen(strPath_a, "r+b");
    if (ptrFile)
    {
        fseek(ptrFile, 0, SEEK_END);
        lngSize = ftell(ptrFile);
        fseek(ptrFile, 0, SEEK_SET);
        byVal = 0xFF;
        for (intI = 0; intI < lngSize; intI++) { fwrite(&byVal, 1, 1, ptrFile); }
        fseek(ptrFile, 0, SEEK_SET);
        byVal = 0x00;
        for (intI = 0; intI < lngSize; intI++) { fwrite(&byVal, 1, 1, ptrFile); }
        fclose(ptrFile);
    }
    remove(strPath_a);
}

// Single-ROM decode pass
static bool decodeSingle(const ROMData* ptrROMData_a, const char* strInputFile_a, const char* strOutputFile_a)
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
        fseek(ptrInput, 0, SEEK_END);
        intInputSize = ftell(ptrInput);
        fseek(ptrInput, 0, SEEK_SET);
        intSlots = intInputSize / 2;

        if (intSlots >= 0)
        {
            ptrOutput = fopen(strOutputFile_a, "wb");
            if (ptrOutput)
            {
                for (intI = 0; intI < intSlots; intI++)
                {
                    if (fread(arrBuf, 2, 1, ptrInput) != 1) { break; }
                    uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);
                    if (intAddr < ptrROMData_a->lngROMSize)
                    {
                        if (fputc(ptrROMData_a->ptrROMData[intAddr], ptrOutput) == EOF) { break; }
                    }
                }
                if (intI == intSlots) { blnSuccess = true; }
                fclose(ptrOutput);
            }
        }
        fclose(ptrInput);
    }
    return blnSuccess;
}

// Tango decode — round-robin across all ROMs per slot
static bool decodeTango(ROMData** arrROMs_a, int intROMCount_a, const char* strInputFile_a, const char* strOutputFile_a)
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
        fseek(ptrInput, 0, SEEK_END);
        intInputSize = ftell(ptrInput);
        fseek(ptrInput, 0, SEEK_SET);
        intSlots = intInputSize / 2;

        if (intSlots >= 0)
        {
            ptrOutput = fopen(strOutputFile_a, "wb");
            if (ptrOutput)
            {
                for (intI = 0; intI < intSlots; intI++)
                {
                    if (fread(arrBuf, 2, 1, ptrInput) != 1) { break; }
                    ROMData* ptrROM = arrROMs_a[intI % intROMCount_a];
                    uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);
                    if (intAddr < ptrROM->lngROMSize)
                    {
                        if (fputc(ptrROM->ptrROMData[intAddr], ptrOutput) == EOF) { break; }
                    }
                }
                if (intI == intSlots) { blnSuccess = true; }
                fclose(ptrOutput);
            }
        }
        fclose(ptrInput);
    }
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    bool blnDecodeOk = false;
    bool blnTango = false;
    int intResult = 1;
    int intROMCount = 0;
    int intI = 0;
    ROMData* arrROMs[3] = {NULL, NULL, NULL};
    const char* strInputFile = NULL;
    const char* strOutputFile = NULL;
    char strTempPath1[4096] = {0};
    char strTempPath2[4096] = {0};

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Decoder v20260601\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (intArgC_a >= 4 && intArgC_a <= 7)
    {
        if (strcmp(strArgv_a[intArgC_a - 1], "-t") == 0) { blnTango = true; intArgC_a--; }

        strOutputFile = strArgv_a[intArgC_a - 1];
        strInputFile  = strArgv_a[intArgC_a - 2];
        intROMCount   = intArgC_a - 3;

        if (intROMCount < 1 || intROMCount > 3) { fprintf(stderr, "Usage: %s <rom1> [rom2] [rom3] <encoded> <output> [-t]\n", strArgv_a[0]); return 1; }
        if (blnTango && intROMCount < 2) { fprintf(stderr, "Error: Tango mode requires at least 2 ROMs\n"); return 1; }

        for (intI = 0; intI < intROMCount; intI++)
        {
            arrROMs[intI] = loadROM(strArgv_a[1 + intI]);
            if (!arrROMs[intI])
            {
                fprintf(stderr, "Failed to load ROM: %s\n", strArgv_a[1 + intI]);
                for (intI = 0; intI < intROMCount; intI++) { if (arrROMs[intI]) { unloadROM(arrROMs[intI]); } }
                return 1;
            }
        }

        if (blnTango)
        {
            blnDecodeOk = decodeTango(arrROMs, intROMCount, strInputFile, strOutputFile);
        }
        else if (intROMCount == 1)
        {
            blnDecodeOk = decodeSingle(arrROMs[0], strInputFile, strOutputFile);
        }
        else if (intROMCount == 2)
        {
            // Decode in reverse order
            snprintf(strTempPath1, sizeof(strTempPath1), "%s.tmp", strOutputFile);
            blnDecodeOk = decodeSingle(arrROMs[1], strInputFile, strTempPath1);
            if (blnDecodeOk) { blnDecodeOk = decodeSingle(arrROMs[0], strTempPath1, strOutputFile); }
            secureDelete(strTempPath1);
        }
        else if (intROMCount == 3)
        {
            snprintf(strTempPath1, sizeof(strTempPath1), "%s.tmp",  strOutputFile);
            snprintf(strTempPath2, sizeof(strTempPath2), "%s.tmp2", strOutputFile);
            blnDecodeOk = decodeSingle(arrROMs[2], strInputFile, strTempPath1);
            if (blnDecodeOk) { blnDecodeOk = decodeSingle(arrROMs[1], strTempPath1, strTempPath2); }
            if (blnDecodeOk) { blnDecodeOk = decodeSingle(arrROMs[0], strTempPath2, strOutputFile); }
            secureDelete(strTempPath1);
            secureDelete(strTempPath2);
        }

        for (intI = 0; intI < intROMCount; intI++) { unloadROM(arrROMs[intI]); }

        if (blnDecodeOk) { intResult = 0; }
        else { fprintf(stderr, "Decode failed\n"); }
    }
    else
    {
        fprintf(stderr, "Usage: %s <rom1> [rom2] [rom3] <encoded> <output> [-t]\n", strArgv_a[0]);
    }

    return intResult;
}
