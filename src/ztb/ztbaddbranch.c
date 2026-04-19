// Cyborg ZTB Branch Creator v20260420
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Usage: ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -t "text"
// Usage: ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -f <file>

#include "ztbcommon.c"

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
    BlockInfo *arrTrunkHistory = NULL;
    BlockInfo *arrBranchHistory = NULL;

    printf("ZTB Branch Creator v20260420\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 6 || argc > 7)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id> <new_branch_id> -t \"text\" [-x1]\n", argv[0]);
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id> <new_branch_id> -f <file> [-x1]\n", argv[0]);
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strTrunkId_a = argv[2];
        const char *strNewBranchId_a = argv[3];
        const char *strFlag_a = argv[4];
        const char *strDataSource_a = argv[5];
        uint8_t byMode = MODE_NORMAL;

        if (argc == 7 && strcmp(argv[6], "-x1") == 0)
        {
            byMode = MODE_X1;
            printf("Mode: X1 (extended security)\n");
        }

        // Seed RNG from genesis ROM content
        uint8_t *bySeedRom = load_rom_and_seed_rng(strGenesisRomFile_a);
        if (bySeedRom) { free(bySeedRom); }

        size_t intPayloadLen = 0;
        size_t intPaddedLen = 0;
        int intTrunkCount = 0;

        // --- 1. Scan trunk blocks ---
        arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
        if (!arrTrunkHistory)
        {
            fprintf(stderr, "Error: Cannot allocate trunk history\n");
            intResult = 1;
        }

        if (intResult == 0)
        {
            intTrunkCount = scan_chain_blocks(strTrunkId_a, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);

            if (intTrunkCount == 0)
            {
                fprintf(stderr, "Error: Trunk '%s' not found.\n", strTrunkId_a);
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Found trunk '%s' with %d blocks\n", strTrunkId_a, intTrunkCount);

            // --- 2. Check if branch already exists ---
            arrBranchHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (arrBranchHistory)
            {
                int intBranchCount = scan_chain_blocks(strNewBranchId_a, arrBranchHistory, MAX_BLOCKS_TO_SCAN);

                if (intBranchCount > 0)
                {
                    fprintf(stderr, "Error: Branch '%s' already exists with %d blocks.\n",
                            strNewBranchId_a, intBranchCount);
                    fprintf(stderr, "Use ztbaddblock to add to existing branch.\n");
                    intResult = 1;
                }
            }
            else
            {
                fprintf(stderr, "Error: Cannot allocate branch history\n");
                intResult = 1;
            }
        }

        // --- 3. Load payload ---
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

        // --- 4. Apply padding ---
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

        // --- 5. Build Rolling ROM (trunk history -> genesis) ---
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
            if (!build_rolling_rom(strGenesisRomFile_a, NULL, 0,
                                   arrTrunkHistory, intTrunkCount, 1, byRollingRom))
            {
                fprintf(stderr, "Error: Failed to build rolling ROM\n");
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            printf("Rolling ROM built from trunk history\n");

            // --- 6. Create Branch Block Header ---
            ZTB_BlockHeader objHeader;
            generate_guid(objHeader.block_id);
            strncpy(objHeader.prev_block_id, arrTrunkHistory[intTrunkCount - 1].block_id, GUID_LEN - 1);
            objHeader.prev_block_id[GUID_LEN - 1] = '\0';
            strncpy(objHeader.trunk_id, strTrunkId_a, GUID_LEN - 1);
            objHeader.trunk_id[GUID_LEN - 1] = '\0';
            objHeader.payload_len = intPayloadLen;
            objHeader.padded_len = intPaddedLen;
            objHeader.timestamp = time(NULL);
            objHeader.is_branch = 1;

            printf("Branch Block ID: %s\n", objHeader.block_id);
            printf("Anchored to trunk block: %s\n", objHeader.prev_block_id);

            // --- 7. Create complete raw block ---
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

                // --- 8. ZOSCII encode with plain rolling ROM (always) ---
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
                    // --- 9. CRC32 over encoded block data ---
                    uint32_t intCrc = calculate_checksum(byEncodedBlock, intEncodedLen);
                    printf("CRC32 (current block): 0x%08X\n", intCrc);
                    printf("ZOSCII encoded: %zu bytes -> %zu bytes\n", intRawBlockLen, intEncodedLen);

                    // --- 10. CRC32 of trunk's last block on-disk data (X1 only; zero otherwise) ---
                    // For branch block 1, the "previous block" is the trunk's last block.
                    const char *strPrevBlockFilename = arrTrunkHistory[intTrunkCount - 1].filename;
                    uint32_t intPrevCrc = 0;
                    if (byMode == MODE_X1)
                    {
                        intPrevCrc = calculate_file_checksum(strPrevBlockFilename);
                        printf("CRC32 (previous trunk block on-disk): 0x%08X\n", intPrevCrc);
                    }

                    // --- 11. Assemble and encode prefix with plain rolling ROM ---
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
                        // --- 12. Concatenate prefix + encoded block into final output ---
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

                            // --- 13. X1: XOR entire output with trunk's last block file ---
                            // Branch block 1's previous block is always the trunk's last block.
                            // Encoded mode bytes 0-1 are restored after XOR so readers can always
                            // identify the mode without needing to un-XOR first.
                            if (byMode == MODE_X1)
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
                                    printf("X1: Output XOR'd with trunk's last block (%s)\n",
                                           strPrevBlockFilename);
                                }
                            }

                            // --- 14. Write final output ---
                            if (intResult == 0)
                            {
                                char strFilename[FILENAME_MAX];
                                snprintf(strFilename, FILENAME_MAX, "%s_%04d_%s.ztb",
                                         strNewBranchId_a, 1, objHeader.block_id);

                                FILE *f_out = fopen(strFilename, "wb");
                                if (f_out)
                                {
                                    if (fwrite(byFinalOutput, 1, intFinalLen, f_out) == intFinalLen)
                                    {
                                        printf("\n+ Branch block created: %s\n", strFilename);
                                        printf("+ New branch '%s' started from trunk '%s'\n",
                                               strNewBranchId_a, strTrunkId_a);
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
    if (arrTrunkHistory) { free(arrTrunkHistory); }
    if (arrBranchHistory){ free(arrBranchHistory); }

    return intResult;
}