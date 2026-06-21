// Cyborg ZTB Genesis Block Creator v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Creates a genesis block (.ztb, exactly 65536 bytes) from 1-3 entropy source files.
// Matches ZTBChain.Create() exactly.
//
// Usage: ztbcreate <source1> [source2] [source3] <workdir> <new_block_id>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Genesis Block Creator v20260618\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 4 || argc > 6)
    {
        fprintf(stderr, "Usage: %s <source1> [source2] [source3] <workdir> <new_block_id>\n", argv[0]);
        fprintf(stderr, "Example: %s photo.jpg music.mp3 . A1B2C3D4-E5F6-4A7B-8C9D-E0F1A2B3C4D5\n", argv[0]);
        intResult = 1;
    }

    if (intResult == 0)
    {
        // Last arg is block_id, second-to-last is workdir, everything before is sources
        int intSourceCount  = argc - 3;
        const char *strWorkDir_a   = argv[argc - 2];
        const char *strNewBlockID_a = argv[argc - 1];

        // Build output path
        char strOutputPath[FILENAME_MAX];
        snprintf(strOutputPath, FILENAME_MAX, "%s/%s.ztb", strWorkDir_a, strNewBlockID_a);

        // Refuse duplicate (matches C# behaviour)
        FILE *fCheck = fopen(strOutputPath, "rb");
        if (fCheck)
        {
            fclose(fCheck);
            fprintf(stderr, "Error: Output file already exists: %s\n", strOutputPath);
            intResult = 1;
        }

        if (intResult == 0)
        {
            // Load source files
            uint8_t *arrSrc[3]  = {NULL, NULL, NULL};
            long     arrLen[3]  = {0,    0,    0};
            double   arrStep[3] = {0.0,  0.0,  0.0};
            double   arrPos[3]  = {0.0,  0.0,  0.0};
            int      intValid   = 1;

            int intI;
            for (intI = 0; intI < intSourceCount && intValid; intI++)
            {
                printf("Loading source %d: %s\n", intI + 1, argv[intI + 1]);
                FILE *f = fopen(argv[intI + 1], "rb");
                if (!f)
                {
                    fprintf(stderr, "Error: Cannot open source file: %s\n", argv[intI + 1]);
                    intValid = 0;
                }
                else
                {
                    fseek(f, 0, SEEK_END);
                    arrLen[intI] = ftell(f);
                    fseek(f, 0, SEEK_SET);

                    if (arrLen[intI] == 0)
                    {
                        fprintf(stderr, "Error: Source file is empty: %s\n", argv[intI + 1]);
                        intValid = 0;
                    }
                    else
                    {
                        arrSrc[intI] = (uint8_t*)malloc(arrLen[intI]);
                        if (!arrSrc[intI])
                        {
                            fprintf(stderr, "Error: Cannot allocate source buffer\n");
                            intValid = 0;
                        }
                        else if (fread(arrSrc[intI], 1, arrLen[intI], f) != (size_t)arrLen[intI])
                        {
                            fprintf(stderr, "Error: Cannot read source file: %s\n", argv[intI + 1]);
                            intValid = 0;
                        }
                        else
                        {
                            // Step matches C# exactly: step = srcLen / ROM_SIZE (not per-source share)
                            arrStep[intI] = (double)arrLen[intI] / (double)ROM_SIZE;
                            printf("  Loaded %ld bytes\n", arrLen[intI]);
                        }
                    }
                    fclose(f);
                }
            }

            if (!intValid) { intResult = 1; }

            if (intResult == 0)
            {
                // Build genesis block (matches C# Create exactly)
                // byte[0] = BLOCK_TYPE_GENESIS
                // bytes[1..ROM_SIZE-1] = XOR blend of sources
                uint8_t arrGenBlock[ROM_SIZE];
                memset(arrGenBlock, 0, ROM_SIZE);
                arrGenBlock[0] = BLOCK_TYPE_GENESIS;

                int intOutI = 1;
                while (intOutI < ROM_SIZE)
                {
                    uint8_t byVal = 0;
                    int intJ      = 0;
                    while (intJ < intSourceCount)
                    {
                        long lngPos = (long)arrPos[intJ];
                        if (lngPos >= arrLen[intJ]) { lngPos = arrLen[intJ] - 1; }
                        byVal       ^= arrSrc[intJ][lngPos];
                        arrPos[intJ] += arrStep[intJ];
                        intJ++;
                    }
                    arrGenBlock[intOutI] = byVal;
                    intOutI++;
                }

                // Write via tmp then rename (matches C#)
                char strTmp[FILENAME_MAX + 4];
                snprintf(strTmp, FILENAME_MAX + 4, "%s.tmp", strOutputPath);

                FILE *fOut = fopen(strTmp, "wb");
                if (!fOut)
                {
                    fprintf(stderr, "Error: Cannot create output file: %s\n", strTmp);
                    intResult = 1;
                }
                else
                {
                    if (fwrite(arrGenBlock, 1, ROM_SIZE, fOut) != ROM_SIZE)
                    {
                        fprintf(stderr, "Error: Write failed\n");
                        intResult = 1;
                    }
                    fclose(fOut);

                    if (intResult == 0)
                    {
                        // On Windows, rename() fails if destination exists (unlike POSIX).
                        // Destination should not exist here (checked above), but remove
                        // defensively in case of a stale .tmp/.ztb from an interrupted run.
                        remove(strOutputPath);
                        if (rename(strTmp, strOutputPath) != 0)
                        {
                            fprintf(stderr, "Error: Cannot rename tmp to output\n");
                            intResult = 1;
                        }
                    }
                }

                if (intResult == 0)
                {
                    printf("\n+ Genesis block created: %s\n", strOutputPath);
                    printf("  Block ID: %s\n", strNewBlockID_a);
                    printf("  Size:     %d bytes\n", ROM_SIZE);
                }
            }

            for (intI = 0; intI < 3; intI++)
            {
                if (arrSrc[intI]) { free(arrSrc[intI]); }
            }
        }
    }

    return intResult;
}