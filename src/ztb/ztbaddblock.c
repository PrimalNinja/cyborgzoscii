// Cyborg ZTB Block Creator v20260420
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Usage: ztbaddblock <genesis_rom> <chain_id> -t "text string" [-x1] [-x2] [-i <block_id>]
// Usage: ztbaddblock <genesis_rom> <chain_id> -f <file_path>   [-x1] [-x2] [-i <block_id>]

#include "ztbcommon.c"

// --- Validate a GUID string (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, hex digits only) ---
/* static int is_valid_guid(const char *strGuid_a)
{
    // Expected format: 8-4-4-4-12 hex digits separated by hyphens = 36 chars
    if (!strGuid_a || strlen(strGuid_a) != 36) { return 0; }
    int arrGroupLen[] = {8, 4, 4, 4, 12};
    int intPos = 0;
    int intG;
    for (intG = 0; intG < 5; intG++)
    {
        int intL;
        for (intL = 0; intL < arrGroupLen[intG]; intL++)
        {
            char c = strGuid_a[intPos++];
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f')))
            {
                return 0;
            }
        }
        if (intG < 4 && strGuid_a[intPos++] != '-') { return 0; }
    }
    return 1;
} */

int main(int argc, char *argv[])
{
    int intResult = 0;
    uint8_t *byRawPayload = NULL;
    uint8_t *byPaddedPayload = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byRawBlock = NULL;
    uint8_t *byEncodedBlock = NULL;
    uint8_t *byPrefixEncoded = NULL;
    uint8_t *byFinalOutput = NULL;
    BlockInfo *arrHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    printf("ZTB Block Creator v20260420\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    // Argument parsing:
    // Required positional: <genesis_rom> <chain_id> <-t|-f> <data>
    // Optional flags (any order after positional): -x1, -x2, -i <block_id>
    // Minimum argc: 5 (no optional flags)
    // Maximum argc: 8 (-x1 or -x2 with -i <block_id>)

    if (argc < 5 || argc > 8)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -t \"text\" [-x1] [-x2] [-i <block_id>]\n", argv[0]);
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -f <file>   [-x1] [-x2] [-i <block_id>]\n", argv[0]);
        fprintf(stderr, "\n  -x1           Extended security: prev-block CRC binding (no XOR)\n");
        fprintf(stderr, "  -x2           Extended security: prev-block CRC binding + on-disk XOR\n");
        fprintf(stderr, "  -i <block_id> Use supplied GUID as the block ID instead of auto-generating\n");
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strChainId_a = argv[2];
        const char *strFlag_a = argv[3];
        const char *strDataSource_a = argv[4];
        uint8_t byMode = MODE_NORMAL;
        const char *strSuppliedBlockId = NULL;

        // Scan optional flags from argv[5] onwards
        int intArg;
        for (intArg = 5; intArg < argc && intResult == 0; intArg++)
        {
            if (strcmp(argv[intArg], "-x1") == 0)
            {
                byMode = MODE_X1;
            }
            else if (strcmp(argv[intArg], "-x2") == 0)
            {
                byMode = MODE_X2;
            }
            else if (strcmp(argv[intArg], "-i") == 0)
            {
                if (intArg + 1 >= argc)
                {
                    fprintf(stderr, "Error: -i requires a block_id argument\n");
                    intResult = 1;
                }
                else
                {
                    intArg++;
                    strSuppliedBlockId = argv[intArg];
                    // if (!is_valid_guid(strSuppliedBlockId))
                    // {
                        // fprintf(stderr, "Error: Invalid block_id '%s'\n", strSuppliedBlockId);
                        // fprintf(stderr, "       Must be xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (hex digits)\n");
                        // intResult = 1;
                    // }
                }
            }
            else
            {
                fprintf(stderr, "Error: Unknown option '%s'\n", argv[intArg]);
                intResult = 1;
            }
        }

        if (intResult == 0 && byMode == MODE_X1)
        {
            printf("Mode: X1 (extended security, CRC binding)\n");
        }
        else if (intResult == 0 && byMode == MODE_X2)
        {
            printf("Mode: X2 (extended security, CRC binding + XOR)\n");
        }

        if (intResult == 0 && strSuppliedBlockId)
        {
            printf("Block ID: %s (supplied)\n", strSuppliedBlockId);
        }

        // Seed RNG from genesis ROM content
        uint8_t *bySeedRom = load_rom_and_seed_rng(strGenesisRomFile_a);
        if (bySeedRom) { free(bySeedRom); }

        size_t intPayloadLen = 0;
        size_t intPaddedLen = 0;

        // --- 1. Load or create payload ---
        if (intResult == 0)
        {
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
        char strPrevBlockFilename[FILENAME_MAX];
        uint8_t byIsBranch = 0;
        char strTrunkId[GUID_LEN];
        int intTrunkCount = 0;

        strcpy(strPrevBlockId, NULL_GUID);
        strcpy(strPrevBlockFilename, "");
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
                strncpy(strPrevBlockFilename, arrHistory[intBlockCount - 1].filename, FILENAME_MAX - 1);
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

            // Use supplied block ID if provided, otherwise auto-generate
            if (strSuppliedBlockId)
            {
                strncpy(objHeader.block_id, strSuppliedBlockId, GUID_LEN - 1);
                objHeader.block_id[GUID_LEN - 1] = '\0';
            }
            else
            {
                generate_guid(objHeader.block_id);
                printf("Block ID: %s\n", objHeader.block_id);
            }

            strncpy(objHeader.prev_block_id, strPrevBlockId, GUID_LEN - 1);
            objHeader.prev_block_id[GUID_LEN - 1] = '\0';
            strncpy(objHeader.trunk_id, strTrunkId, GUID_LEN - 1);
            objHeader.trunk_id[GUID_LEN - 1] = '\0';
            objHeader.payload_len = intPayloadLen;
            objHeader.padded_len = intPaddedLen;
            objHeader.timestamp = time(NULL);
            objHeader.is_branch = byIsBranch;

            // Create complete raw block: header + payload
            size_t intRawBlockLen = sizeof(ZTB_BlockHeader) + intPaddedLen;
            byRawBlock = malloc(intRawBlockLen);
            if (!byRawBlock)
            {
                fprintf(stderr, "Error: Cannot allocate raw block\n");
                intResult = 1;
            }

            if (intResult == 0)
            {
                memcpy(byRawBlock, &objHeader, sizeof(ZTB_BlockHeader));
                memcpy(byRawBlock + sizeof(ZTB_BlockHeader), byPaddedPayload, intPaddedLen);

                // --- 6. ZOSCII encode with plain rolling ROM (always) ---
                size_t intEncodedLen;
                byEncodedBlock = zoscii_encode_block(byRollingRom, byRawBlock,
                                                     intRawBlockLen, &intEncodedLen);
                if (!byEncodedBlock)
                {
                    fprintf(stderr, "Error: ZOSCII encoding failed\n");
                    intResult = 1;
                }

                if (intResult == 0)
                {
                    // --- 7. CRC32 over encoded block data ---
                    uint32_t intCrc = calculate_checksum(byEncodedBlock, intEncodedLen);
                    printf("CRC32 (current block): 0x%08X\n", intCrc);
                    printf("ZOSCII encoded: %zu bytes -> %zu bytes\n", intRawBlockLen, intEncodedLen);

                    // --- 8. CRC32 of previous block's on-disk data (X1/X2 only; zero otherwise) ---
                    uint32_t intPrevCrc = 0;
                    if ((byMode == MODE_X1 || byMode == MODE_X2) && strPrevBlockFilename[0] != '\0')
                    {
                        intPrevCrc = calculate_file_checksum(strPrevBlockFilename);
                        printf("CRC32 (previous block on-disk): 0x%08X\n", intPrevCrc);
                    }

                    // --- 9. Assemble and encode prefix with plain rolling ROM ---
                    // Mode byte is first (bytes 0-1 encoded) so readers can always
                    // identify the mode without any prior context.
                    uint8_t arrPrefixRaw[MODE_SIZE + CRC32_SIZE * 2];
                    arrPrefixRaw[0] = byMode;
                    arrPrefixRaw[1] = intCrc & 0xFF;
                    arrPrefixRaw[2] = (intCrc >> 8) & 0xFF;
                    arrPrefixRaw[3] = (intCrc >> 16) & 0xFF;
                    arrPrefixRaw[4] = (intCrc >> 24) & 0xFF;
                    arrPrefixRaw[5] = intPrevCrc & 0xFF;
                    arrPrefixRaw[6] = (intPrevCrc >> 8) & 0xFF;
                    arrPrefixRaw[7] = (intPrevCrc >> 16) & 0xFF;
                    arrPrefixRaw[8] = (intPrevCrc >> 24) & 0xFF;

                    size_t intPrefixEncodedLen;
                    byPrefixEncoded = zoscii_encode_block(byRollingRom, arrPrefixRaw,
                                                          sizeof(arrPrefixRaw), &intPrefixEncodedLen);
                    if (!byPrefixEncoded)
                    {
                        fprintf(stderr, "Error: Failed to encode prefix\n");
                        intResult = 1;
                    }

                    if (intResult == 0)
                    {
                        // --- 10. Concatenate prefix + encoded block into final output ---
                        size_t intFinalLen = intPrefixEncodedLen + intEncodedLen;
                        byFinalOutput = malloc(intFinalLen);
                        if (!byFinalOutput)
                        {
                            fprintf(stderr, "Error: Cannot allocate final output buffer\n");
                            intResult = 1;
                        }

                        if (intResult == 0)
                        {
                            memcpy(byFinalOutput, byPrefixEncoded, intPrefixEncodedLen);
                            memcpy(byFinalOutput + intPrefixEncodedLen, byEncodedBlock, intEncodedLen);

                            // --- 11. X2: XOR entire output with previous block file ---
                            // Only performed in X2 mode. Block 1 of a trunk has no previous
                            // block — written as-is. Encoded mode bytes 0-1 are restored after
                            // XOR so readers can always identify the mode without un-XOR'ing first.
                            if (byMode == MODE_X2 && strPrevBlockFilename[0] != '\0')
                            {
                                uint8_t byModeSave0 = byFinalOutput[0];
                                uint8_t byModeSave1 = byFinalOutput[1];

                                if (!xor_buffer_with_file(byFinalOutput, intFinalLen, strPrevBlockFilename))
                                {
                                    fprintf(stderr, "Error: X1 XOR failed\n");
                                    intResult = 1;
                                }
                                else
                                {
                                    byFinalOutput[0] = byModeSave0;
                                    byFinalOutput[1] = byModeSave1;
                                    printf("X2: Output XOR'd with previous block (%s)\n", strPrevBlockFilename);
                                }
                            }

                            // --- 12. Write final output ---
                            if (intResult == 0)
                            {
                                char strFilename[FILENAME_MAX];
                                snprintf(strFilename, FILENAME_MAX, "%s_%04d_%s.ztb",
                                         strChainId_a, intNewIndex, objHeader.block_id);

                                FILE *f_out = fopen(strFilename, "wb");
                                if (f_out)
                                {
                                    if (fwrite(byFinalOutput, 1, intFinalLen, f_out) == intFinalLen)
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
                        }
                    }
                }
            }
        }
    }

    // --- Cleanup ---
    if (byRawPayload)    { free(byRawPayload); }
    if (byPaddedPayload) { free(byPaddedPayload); }
    if (byRollingRom)    { free(byRollingRom); }
    if (byRawBlock)      { free(byRawBlock); }
    if (byEncodedBlock)  { free(byEncodedBlock); }
    if (byPrefixEncoded) { free(byPrefixEncoded); }
    if (byFinalOutput)   { free(byFinalOutput); }
    if (arrHistory)      { free(arrHistory); }
    if (arrTrunkHistory) { free(arrTrunkHistory); }

    return intResult;
}