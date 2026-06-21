// Cyborg ZTB Truncate v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Writes a truncation block below the nominated checkpoint, severing the chain.
// Matches ZTBChain.Truncate exactly.
//
// Logic:
//   1. Load checkpoint block -> its prev_block_id is block10ID
//   2. Load block10 (existence check only)
//   3. Build the rolling ROM as of block10 (the ROM block11/checkpoint would have used)
//   4. Write raw header: block_type=Truncation, is_branch=false,
//      trunk_id=NULL_GUID, block_id=block10ID, prev_block_id=NULL_GUID
//   5. Output = raw header (111 bytes) + raw rolling ROM (65536 bytes), NOT ZOSCII encoded
//   6. This OVERWRITES block10's own file (<block10ID>.ztb), severing the chain there.
//
// Usage: ztbtruncate <workdir> <checkpoint_block_id>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Truncate v20260618\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <workdir> <checkpoint_block_id>\n", argv[0]);
        fprintf(stderr, "\nWrites a truncation block at block10 (the checkpoint's prev_block_id),\n");
        fprintf(stderr, "overwriting <block10ID>.ztb with a raw rolling ROM payload.\n");
        fprintf(stderr, "This severs the chain there for archival/checkpoint splitting.\n");
        intResult = 1;
    }

    uint8_t *byCheckpoint  = NULL;
    uint8_t *byBlock10     = NULL;
    uint8_t *byRollingRom  = NULL;
    uint8_t *byFinal       = NULL;

    if (intResult == 0)
    {
        const char *strWorkDir_a         = argv[1];
        const char *strCheckpointID_a    = argv[2];

        // --- 1. Load checkpoint block, read its prev_block_id -> block10ID ---
        int intCpLen = 0;
        byCheckpoint = load_block(strWorkDir_a, strCheckpointID_a, &intCpLen);
        if (!byCheckpoint || intCpLen < HEADER_RAW_SIZE)
        {
            fprintf(stderr, "Error: Cannot load checkpoint block '%s'\n", strCheckpointID_a);
            intResult = 1;
        }

        char strBlock10ID[GUID_LEN];

        if (intResult == 0)
        {
            read_fixed_string(byCheckpoint, RAW_OFF_PREV_ID, 36, strBlock10ID);

            // --- 2. Load block10 (existence check, matches C# Truncate) ---
            int intB10Len = 0;
            byBlock10 = load_block(strWorkDir_a, strBlock10ID, &intB10Len);
            if (!byBlock10 || intB10Len < HEADER_RAW_SIZE)
            {
                fprintf(stderr, "Error: Cannot load block10 '%s' (checkpoint's prev block)\n",
                        strBlock10ID);
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Checkpoint:  %s\n", strCheckpointID_a);
            printf("Block10:     %s (will be overwritten with truncation marker)\n", strBlock10ID);

            // --- 3. Build rolling ROM as of block10 ---
            byRollingRom = build_rolling_rom(strWorkDir_a, strBlock10ID);
            if (!byRollingRom)
            {
                fprintf(stderr, "Error: Cannot build rolling ROM as of block10\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            // --- 4. Build raw header ---
            // block_type=Truncation, is_branch=false, trunk_id=NULL_GUID,
            // block_id=strBlock10ID, prev_block_id=NULL_GUID
            uint8_t arrRawHeader[HEADER_RAW_SIZE];
            memset(arrRawHeader, 0, HEADER_RAW_SIZE);
            arrRawHeader[RAW_OFF_BLOCK_TYPE] = BLOCK_TYPE_TRUNCATION;
            arrRawHeader[RAW_OFF_BLOCK_VER]  = BLOCK_VERSION;
            arrRawHeader[RAW_OFF_IS_BRANCH]  = 0;
            write_fixed_string(arrRawHeader, RAW_OFF_TRUNK_ID, 36, NULL_GUID);
            write_fixed_string(arrRawHeader, RAW_OFF_BLOCK_ID, 36, strBlock10ID);
            write_fixed_string(arrRawHeader, RAW_OFF_PREV_ID,  36, NULL_GUID);

            // --- 5. Assemble final output: raw header + raw rolling ROM (NOT ZOSCII encoded) ---
            int intFinalLen = HEADER_RAW_SIZE + ROM_SIZE;
            byFinal         = (uint8_t*)malloc(intFinalLen);
            if (!byFinal)
            {
                fprintf(stderr, "Error: Cannot allocate output buffer\n");
                intResult = 1;
            }

            if (intResult == 0)
            {
                memcpy(byFinal, arrRawHeader, HEADER_RAW_SIZE);
                memcpy(byFinal + HEADER_RAW_SIZE, byRollingRom, ROM_SIZE);

                // --- 6. Write via tmp then rename, overwriting <block10ID>.ztb ---
                char strOutPath[FILENAME_MAX];
                char strTmpPath[FILENAME_MAX + 4];
                snprintf(strOutPath, FILENAME_MAX, "%s/%s.ztb", strWorkDir_a, strBlock10ID);
                snprintf(strTmpPath, FILENAME_MAX + 4, "%s.tmp", strOutPath);

                FILE *fOut = fopen(strTmpPath, "wb");
                if (!fOut)
                {
                    fprintf(stderr, "Error: Cannot create: %s\n", strTmpPath);
                    intResult = 1;
                }
                else
                {
                    if (fwrite(byFinal, 1, intFinalLen, fOut) != (size_t)intFinalLen)
                    {
                        fprintf(stderr, "Error: Write failed\n");
                        intResult = 1;
                    }
                    fclose(fOut);

                    if (intResult == 0)
                    {
                        // remove() first: Windows rename() fails if destination exists
                        // (this call MUST overwrite block10's existing file)
                        remove(strOutPath);
                        if (rename(strTmpPath, strOutPath) != 0)
                        {
                            fprintf(stderr, "Error: Cannot rename tmp to output\n");
                            intResult = 1;
                        }
                    }
                }

                if (intResult == 0)
                {
                    printf("\n+ Truncation block written: %s\n", strOutPath);
                    printf("  Block ID:    %s\n", strBlock10ID);
                    printf("  Block Type:  Truncation\n");
                    printf("  Prev ID:     %s\n", NULL_GUID);
                    printf("  Payload:     raw rolling ROM (%d bytes, not ZOSCII encoded)\n", ROM_SIZE);
                    printf("\nChain severed at %s. Blocks below this point are now archival.\n",
                           strBlock10ID);
                }
            }
        }
    }

    if (byCheckpoint) { free(byCheckpoint); }
    if (byBlock10)    { free(byBlock10); }
    if (byRollingRom) { free(byRollingRom); }
    if (byFinal)      { free(byFinal); }

    return intResult;
}