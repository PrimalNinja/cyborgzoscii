// Cyborg ZOSCII v20260628 - Mask ROM Decoder
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version
// Decodes an encoded file against a mask ROM, skipping any address whose
// ROM value is 0x00 (the "don't care" sentinel used by zcreatemask).

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
    long     lngROMSize;
} ROMData;

static ROMData* loadROM(const char* strFilename_a)
{
    ROMData* ptrROMData = NULL;
    FILE*    ptrROMFile = NULL;

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

static bool decodeMask(const ROMData* ptrROM_a,
                       const char*    strAssetFile_a,
                       const char*    strOutputFile_a)
{
    uint8_t  arrBuf[2];
    bool     blnSuccess   = false;
    long     intI         = 0;
    long     lngAssetSize = 0;
    long     lngPairCount = 0;
    long     lngWritten   = 0;
    long     lngSkipped   = 0;
    bool     arrUsed[65536] = {false};
    FILE*    ptrAsset     = NULL;
    FILE*    ptrOutput    = NULL;

    ptrAsset = fopen(strAssetFile_a, "rb");
    if (!ptrAsset)
    {
        perror("Failed to open asset file");
        return false;
    }

    fseek(ptrAsset, 0, SEEK_END);
    lngAssetSize = ftell(ptrAsset);
    fseek(ptrAsset, 0, SEEK_SET);
    lngPairCount = lngAssetSize / 2;

    ptrOutput = fopen(strOutputFile_a, "wb");
    if (!ptrOutput)
    {
        perror("Failed to open output file");
        fclose(ptrAsset);
        return false;
    }

    for (intI = 0; intI < lngPairCount; intI++)
    {
        if (fread(arrBuf, 2, 1, ptrAsset) != 1) { break; }

        uint16_t intAddr = (uint16_t)arrBuf[0] | ((uint16_t)arrBuf[1] << 8);

        if (intAddr >= ptrROM_a->lngROMSize)
        {
            lngSkipped++;
            continue;
        }

        uint8_t byVal = ptrROM_a->ptrROMData[intAddr];

        // Zero = don't care - skip silently
        if (byVal == 0x00)
        {
            lngSkipped++;
            continue;
        }

        // Already emitted this address once - skip all subsequent hits
        if (arrUsed[intAddr])
        {
            lngSkipped++;
            continue;
        }

        arrUsed[intAddr] = true;
        if (fputc(byVal, ptrOutput) == EOF) { break; }
        lngWritten++;
    }

    if (intI == lngPairCount) { blnSuccess = true; }

    fclose(ptrOutput);
    fclose(ptrAsset);

    printf("Pairs processed:  %ld\n", lngPairCount);
    printf("Bytes written:    %ld\n", lngWritten);
    printf("Zero slots skipped: %ld\n", lngSkipped);

    return blnSuccess;
}

int main(int intArgC_a, char* strArgV_a[])
{
    bool     blnDecodeOk = false;
    int      intResult   = 1;
    ROMData* ptrROM      = NULL;

#ifdef _WIN32
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Mask Decoder v20260628\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");
    printf("=============================================\n\n");

    if (intArgC_a != 4)
    {
        fprintf(stderr, "Usage: %s <maskrom> <assetfile> <output>\n", strArgV_a[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "  maskrom    - 64KB mask ROM created by zcreatemask\n");
        fprintf(stderr, "  assetfile  - The same asset used with zcreatemask\n");
        fprintf(stderr, "  output     - File to write decoded message to\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Zero bytes in the mask ROM are treated as 'don't care' and skipped.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  zunmask photo.jpg.mask photo.jpg decoded.txt\n");
        return 1;
    }

    printf("Mask ROM:    %s\n", strArgV_a[1]);
    printf("Asset file:  %s\n", strArgV_a[2]);
    printf("Output:      %s\n\n", strArgV_a[3]);

    ptrROM = loadROM(strArgV_a[1]);
    if (!ptrROM)
    {
        fprintf(stderr, "Failed to load mask ROM: %s\n", strArgV_a[1]);
        return 1;
    }

    blnDecodeOk = decodeMask(ptrROM, strArgV_a[2], strArgV_a[3]);

    unloadROM(ptrROM);

    if (blnDecodeOk)
    {
        printf("\nSUCCESS: Message decoded to \"%s\"\n", strArgV_a[3]);
        intResult = 0;
    }
    else
    {
        fprintf(stderr, "\nERROR: Decode failed\n");
    }

    return intResult;
}