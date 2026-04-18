// Cyborg ZTB Block Fetcher v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Usage: ztbfetch <genesis_rom> <chain_id> <block_index>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *byEncodedBlock = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byDecodedBlock = NULL;
    BlockInfo *arrChainHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    printf("ZTB Block Fetcher v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> <block_index>\n", argv[0]);
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strChainId_a = argv[2];
        int intTargetIndex = atoi(argv[3]);

        if (intTargetIndex < 1)
        {
            fprintf(stderr, "Error: Block index must be >= 1\n");
            intResult = 1;
        }

        // --- 1. Scan chain blocks ---
        int intChainCount = 0;

        if (intResult == 0)
        {
            arrChainHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (!arrChainHistory)
            {
                fprintf(stderr, "Error: Cannot allocate chain history\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            intChainCount = scan_chain_blocks(strChainId_a, arrChainHistory, MAX_BLOCKS_TO_SCAN);

            if (intChainCount == 0)
            {
                fprintf(stderr, "Error: Chain '%s' not found\n", strChainId_a);
                intResult = 1;
            }
        }

        // --- 2. Find target block ---
        BlockInfo *objTargetBlock = NULL;

        if (intResult == 0)
        {
            int intI;
            for (intI = 0; intI < intChainCount; intI++)
            {
                if (arrChainHistory[intI].index == intTargetIndex)
                {
                    objTargetBlock = &arrChainHistory[intI];
                }
            }

            if (!objTargetBlock)
            {
                fprintf(stderr, "Error: Block %d not found in chain '%s'\n", intTargetIndex, strChainId_a);
                intResult = 1;
            }
        }

        // --- 3. Load encoded block file ---
        size_t intEncodedLen = 0;
        uint32_t intStoredCrc = 0;

        if (intResult == 0)
        {
            printf("Fetching block %d from chain '%s'\n", intTargetIndex, strChainId_a);
            printf("Filename: %s\n", objTargetBlock->filename);

            FILE *f = fopen(objTargetBlock->filename, "rb");
            if (!f)
            {
                fprintf(stderr, "Error: Cannot open block file '%s'\n", objTargetBlock->filename);
                intResult = 1;
            }
            else
            {
                fseek(f, 0, SEEK_END);
                size_t intFileLen = ftell(f);
                fseek(f, 0, SEEK_SET);

                if (intFileLen <= CRC32_PREFIX_SIZE)
                {
                    fprintf(stderr, "Error: Block file too small\n");
                    intResult = 1;
                }
                else
                {
                    // Read CRC32 prefix
                    uint8_t arrCrcBytes[CRC32_PREFIX_SIZE];
                    if (fread(arrCrcBytes, 1, CRC32_PREFIX_SIZE, f) == CRC32_PREFIX_SIZE)
                    {
                        intStoredCrc = arrCrcBytes[0] | (arrCrcBytes[1] << 8) |
                                       (arrCrcBytes[2] << 16) | (arrCrcBytes[3] << 24);

                        intEncodedLen = intFileLen - CRC32_PREFIX_SIZE;
                        byEncodedBlock = malloc(intEncodedLen);
                        if (byEncodedBlock)
                        {
                            if (fread(byEncodedBlock, 1, intEncodedLen, f) != intEncodedLen)
                            {
                                fprintf(stderr, "Error: Cannot read encoded block\n");
                                intResult = 1;
                            }
                        }
                        else
                        {
                            fprintf(stderr, "Error: Cannot allocate encoded block\n");
                            intResult = 1;
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Error: Cannot read CRC32 prefix\n");
                        intResult = 1;
                    }
                }
                fclose(f);
            }
        }

        if (intResult == 0)
        {
            printf("Encoded block: %zu bytes\n", intEncodedLen);

            // --- Verify CRC32 over encoded data ---
            uint32_t intCalcCrc = calculate_checksum(byEncodedBlock, intEncodedLen);
            printf("Stored CRC32:     0x%08X\n", intStoredCrc);
            printf("Calculated CRC32: 0x%08X\n", intCalcCrc);

            if (intCalcCrc != intStoredCrc)
            {
                fprintf(stderr, "\n!!! INTEGRITY FAILURE !!!\n");
                fprintf(stderr, "CRC32 mismatch - data is corrupt or tampered\n");
                intResult = 1;
            }
            else
            {
                printf("+ CRC32 verified\n");
            }
        }

        // --- 4. Detect if this is a branch ---
        uint8_t byIsBranch = 0;
        char strTrunkId[GUID_LEN];
        int intTrunkCount = 0;
        strcpy(strTrunkId, NULL_GUID);

        if (intResult == 0)
        {
            byIsBranch = detect_branch_status(strGenesisRomFile_a, arrChainHistory, intChainCount,
                                               strChainId_a, strTrunkId) ? 1 : 0;
        }

        // --- 5. Load trunk history if this is a branch ---
        if (intResult == 0 && byIsBranch && strcmp(strTrunkId, NULL_GUID) != 0)
        {
            printf("Loading trunk history: %s\n", strTrunkId);
            arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (arrTrunkHistory)
            {
                intTrunkCount = scan_chain_blocks(strTrunkId, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);
                if (intTrunkCount > 0)
                {
                    printf("Found trunk with %d blocks\n", intTrunkCount);
                }
            }
        }

        // --- 6. Build Rolling ROM ---
        if (intResult == 0)
        {
            byRollingRom = malloc(ROM_SIZE);
            if (!byRollingRom)
            {
                fprintf(stderr, "Error: Cannot allocate rolling ROM\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            if (!build_rolling_rom(strGenesisRomFile_a, arrChainHistory, intChainCount,
                                   arrTrunkHistory, intTrunkCount, intTargetIndex, byRollingRom))
            {
                fprintf(stderr, "Error: Failed to build rolling ROM\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Rolling ROM reconstructed\n");
        }

        // --- 7. Decode ENTIRE Block ---
        size_t intDecodedLen = 0;

        if (intResult == 0)
        {
            byDecodedBlock = zoscii_decode_block(byRollingRom, byEncodedBlock,
                                                 intEncodedLen, &intDecodedLen);
            if (!byDecodedBlock)
            {
                fprintf(stderr, "Error: ZOSCII decoding failed\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Decoded block: %zu bytes\n", intDecodedLen);
        }

        // --- 8. Extract Header and Payload ---
        if (intResult == 0)
        {
            if (intDecodedLen < sizeof(ZTB_BlockHeader))
            {
                fprintf(stderr, "Error: Decoded block too small for header\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            ZTB_BlockHeader *objHeader = (ZTB_BlockHeader*)byDecodedBlock;

            printf("\n--- Block Header ---\n");
            printf("Block ID:      %s\n", objHeader->block_id);
            printf("Prev Block ID: %s\n", objHeader->prev_block_id);
            printf("Trunk ID:      %s\n", objHeader->trunk_id);
            printf("Is Branch:     %s\n", objHeader->is_branch ? "Yes" : "No");
            printf("Payload Len:   %u bytes\n", objHeader->payload_len);
            printf("Padded Len:    %u bytes\n", objHeader->padded_len);
            printf("Timestamp:     %lu\n", (unsigned long)objHeader->timestamp);

            // --- 9. Output Payload ---
            uint8_t *byPayload = byDecodedBlock + sizeof(ZTB_BlockHeader);
            printf("\n--- Decoded Payload (%u bytes) ---\n", objHeader->payload_len);
            fwrite(byPayload, 1, objHeader->payload_len, stdout);
            printf("\n--- End Payload ---\n");
        }
    }

    // --- Cleanup ---
    if (byEncodedBlock) { free(byEncodedBlock); }
    if (byRollingRom) { free(byRollingRom); }
    if (byDecodedBlock) { free(byDecodedBlock); }
    if (arrChainHistory) { free(arrChainHistory); }
    if (arrTrunkHistory) { free(arrTrunkHistory); }

    return intResult;
}