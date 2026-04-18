// Cyborg ZTB Checkpoint Creator v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Creates a new Genesis ROM from the chains current rolling ROM state.
// This cleanly splits the chain:
// Usage: ztbcheckpoint <genesis_rom> <chain_id> <output_checkpoint_rom>

#include "ztbcommon.c"

// --- Analyze ROM entropy (same logic as ztbcreate) ---
int analyze_checkpoint_rom(const uint8_t *byRomData_a)
{
    uint32_t arrByteCounts[256] = {0};
    uint32_t intI;
    size_t intIdx;

    for (intIdx = 0; intIdx < ROM_SIZE; intIdx++)
    {
        arrByteCounts[byRomData_a[intIdx]]++;
    }

    uint32_t intMinCount = ROM_SIZE;
    uint32_t intMaxCount = 0;
    uint32_t intMissingBytes = 0;

    for (intI = 0; intI < 256; intI++)
    {
        if (arrByteCounts[intI] == 0)
        {
            intMissingBytes++;
        }
        else
        {
            if (arrByteCounts[intI] < intMinCount)
            {
                intMinCount = arrByteCounts[intI];
            }
            if (arrByteCounts[intI] > intMaxCount)
            {
                intMaxCount = arrByteCounts[intI];
            }
        }
    }

    double dblExpected = ROM_SIZE / 256.0;

    printf("\n--- Checkpoint ROM Entropy Analysis ---\n");
    printf("Total bytes:           %d\n", ROM_SIZE);
    printf("Expected per value:    %.1f\n", dblExpected);
    printf("Minimum occurrences:   %u\n", intMinCount);
    printf("Maximum occurrences:   %u\n", intMaxCount);
    printf("Missing byte values:   %u\n", intMissingBytes);

    if (intMissingBytes > 0)
    {
        printf("\n!! WARNING: %u byte values missing!\n", intMissingBytes);
        printf("   This ROM cannot be used for encoding.\n");
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
        printf("!! Fair entropy.\n");
    }
    printf("---------------------------------------\n");

    return (intMissingBytes == 0) ? 1 : 0;
}

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *byRollingRom = NULL;
    BlockInfo *arrChainHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    printf("ZTB Checkpoint Creator v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> <output_checkpoint_rom>\n", argv[0]);
        fprintf(stderr, "Example: %s genesis.rom MyTrunk checkpoint.rom\n", argv[0]);
        fprintf(stderr, "\nCreates a new Genesis ROM from the chain's current state.\n");
        fprintf(stderr, "The rolling ROM at the current chain tip becomes the new genesis.\n");
        fprintf(stderr, "\nAfter checkpoint:\n");
        fprintf(stderr, "  ARCHIVE: original genesis.rom + blocks 1..N (+ branches)\n");
        fprintf(stderr, "  ACTIVE:  checkpoint.rom + blocks N+1 onwards\n");
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strChainId_a = argv[2];
        const char *strOutputFile_a = argv[3];

        // --- 1. Scan existing chain ---
        arrChainHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
        int intChainCount = 0;

        if (!arrChainHistory)
        {
            fprintf(stderr, "Error: Cannot allocate chain history\n");
            intResult = 1;
        }

        if (intResult == 0)
        {
            intChainCount = scan_chain_blocks(strChainId_a, arrChainHistory, MAX_BLOCKS_TO_SCAN);

            if (intChainCount == 0)
            {
                fprintf(stderr, "Error: Chain '%s' not found or empty.\n", strChainId_a);
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Found chain '%s' with %d blocks\n", strChainId_a, intChainCount);

            // --- 2. Determine if this is a branch (need trunk history for rolling ROM) ---
            uint8_t byIsBranch = 0;
            char strTrunkId[GUID_LEN];
            int intTrunkCount = 0;
            strcpy(strTrunkId, NULL_GUID);

            byIsBranch = detect_branch_status(strGenesisRomFile_a, arrChainHistory, intChainCount,
                                               strChainId_a, strTrunkId) ? 1 : 0;

            // Load trunk history if this is a branch
            if (byIsBranch && strcmp(strTrunkId, NULL_GUID) != 0)
            {
                arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
                if (arrTrunkHistory)
                {
                    intTrunkCount = scan_chain_blocks(strTrunkId, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);
                    printf("Loaded trunk '%s' with %d blocks\n", strTrunkId, intTrunkCount);
                }
            }

            // --- 3. Build rolling ROM for the NEXT block position ---
            //     This is the rolling ROM that block N+1 would use.
            //     It becomes the new genesis ROM.
            int intNextIndex = intChainCount + 1;
            byRollingRom = malloc(ROM_SIZE);

            if (!byRollingRom)
            {
                fprintf(stderr, "Error: Cannot allocate rolling ROM\n");
                intResult = 1;
            }

            if (intResult == 0)
            {
                if (!build_rolling_rom(strGenesisRomFile_a, arrChainHistory, intChainCount,
                                       arrTrunkHistory, intTrunkCount, intNextIndex, byRollingRom))
                {
                    fprintf(stderr, "Error: Failed to build rolling ROM\n");
                    intResult = 1;
                }
            }

            if (intResult == 0)
            {
                printf("Rolling ROM built from %d blocks (target index %d)\n", intChainCount, intNextIndex);

                // --- 4. Entropy check ---
                if (!analyze_checkpoint_rom(byRollingRom))
                {
                    fprintf(stderr, "\nError: Checkpoint ROM has missing byte values.\n");
                    fprintf(stderr, "This chain state cannot produce a valid checkpoint ROM.\n");
                    intResult = 1;
                }
            }

            if (intResult == 0)
            {
                // --- 5. Write checkpoint ROM ---
                printf("\nWriting checkpoint ROM to: %s\n", strOutputFile_a);

                FILE *f_out = fopen(strOutputFile_a, "wb");
                if (!f_out)
                {
                    fprintf(stderr, "Error: Cannot create file '%s'\n", strOutputFile_a);
                    intResult = 1;
                }

                if (intResult == 0)
                {
                    if (fwrite(byRollingRom, 1, ROM_SIZE, f_out) != ROM_SIZE)
                    {
                        fprintf(stderr, "Error: Failed to write checkpoint ROM\n");
                        intResult = 1;
                    }
                    fclose(f_out);
                }
            }

            if (intResult == 0)
            {
                printf("+ Checkpoint ROM created successfully\n");

                printf("\n=== CHECKPOINT COMPLETE ===\n");
                printf("Chain:     %s\n", strChainId_a);
                printf("Blocks:    %d\n", intChainCount);
                printf("New ROM:   %s\n", strOutputFile_a);
                printf("\n--- NEXT STEPS ---\n");
                printf("1. ARCHIVE: Copy original genesis ROM + all %d blocks\n", intChainCount);
                printf("   (+ any branches) to archive/cold storage.\n");
                printf("2. ACTIVE:  Use '%s' as the genesis ROM for new blocks:\n", strOutputFile_a);
                printf("   ztbaddblock %s %s -t \"First post-checkpoint block\"\n", strOutputFile_a, strChainId_a);
                printf("3. The archive is fully self-contained and independently verifiable\n");
                printf("   with the original genesis ROM.\n");
                printf("4. The active chain is fully self-contained and independently verifiable\n");
                printf("   with the checkpoint ROM.\n");
                printf("5. Branches below this checkpoint belong to the archive.\n");
                printf("===========================\n");
            }
        }
    }

    // --- Cleanup ---
    if (byRollingRom)
    {
        free(byRollingRom);
    }
    if (arrChainHistory)
    {
        free(arrChainHistory);
    }
    if (arrTrunkHistory)
    {
        free(arrTrunkHistory);
    }

    return intResult;
}