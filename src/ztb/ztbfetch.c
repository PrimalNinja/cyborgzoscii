// Cyborg ZTB Block Fetcher v20260420
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Usage: ztbfetch <genesis_rom> <chain_id> <block_index>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *byFileData = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byDecodedBlock = NULL;
    BlockInfo *arrChainHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    printf("ZTB Block Fetcher v20260420\n");
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

        // --- 3. Load block file ---
        size_t intFileLen = 0;

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
                intFileLen = ftell(f);
                fseek(f, 0, SEEK_SET);

                if (intFileLen <= BLOCK_PREFIX_ENCODED_SIZE)
                {
                    fprintf(stderr, "Error: Block file too small\n");
                    intResult = 1;
                }
                else
                {
                    byFileData = malloc(intFileLen);
                    if (byFileData)
                    {
                        if (fread(byFileData, 1, intFileLen, f) != intFileLen)
                        {
                            fprintf(stderr, "Error: Cannot read block file\n");
                            intResult = 1;
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Error: Cannot allocate file buffer\n");
                        intResult = 1;
                    }
                }
                fclose(f);
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

            // --- 7. Find previous block filename for X1 mode ---
            // For a trunk block N>1: previous block is chain block N-1.
            // For a branch block 1: previous block is trunk's last block.
            // For trunk block 1 or branch block 1 with no trunk: no previous block.
            const char *strPrevBlockFilename = NULL;

            // First look in chain history for index-1
            int intI;
            for (intI = 0; intI < intChainCount; intI++)
            {
                if (arrChainHistory[intI].index == intTargetIndex - 1)
                {
                    strPrevBlockFilename = arrChainHistory[intI].filename;
                }
            }

            // If not found and this is branch block 1, use trunk's last block
            if (!strPrevBlockFilename && byIsBranch && intTargetIndex == 1 && intTrunkCount > 0)
            {
                strPrevBlockFilename = arrTrunkHistory[intTrunkCount - 1].filename;
            }

            // --- 8. Read mode from encoded bytes 0-1 (never XOR'd on disk) ---
            // Mode is always at the front and always plain, so we can identify
            // the block type before doing anything else.
            size_t intModeDecodedLen;
            uint8_t *byModeDecoded = zoscii_decode_block(byRollingRom, byFileData, 2,
                                                          &intModeDecodedLen);
            uint8_t byMode = MODE_NORMAL;
            uint32_t intStoredCrc = 0;
            uint32_t intStoredPrevCrc = 0;

            if (!byModeDecoded || intModeDecodedLen < 1)
            {
                fprintf(stderr, "Error: Failed to decode mode byte\n");
                intResult = 1;
                if (byModeDecoded) { free(byModeDecoded); byModeDecoded = NULL; }
            }
            else
            {
                byMode = byModeDecoded[0];
                free(byModeDecoded);
                byModeDecoded = NULL;

                if (byMode == MODE_X1)
                {
                    printf("Mode: X1 (extended security)\n");

                    if (strPrevBlockFilename == NULL)
                    {
                        // Block 1 of a trunk has no previous block and was written plain.
                        printf("X1: No previous block (block 1 of trunk) - stored plain\n");
                    }
                    else if (!xor_buffer_with_file(byFileData, intFileLen, strPrevBlockFilename))
                    {
                        fprintf(stderr, "Error: X1 un-XOR failed\n");
                        intResult = 1;
                    }
                    else
                    {
                        printf("X1: File data un-XOR'd with previous block\n");
                        // Bytes 0-1 are now doubly-XOR'd but we already have byMode.
                        // Bytes 2-17 (the two CRCs) are correctly un-XOR'd.
                    }
                }

                // --- 9. Decode CRC fields from encoded bytes 2-17 ---
                if (intResult == 0)
                {
                    size_t intCrcDecodedLen;
                    uint8_t *byCrcDecoded = zoscii_decode_block(byRollingRom, byFileData + 2,
                                                                 CRC_PREFIX_ENCODED_SIZE,
                                                                 &intCrcDecodedLen);
                    if (!byCrcDecoded || intCrcDecodedLen < CRC32_SIZE * 2)
                    {
                        fprintf(stderr, "Error: Failed to decode CRC fields\n");
                        intResult = 1;
                        if (byCrcDecoded) { free(byCrcDecoded); byCrcDecoded = NULL; }
                    }
                    else
                    {
                        intStoredCrc     = byCrcDecoded[0] | (byCrcDecoded[1] << 8) |
                                           (byCrcDecoded[2] << 16) | (byCrcDecoded[3] << 24);
                        intStoredPrevCrc = byCrcDecoded[4] | (byCrcDecoded[5] << 8) |
                                           (byCrcDecoded[6] << 16) | (byCrcDecoded[7] << 24);
                        free(byCrcDecoded);

                        if (byMode == MODE_X1 && intStoredPrevCrc != 0 && strPrevBlockFilename)
                        {
                            uint32_t intCalcPrevCrc = calculate_file_checksum(strPrevBlockFilename);
                            printf("Stored prev CRC32:     0x%08X\n", intStoredPrevCrc);
                            printf("Calculated prev CRC32: 0x%08X\n", intCalcPrevCrc);

                            if (intCalcPrevCrc != intStoredPrevCrc)
                            {
                                fprintf(stderr, "\n!!! INTEGRITY FAILURE !!!\n");
                                fprintf(stderr, "Previous block CRC mismatch\n");
                                intResult = 1;
                            }
                            else
                            {
                                printf("+ Previous block CRC verified\n");
                            }
                        }
                    }
                }
            }

            // --- 10. Verify CRC32 over encoded block data ---
            if (intResult == 0)
            {
                size_t intEncodedLen = intFileLen - BLOCK_PREFIX_ENCODED_SIZE;
                uint32_t intCalcCrc = calculate_checksum(byFileData + BLOCK_PREFIX_ENCODED_SIZE,
                                                         intEncodedLen);
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

            // --- 11. Decode block data with plain rolling ROM ---
            if (intResult == 0)
            {
                size_t intEncodedLen = intFileLen - BLOCK_PREFIX_ENCODED_SIZE;
                size_t intDecodedLen = 0;

                byDecodedBlock = zoscii_decode_block(byRollingRom,
                                                     byFileData + BLOCK_PREFIX_ENCODED_SIZE,
                                                     intEncodedLen, &intDecodedLen);
                if (!byDecodedBlock)
                {
                    fprintf(stderr, "Error: ZOSCII decoding failed\n");
                    intResult = 1;
                }
                else
                {
                    printf("Decoded block: %zu bytes\n", intDecodedLen);

                    if (intDecodedLen < sizeof(ZTB_BlockHeader))
                    {
                        fprintf(stderr, "Error: Decoded block too small for header\n");
                        intResult = 1;
                    }
                }
            }

            // --- 12. Extract Header and Payload ---
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

                uint8_t *byPayload = byDecodedBlock + sizeof(ZTB_BlockHeader);
                printf("\n--- Decoded Payload (%u bytes) ---\n", objHeader->payload_len);
                fwrite(byPayload, 1, objHeader->payload_len, stdout);
                printf("\n--- End Payload ---\n");
            }
        }
    }

    // --- Cleanup ---
    if (byFileData)      { free(byFileData); }
    if (byRollingRom)    { free(byRollingRom); }
    if (byDecodedBlock)  { free(byDecodedBlock); }
    if (arrChainHistory) { free(arrChainHistory); }
    if (arrTrunkHistory) { free(arrTrunkHistory); }

    return intResult;
}