// Cyborg ZTB Block Creator v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Usage: ztbaddblock <genesis_rom> <chain_id> -t "text string"
// Usage: ztbaddblock <genesis_rom> <chain_id> -f <file_path>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *byRawPayload = NULL;
    uint8_t *byPaddedPayload = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byRawBlock = NULL;
    uint8_t *byEncodedBlock = NULL;
    BlockInfo *arrHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    printf("ZTB Block Creator v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -t \"text\"\n", argv[0]);
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -f <file>\n", argv[0]);
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strChainId_a = argv[2];
        const char *strFlag_a = argv[3];
        const char *strDataSource_a = argv[4];

        // Seed RNG from genesis ROM content
        uint8_t *bySeedRom = load_rom_and_seed_rng(strGenesisRomFile_a);
        if (bySeedRom) { free(bySeedRom); }

        size_t intPayloadLen = 0;
        size_t intPaddedLen = 0;

        // --- 1. Load or create payload ---
        if (strcmp(strFlag_a, "-t") == 0)
        {
            intPayloadLen = strlen(strDataSource_a);
            byRawPayload = malloc(intPayloadLen);
            if (byRawPayload)
            {
                memcpy(byRawPayload, strDataSource_a, intPayloadLen);
            }
            else
            {
                fprintf(stderr, "Error: Cannot allocate payload\n");
                intResult = 1;
            }
        }
        else if (strcmp(strFlag_a, "-f") == 0)
        {
            FILE *f = fopen(strDataSource_a, "rb");
            if (!f)
            {
                fprintf(stderr, "Error: Cannot open file '%s'\n", strDataSource_a);
                intResult = 1;
            }
            else
            {
                fseek(f, 0, SEEK_END);
                intPayloadLen = ftell(f);
                fseek(f, 0, SEEK_SET);

                byRawPayload = malloc(intPayloadLen);
                if (byRawPayload)
                {
                    if (fread(byRawPayload, 1, intPayloadLen, f) != intPayloadLen)
                    {
                        fprintf(stderr, "Error: Failed to read file '%s'\n", strDataSource_a);
                        intResult = 1;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: Cannot allocate payload\n");
                    intResult = 1;
                }
                fclose(f);
            }
        }
        else
        {
            fprintf(stderr, "Error: Invalid flag '%s'\n", strFlag_a);
            intResult = 1;
        }

        // --- 2. Apply padding ---
        if (intResult == 0)
        {
            printf("Payload loaded: %zu bytes\n", intPayloadLen);

            intPaddedLen = (intPayloadLen < MIN_PAYLOAD_SIZE) ? MIN_PAYLOAD_SIZE : intPayloadLen;
            byPaddedPayload = calloc(intPaddedLen, 1);
            if (byPaddedPayload)
            {
                memcpy(byPaddedPayload, byRawPayload, intPayloadLen);
                free(byRawPayload);
                byRawPayload = NULL;

                if (intPaddedLen > intPayloadLen)
                {
                    printf("Padded to %zu bytes\n", intPaddedLen);
                }
            }
            else
            {
                fprintf(stderr, "Error: Cannot allocate padded payload\n");
                intResult = 1;
            }
        }

        // --- 3. Scan existing blocks ---
        int intBlockCount = 0;
        int intNewIndex = 0;
        char strPrevBlockId[GUID_LEN];
        uint8_t byIsBranch = 0;
        char strTrunkId[GUID_LEN];
        int intTrunkCount = 0;

        strcpy(strPrevBlockId, NULL_GUID);
        strcpy(strTrunkId, NULL_GUID);

        if (intResult == 0)
        {
            arrHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (!arrHistory)
            {
                fprintf(stderr, "Error: Cannot allocate block history\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            intBlockCount = scan_chain_blocks(strChainId_a, arrHistory, MAX_BLOCKS_TO_SCAN);
            intNewIndex = intBlockCount + 1;

            if (intBlockCount > 0)
            {
                // Detect if this chain is a branch
                byIsBranch = detect_branch_status(strGenesisRomFile_a, arrHistory, intBlockCount,
                                                   strChainId_a, strTrunkId) ? 1 : 0;

                strncpy(strPrevBlockId, arrHistory[intBlockCount - 1].block_id, GUID_LEN - 1);
                printf("Continuing chain from block %d\n", intBlockCount);
            }
            else
            {
                printf("Creating first block for chain\n");
            }
        }

        // --- 4. Build Rolling ROM ---
        if (intResult == 0)
        {
            if (byIsBranch && strcmp(strTrunkId, NULL_GUID) != 0)
            {
                arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
                if (arrTrunkHistory)
                {
                    intTrunkCount = scan_chain_blocks(strTrunkId, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);
                }
            }

            byRollingRom = malloc(ROM_SIZE);
            if (!byRollingRom)
            {
                fprintf(stderr, "Error: Cannot allocate rolling ROM\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            if (!build_rolling_rom(strGenesisRomFile_a, arrHistory, intBlockCount,
                                   arrTrunkHistory, intTrunkCount, intNewIndex, byRollingRom))
            {
                fprintf(stderr, "Error: Failed to build rolling ROM\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Rolling ROM built from %d previous blocks\n", intBlockCount);

            // --- 5. Create Block (Header + Payload) ---
            ZTB_BlockHeader objHeader;
            generate_guid(objHeader.block_id);
            strncpy(objHeader.prev_block_id, strPrevBlockId, GUID_LEN - 1);
            objHeader.prev_block_id[GUID_LEN - 1] = '\0';
            strncpy(objHeader.trunk_id, strTrunkId, GUID_LEN - 1);
            objHeader.trunk_id[GUID_LEN - 1] = '\0';
            objHeader.payload_len = intPayloadLen;
            objHeader.padded_len = intPaddedLen;
            objHeader.timestamp = time(NULL);
            objHeader.is_branch = byIsBranch;

            printf("Block ID: %s\n", objHeader.block_id);

            // Create complete raw block: header + payload
            size_t intRawBlockLen = sizeof(ZTB_BlockHeader) + intPaddedLen;
            byRawBlock = malloc(intRawBlockLen);
            if (byRawBlock)
            {
                memcpy(byRawBlock, &objHeader, sizeof(ZTB_BlockHeader));
                memcpy(byRawBlock + sizeof(ZTB_BlockHeader), byPaddedPayload, intPaddedLen);

                // --- 6. ZOSCII Encode ENTIRE Block ---
                size_t intEncodedLen;
                byEncodedBlock = zoscii_encode_block(byRollingRom, byRawBlock,
                                                     intRawBlockLen, &intEncodedLen);
                if (byEncodedBlock)
                {
                    // --- 7. Calculate CRC32 over encoded bytes ---
                    uint32_t intCrc = calculate_checksum(byEncodedBlock, intEncodedLen);
                    printf("CRC32: 0x%08X\n", intCrc);
                    printf("ZOSCII encoded: %zu bytes -> %zu bytes\n", intRawBlockLen, intEncodedLen);

                    // --- 8. Write CRC32 prefix + Encoded Block File ---
                    char strFilename[FILENAME_MAX];
                    snprintf(strFilename, FILENAME_MAX, "%s_%04d_%s.ztb",
                             strChainId_a, intNewIndex, objHeader.block_id);

                    FILE *f_out = fopen(strFilename, "wb");
                    if (f_out)
                    {
                        uint8_t arrCrcBytes[CRC32_PREFIX_SIZE];
                        arrCrcBytes[0] = intCrc & 0xFF;
                        arrCrcBytes[1] = (intCrc >> 8) & 0xFF;
                        arrCrcBytes[2] = (intCrc >> 16) & 0xFF;
                        arrCrcBytes[3] = (intCrc >> 24) & 0xFF;

                        if (fwrite(arrCrcBytes, 1, CRC32_PREFIX_SIZE, f_out) == CRC32_PREFIX_SIZE &&
                            fwrite(byEncodedBlock, 1, intEncodedLen, f_out) == intEncodedLen)
                        {
                            printf("\n+ Block created: %s\n", strFilename);
                        }
                        else
                        {
                            fprintf(stderr, "Error: Failed to write block file\n");
                            intResult = 1;
                        }
                        fclose(f_out);
                    }
                    else
                    {
                        fprintf(stderr, "Error: Cannot create file '%s'\n", strFilename);
                        intResult = 1;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: ZOSCII encoding failed\n");
                    intResult = 1;
                }
            }
            else
            {
                fprintf(stderr, "Error: Cannot allocate raw block\n");
                intResult = 1;
            }
        }
    }

    // --- Cleanup ---
    if (byRawPayload) { free(byRawPayload); }
    if (byPaddedPayload) { free(byPaddedPayload); }
    if (byRollingRom) { free(byRollingRom); }
    if (byRawBlock) { free(byRawBlock); }
    if (byEncodedBlock) { free(byEncodedBlock); }
    if (arrHistory) { free(arrHistory); }
    if (arrTrunkHistory) { free(arrTrunkHistory); }

    return intResult;
}