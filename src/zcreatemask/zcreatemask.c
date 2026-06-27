// Cyborg ZOSCII v20260628 - Mask ROM Generator
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version
// Creates a 64KB mask ROM (all zeros except mapped bytes) such that
// decoding any asset file against it reveals the given message.
// Zero bytes in the ROM mean "don't care" - zunmask skips them on output.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

#define ZOSCII_ROM_SIZE 65536

int main(int intArgC_a, char* strArgV_a[])
{
    FILE*    ptrAsset     = NULL;
    FILE*    ptrOutput    = NULL;
    uint8_t* ptrAssetData = NULL;
    uint8_t* ptrROM       = NULL;
    bool*    ptrSet       = NULL;
    long     lngAssetSize = 0;
    long     lngPairCount = 0;
    int      intMsgLen    = 0;
    int      intMapped    = 0;
    int      intConflicts = 0;
    int      intI         = 0;
    char     strOutputName[4096] = {0};
    int      intResult    = 1;

#ifdef _WIN32
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Mask ROM Generator v20260628\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");
    printf("=============================================\n\n");

    if (intArgC_a != 3)
    {
        fprintf(stderr, "Usage: %s <assetfile> <message>\n", strArgV_a[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "  assetfile  - Any file (JPEG, PDF, etc.) used as address source\n");
        fprintf(stderr, "  message    - Text to embed (quote if it contains spaces)\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Output: <assetfile>.mask  - 64KB ROM, zero except at mapped addresses\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s photo.jpg \"Ikari Warriors\"\n", strArgV_a[0]);
        fprintf(stderr, "  zunmask photo.jpg.mask photo.jpg decoded.txt\n");
        return 1;
    }

    intMsgLen = (int)strlen(strArgV_a[2]);
    if (intMsgLen == 0)
    {
        fprintf(stderr, "Error: Message is empty\n");
        return 1;
    }

    // Build output filename
    snprintf(strOutputName, sizeof(strOutputName), "%s.mask", strArgV_a[1]);

    // Load asset file
    ptrAsset = fopen(strArgV_a[1], "rb");
    if (!ptrAsset)
    {
        perror("Failed to open asset file");
        return 1;
    }

    fseek(ptrAsset, 0, SEEK_END);
    lngAssetSize = ftell(ptrAsset);
    fseek(ptrAsset, 0, SEEK_SET);

    if (lngAssetSize < 2)
    {
        fprintf(stderr, "Error: Asset file too small to contain any addresses\n");
        fclose(ptrAsset);
        return 1;
    }

    lngPairCount = lngAssetSize / 2;

    printf("Asset file:  %s (%ld bytes, %ld address pairs)\n", strArgV_a[1], lngAssetSize, lngPairCount);
    printf("Message:     \"%s\" (%d bytes)\n", strArgV_a[2], intMsgLen);
    printf("Output ROM:  %s\n\n", strOutputName);

    if (lngPairCount < (long)intMsgLen)
    {
        fprintf(stderr, "Error: Asset only provides %ld address pairs but message needs %d\n",
                lngPairCount, intMsgLen);
        fclose(ptrAsset);
        return 1;
    }

    ptrAssetData = (uint8_t*)malloc(lngAssetSize);
    if (!ptrAssetData)
    {
        fprintf(stderr, "Error: Failed to allocate memory for asset\n");
        fclose(ptrAsset);
        return 1;
    }

    if (fread(ptrAssetData, 1, lngAssetSize, ptrAsset) != (size_t)lngAssetSize)
    {
        fprintf(stderr, "Error: Failed to read asset file\n");
        free(ptrAssetData);
        fclose(ptrAsset);
        return 1;
    }
    fclose(ptrAsset);

    // Allocate 64KB ROM (all zeros = don't care)
    ptrROM = (uint8_t*)calloc(ZOSCII_ROM_SIZE, 1);
    if (!ptrROM)
    {
        fprintf(stderr, "Error: Failed to allocate ROM buffer\n");
        free(ptrAssetData);
        return 1;
    }

    // Track which ROM addresses have been set
    ptrSet = (bool*)calloc(ZOSCII_ROM_SIZE, sizeof(bool));
    if (!ptrSet)
    {
        fprintf(stderr, "Error: Failed to allocate mapping table\n");
        free(ptrROM);
        free(ptrAssetData);
        return 1;
    }

    // Walk asset pairs with a forward cursor.
    // For each message byte, advance until we find an address that is free
    // (not already set to a different value).  Asset pairs that land on an
    // already-used address are simply consumed and skipped; they leave a
    // zero in the ROM so zunmask ignores them on decode.
    {
        long lngCursor    = 0;   // current position in asset (pair index)
        int  intUnmapped  = 0;   // message bytes we ran out of asset to map

        for (intI = 0; intI < intMsgLen; intI++)
        {
            uint8_t  byTarget = (uint8_t)strArgV_a[2][intI];
            bool     blnDone  = false;

            // Zero is our "don't care" sentinel
            if (byTarget == 0x00)
            {
                printf("Warning: Message byte %d is 0x00 - cannot embed (zero = don't care)\n", intI);
                intUnmapped++;
                continue;
            }

            // Advance cursor until we land on a free address
            while (lngCursor < lngPairCount)
            {
                uint16_t intAddr = (uint16_t)ptrAssetData[lngCursor * 2] |
                                   ((uint16_t)ptrAssetData[lngCursor * 2 + 1] << 8);
                lngCursor++;

                if (!ptrSet[intAddr])
                {
                    // Free address - claim it
                    ptrROM[intAddr]  = byTarget;
                    ptrSet[intAddr]  = true;
                    intMapped++;
                    blnDone = true;
                    break;
                }
                // Already set (to same or different value) - skip this pair,
                // it stays zero in the ROM and zunmask will ignore it
            }

            if (!blnDone)
            {
                // Ran out of asset pairs before mapping this byte
                intUnmapped++;
                if (intUnmapped <= 5)
                {
                    printf("Warning: Exhausted asset pairs at message byte %d ('%c') - unmapped\n",
                           intI, (byTarget >= 32 && byTarget <= 126) ? byTarget : '?');
                }
            }
        }

        intConflicts = intUnmapped;   // repurpose for summary
    }

    // Write mask ROM
    ptrOutput = fopen(strOutputName, "wb");
    if (!ptrOutput)
    {
        perror("Failed to create output ROM file");
        free(ptrSet);
        free(ptrROM);
        free(ptrAssetData);
        return 1;
    }

    fwrite(ptrROM, 1, ZOSCII_ROM_SIZE, ptrOutput);
    fclose(ptrOutput);

    free(ptrSet);
    free(ptrROM);
    free(ptrAssetData);

    printf("Mask ROM Created\n");
    printf("================\n");
    printf("Addresses mapped:  %d / %d\n", intMapped, intMsgLen);
    printf("Unmapped bytes:    %d\n", intConflicts);
    printf("Non-zero bytes:    %d (%.4f%% of 64KB)\n",
           intMapped,
           (double)intMapped / (double)ZOSCII_ROM_SIZE * 100.0);

    if (intConflicts > 0)
    {
        printf("\nNote: %d byte(s) could not be mapped (asset exhausted).\n", intConflicts);
        printf("      Decoded output will have gaps at those positions.\n");
    }

    printf("\nTo decode:\n");
    printf("  zunmask \"%s\" \"%s\" decoded.txt\n", strOutputName, strArgV_a[1]);

    intResult = 0;
    return intResult;
}