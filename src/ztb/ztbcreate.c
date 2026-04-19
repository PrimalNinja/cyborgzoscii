// Cyborg ZTB Genesis ROM Creator v20260420
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Creates a 64KB Genesis ROM from 1-3 entropy source files (e.g. JPEG, MP3).
// Usage: ztbcreate <input1> [input2] [input3] <output_genesis_rom>

#include "ztbcommon.c"

// --- Load a file into a buffer ---
static uint8_t* load_source_file(const char *strFilename_a, long *lngSize_a)
{
    uint8_t *ptrData = NULL;
    FILE *f = NULL;

    *lngSize_a = 0;
    f = fopen(strFilename_a, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        *lngSize_a = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (*lngSize_a > 0)
        {
            ptrData = (uint8_t*)malloc(*lngSize_a);
            if (ptrData)
            {
                if (fread(ptrData, 1, *lngSize_a, f) != (size_t)*lngSize_a)
                {
                    free(ptrData);
                    ptrData = NULL;
                    *lngSize_a = 0;
                }
            }
            else
            {
                *lngSize_a = 0;
            }
        }
        fclose(f);
    }

    return ptrData;
}

// --- Analyze ROM entropy ---
static void analyze_rom_entropy(const uint8_t *byRomData_a)
{
    uint32_t arrByteCounts[256] = {0};
    size_t intI;

    for (intI = 0; intI < ROM_SIZE; intI++)
    {
        arrByteCounts[byRomData_a[intI]]++;
    }

    uint32_t intMinCount = ROM_SIZE;
    uint32_t intMaxCount = 0;
    uint32_t intMissingBytes = 0;
    int intJ;

    for (intJ = 0; intJ < 256; intJ++)
    {
        if (arrByteCounts[intJ] == 0)
        {
            intMissingBytes++;
        }
        else
        {
            if (arrByteCounts[intJ] < intMinCount) { intMinCount = arrByteCounts[intJ]; }
            if (arrByteCounts[intJ] > intMaxCount) { intMaxCount = arrByteCounts[intJ]; }
        }
    }

    double dblExpected = ROM_SIZE / 256.0;

    printf("\n--- ROM Entropy Analysis ---\n");
    printf("Total bytes:           %d\n", ROM_SIZE);
    printf("Expected per value:    %.1f\n", dblExpected);
    printf("Minimum occurrences:   %u\n", intMinCount);
    printf("Maximum occurrences:   %u\n", intMaxCount);
    printf("Missing byte values:   %u\n", intMissingBytes);

    if (intMissingBytes > 0)
    {
        printf("\n!! WARNING: %u byte values missing!\n", intMissingBytes);
        printf("   This will cause encoding failures.\n");
        printf("   Try different source files.\n");
    }
    else
    {
        printf("\n+ All 256 byte values present.\n");
    }

    double dblDeviation = ((double)intMaxCount - intMinCount) / dblExpected * 100.0;
    printf("Distribution variance: %.1f%%\n", dblDeviation);

    if (dblDeviation < 50.0)
    {
        printf("+ Excellent entropy.\n");
    }
    else if (dblDeviation < 100.0)
    {
        printf("+ Good entropy.\n");
    }
    else
    {
        printf("!! Fair entropy (consider different source files).\n");
    }
    printf("----------------------------\n");
}

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *arrInputData[3] = {NULL, NULL, NULL};
    long arrInputSize[3] = {0, 0, 0};
    long arrShare[3] = {0, 0, 0};
    double dblPos[3] = {0.0, 0.0, 0.0};
    double dblStep[3] = {0.0, 0.0, 0.0};
    uint8_t *byRomData = NULL;
    int intInputCount = 0;
    int intI = 0;
    const char *strOutputFilename = NULL;

    printf("ZTB Genesis ROM Creator v20260420\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 3 || argc > 5)
    {
        fprintf(stderr, "Usage: %s <input1> [input2] [input3] <output_genesis_rom>\n", argv[0]);
        fprintf(stderr, "Example: %s photo.jpg music.mp3 genesis.rom\n", argv[0]);
        fprintf(stderr, "\nCreates a 64KB high-entropy Genesis ROM from 1-3 source files.\n");
        fprintf(stderr, "Recommended sources: JPEG photos, MP3s, or any real-world binary files.\n");
        fprintf(stderr, "The sources are never stored - only their entropy is sampled.\n");
        intResult = 1;
    }

    if (intResult == 0)
    {
        // Last arg is always output, everything before is input
        intInputCount = argc - 2;
        strOutputFilename = argv[argc - 1];

        // Check output doesn't already exist
        FILE *fCheck = fopen(strOutputFilename, "rb");
        if (fCheck)
        {
            fclose(fCheck);
            fprintf(stderr, "Error: Output file already exists: %s\n", strOutputFilename);
            intResult = 1;
        }
    }

    // Load all input files
    if (intResult == 0)
    {
        for (intI = 0; intI < intInputCount && intResult == 0; intI++)
        {
            printf("Loading source %d: %s\n", intI + 1, argv[intI + 1]);
            arrInputData[intI] = load_source_file(argv[intI + 1], &arrInputSize[intI]);
            if (!arrInputData[intI] || arrInputSize[intI] == 0)
            {
                fprintf(stderr, "Error: Cannot load source file: %s\n", argv[intI + 1]);
                intResult = 1;
            }
            else
            {
                printf("  Loaded %ld bytes\n", arrInputSize[intI]);
            }
        }
    }

    // Build ROM by interleaved sampling (same logic as ucreate)
    if (intResult == 0)
    {
        byRomData = (uint8_t*)malloc(ROM_SIZE);
        if (!byRomData)
        {
            fprintf(stderr, "Error: Cannot allocate ROM buffer\n");
            intResult = 1;
        }
    }

    if (intResult == 0)
    {
        // Calculate each file's share of the ROM and step size
        long intBytesPerFile = ROM_SIZE / intInputCount;
        long intRemainder = ROM_SIZE % intInputCount;

        for (intI = 0; intI < intInputCount; intI++)
        {
            arrShare[intI] = intBytesPerFile;
            if (intI < intRemainder) { arrShare[intI]++; }
            dblStep[intI] = (double)arrInputSize[intI] / (double)arrShare[intI];
            dblPos[intI] = 0.0;
        }

        // Interleave samples from each source file
        long intOutPos = 0;
        int intFileIdx = 0;
        while (intOutPos < ROM_SIZE)
        {
            for (intFileIdx = 0; intFileIdx < intInputCount && intOutPos < ROM_SIZE; intFileIdx++)
            {
                if (arrShare[intFileIdx] > 0)
                {
                    long intSamplePos = (long)dblPos[intFileIdx];
                    if (intSamplePos >= arrInputSize[intFileIdx])
                    {
                        intSamplePos = arrInputSize[intFileIdx] - 1;
                    }
                    byRomData[intOutPos] = arrInputData[intFileIdx][intSamplePos];
                    dblPos[intFileIdx] += dblStep[intFileIdx];
                    arrShare[intFileIdx]--;
                    intOutPos++;
                }
            }
        }

        printf("\nGenesis ROM built from %d source file(s)\n", intInputCount);
        analyze_rom_entropy(byRomData);

        if (intResult == 0)
        {
            printf("\nWriting Genesis ROM to: %s\n", strOutputFilename);
            FILE *f = fopen(strOutputFilename, "wb");
            if (!f)
            {
                fprintf(stderr, "Error: Cannot create output file\n");
                intResult = 1;
            }
            else
            {
                if (fwrite(byRomData, 1, ROM_SIZE, f) != ROM_SIZE)
                {
                    fprintf(stderr, "Error: Write failed\n");
                    intResult = 1;
                }
                else
                {
                    printf("+ Genesis ROM created successfully\n");
                    printf("\n--- Sources used ---\n");
                    for (intI = 0; intI < intInputCount; intI++)
                    {
                        printf("  %s (%ld bytes)\n", argv[intI + 1], arrInputSize[intI]);
                    }
                    printf("\n--- IMPORTANT SECURITY NOTES ---\n");
                    printf("1. Keep this Genesis ROM file secure and backed up.\n");
                    printf("2. All blocks depend on this file.\n");
                    printf("3. Loss or corruption breaks verification.\n");
                    printf("4. The source files used are NOT needed again.\n");
                    printf("5. If any byte values are missing, try different source files.\n");
                    printf("--------------------------------\n");
                }
                fclose(f);
            }
        }
    }

    // Cleanup
    for (intI = 0; intI < 3; intI++)
    {
        if (arrInputData[intI]) { free(arrInputData[intI]); }
    }
    if (byRomData) { free(byRomData); }

    return intResult;
}