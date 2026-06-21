// Cyborg ZTB Add Block v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Adds a block to a chain. Matches ZTBChain.AddBlock/WriteBlock exactly.
//
// Usage: ztbaddblock <workdir> <chain_id> <new_block_id> <prev_block_id> -t "text" | -f <file>
//
// prev_block_id: use 00000000-0000-0000-0000-000000000000 for the first block

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Add Block v20260618\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 7 || argc > 7)
    {
        fprintf(stderr, "Usage: %s <workdir> <chain_id> <new_block_id> <prev_block_id> -t \"text\"\n", argv[0]);
        fprintf(stderr, "       %s <workdir> <chain_id> <new_block_id> <prev_block_id> -f <file>\n", argv[0]);
        fprintf(stderr, "\n  prev_block_id: %s for first block\n", NULL_GUID);
        intResult = 1;
    }

    uint8_t *byPayload    = NULL;
    uint8_t *byPadded     = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byEncRaw     = NULL;
    uint8_t *byEncZoscii  = NULL;
    uint8_t *byFinal      = NULL;

    if (intResult == 0)
    {
        const char *strWorkDir_a    = argv[1];
        const char *strChainID_a    = argv[2];
        const char *strNewBlockID_a = argv[3];
        const char *strPrevBlockID_a = argv[4];
        const char *strFlag_a       = argv[5];
        const char *strData_a       = argv[6];

        // --- 1. Load payload ---
        int intPayloadLen = 0;

        if (strcmp(strFlag_a, "-t") == 0)
        {
            intPayloadLen = (int)strlen(strData_a);
            byPayload     = (uint8_t*)malloc(intPayloadLen > 0 ? intPayloadLen : 1);
            if (!byPayload) { fprintf(stderr, "Error: Cannot allocate payload\n"); intResult = 1; }
            if (intResult == 0 && intPayloadLen > 0)
            {
                memcpy(byPayload, strData_a, intPayloadLen);
            }
        }
        else if (strcmp(strFlag_a, "-f") == 0)
        {
            FILE *f = fopen(strData_a, "rb");
            if (!f) { fprintf(stderr, "Error: Cannot open file: %s\n", strData_a); intResult = 1; }
            else
            {
                fseek(f, 0, SEEK_END);
                intPayloadLen = (int)ftell(f);
                fseek(f, 0, SEEK_SET);
                byPayload = (uint8_t*)malloc(intPayloadLen > 0 ? intPayloadLen : 1);
                if (!byPayload) { fprintf(stderr, "Error: Cannot allocate payload\n"); intResult = 1; }
                if (intResult == 0 && intPayloadLen > 0)
                {
                    if (fread(byPayload, 1, intPayloadLen, f) != (size_t)intPayloadLen)
                    {
                        fprintf(stderr, "Error: Cannot read file: %s\n", strData_a);
                        intResult = 1;
                    }
                }
                fclose(f);
            }
        }
        else
        {
            fprintf(stderr, "Error: Unknown flag '%s'\n", strFlag_a);
            intResult = 1;
        }

        // --- 2. Pad payload so the COMPLETE on-disk block reaches at least ROM_ENTRY_SIZE
        //        (1024) bytes. The rolling ROM unconditionally copies ROM_ENTRY_SIZE bytes
        //        from each historical block's full on-disk file -- if any block were smaller
        //        than that, the sample would run past the end of the block. Total on-disk
        //        size = raw header (never encoded) + 2x(encoded-section header + payload),
        //        since ZOSCII encoding doubles every byte it touches. (matches C# exactly)
        if (intResult == 0)
        {
            int intPaddedLen = intPayloadLen;
            int intTotalSize = HEADER_RAW_SIZE + 2 * (ENC_HEADER_SIZE + intPaddedLen);

            if (intTotalSize <= ROM_ENTRY_SIZE)
            {
                intPaddedLen = (ROM_ENTRY_SIZE - HEADER_RAW_SIZE) / 2 - ENC_HEADER_SIZE + 1;
                if (intPaddedLen < intPayloadLen) { intPaddedLen = intPayloadLen; }
            }

            byPadded         = (uint8_t*)malloc(intPaddedLen);
            if (!byPadded) { fprintf(stderr, "Error: Cannot allocate padded buffer\n"); intResult = 1; }

            if (intResult == 0)
            {
                memset(byPadded, 0, intPaddedLen);
                if (intPayloadLen > 0) { memcpy(byPadded, byPayload, intPayloadLen); }

                // Fill padding with XorShift32 (matches C# WriteBlock padding)
                if (intPaddedLen > intPayloadLen)
                {
                    uint32_t intSeed = (uint32_t)(time(NULL) & 0xFFFFFFFF);
                    intSeed          = xorshift32(intSeed);
                    int intI         = intPayloadLen;
                    while (intI < intPaddedLen)
                    {
                        intSeed      = xorshift32(intSeed);
                        byPadded[intI] = (uint8_t)(intSeed & 0xFF);
                        intI++;
                    }
                }

                // --- 3. Build raw header (matches WriteRawHeader) ---
                uint8_t arrRawHeader[HEADER_RAW_SIZE];
                memset(arrRawHeader, 0, HEADER_RAW_SIZE);
                arrRawHeader[RAW_OFF_BLOCK_TYPE] = BLOCK_TYPE_NORMAL;
                arrRawHeader[RAW_OFF_BLOCK_VER]  = BLOCK_VERSION;
                arrRawHeader[RAW_OFF_IS_BRANCH]  = 0;
                write_fixed_string(arrRawHeader, RAW_OFF_TRUNK_ID, 36, NULL_GUID);
                write_fixed_string(arrRawHeader, RAW_OFF_BLOCK_ID, 36, strNewBlockID_a);
                write_fixed_string(arrRawHeader, RAW_OFF_PREV_ID,  36,
                                   strPrevBlockID_a ? strPrevBlockID_a : NULL_GUID);

                // --- 4. Build rolling ROM ---
                byRollingRom = build_rolling_rom(strWorkDir_a, strPrevBlockID_a);
                if (!byRollingRom)
                {
                    fprintf(stderr, "Error: Cannot build rolling ROM\n");
                    intResult = 1;
                }

                if (intResult == 0)
                {
                    // --- 5. Compute prev_hash ---
                    uint32_t intPrevHash = 0;
                    if (strPrevBlockID_a != NULL && strcmp(strPrevBlockID_a, NULL_GUID) != 0)
                    {
                        int intPrevLen = 0;
                        uint8_t *byPrev = load_block(strWorkDir_a, strPrevBlockID_a, &intPrevLen);
                        if (byPrev)
                        {
                            intPrevHash = hash_bytes(HASH_TYPE_CRC32_FULL, byPrev, 0, intPrevLen);
                            free(byPrev);
                        }
                    }

                    // --- 6. Build encoded section with the real hash and prev hash ---
                    int intEncRawLen = ENC_HEADER_SIZE + intPaddedLen;
                    byEncRaw         = (uint8_t*)malloc(intEncRawLen);
                    if (!byEncRaw) { fprintf(stderr, "Error: Cannot allocate enc raw\n"); intResult = 1; }

                    if (intResult == 0)
                    {
                        // Current block hash covers the payload ONLY (not header/metadata).
                        // This keeps the payload hashable independently of the header, which
                        // matters once payload hashing is streamed -- the header is always
                        // small and fully in-memory, so it must never be folded into a hash
                        // that the payload portion needs to compute incrementally.
                        uint32_t intHash = hash_bytes(HASH_TYPE_CRC32_FULL, byPadded, 0, intPaddedLen);

                        byEncRaw[ENC_OFF_HASH_TYPE] = HASH_TYPE_CRC32_FULL;
                        byEncRaw[ENC_OFF_HASH]     = (uint8_t)(intHash & 0xFF);
                        byEncRaw[ENC_OFF_HASH + 1] = (uint8_t)((intHash >> 8)  & 0xFF);
                        byEncRaw[ENC_OFF_HASH + 2] = (uint8_t)((intHash >> 16) & 0xFF);
                        byEncRaw[ENC_OFF_HASH + 3] = (uint8_t)((intHash >> 24) & 0xFF);
                        byEncRaw[ENC_OFF_PREV_HASH]     = (uint8_t)(intPrevHash & 0xFF);
                        byEncRaw[ENC_OFF_PREV_HASH + 1] = (uint8_t)((intPrevHash >> 8)  & 0xFF);
                        byEncRaw[ENC_OFF_PREV_HASH + 2] = (uint8_t)((intPrevHash >> 16) & 0xFF);
                        byEncRaw[ENC_OFF_PREV_HASH + 3] = (uint8_t)((intPrevHash >> 24) & 0xFF);
                        byEncRaw[ENC_OFF_PAYLOAD_LEN]     = (uint8_t)(intPayloadLen & 0xFF);
                        byEncRaw[ENC_OFF_PAYLOAD_LEN + 1] = (uint8_t)((intPayloadLen >> 8)  & 0xFF);
                        byEncRaw[ENC_OFF_PAYLOAD_LEN + 2] = (uint8_t)((intPayloadLen >> 16) & 0xFF);
                        byEncRaw[ENC_OFF_PAYLOAD_LEN + 3] = (uint8_t)((intPayloadLen >> 24) & 0xFF);
                        byEncRaw[ENC_OFF_PADDED_LEN]     = (uint8_t)(intPaddedLen & 0xFF);
                        byEncRaw[ENC_OFF_PADDED_LEN + 1] = (uint8_t)((intPaddedLen >> 8)  & 0xFF);
                        byEncRaw[ENC_OFF_PADDED_LEN + 2] = (uint8_t)((intPaddedLen >> 16) & 0xFF);
                        byEncRaw[ENC_OFF_PADDED_LEN + 3] = (uint8_t)((intPaddedLen >> 24) & 0xFF);
                        memcpy(byEncRaw + ENC_OFF_PAYLOAD, byPadded, intPaddedLen);

                        // ZOSCII encode the encoded section
                        int intEncZosciiLen = 0;
                        byEncZoscii = zoscii_encode(byRollingRom, byEncRaw,
                                                    intEncRawLen, &intEncZosciiLen);
                        if (!byEncZoscii)
                        {
                            fprintf(stderr, "Error: ZOSCII encoding failed\n");
                            intResult = 1;
                        }

                        if (intResult == 0)
                        {
                            // Assemble final output: raw header + ZOSCII encoded section
                            int intFinalLen = HEADER_RAW_SIZE + intEncZosciiLen;
                            byFinal         = (uint8_t*)malloc(intFinalLen);
                            if (!byFinal) { fprintf(stderr, "Error: Cannot allocate output\n"); intResult = 1; }

                            if (intResult == 0)
                            {
                                memcpy(byFinal, arrRawHeader, HEADER_RAW_SIZE);
                                memcpy(byFinal + HEADER_RAW_SIZE, byEncZoscii, intEncZosciiLen);

                                // Write via tmp then rename
                                char strOutPath[FILENAME_MAX];
                                char strTmpPath[FILENAME_MAX + 4];
                                snprintf(strOutPath, FILENAME_MAX, "%s/%s.ztb",
                                         strWorkDir_a, strNewBlockID_a);
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
                                        remove(strOutPath);
                                        if (rename(strTmpPath, strOutPath) != 0)
                                        {
                                            fprintf(stderr, "Error: Cannot rename tmp\n");
                                            intResult = 1;
                                        }
                                    }
                                }

                                if (intResult == 0)
                                {
                                    printf("+ Block created: %s/%s.ztb\n",
                                           strWorkDir_a, strNewBlockID_a);
                                    printf("  Chain:        %s\n", strChainID_a);
                                    printf("  Block ID:     %s\n", strNewBlockID_a);
                                    printf("  Prev ID:      %s\n",
                                           strPrevBlockID_a ? strPrevBlockID_a : NULL_GUID);
                                    printf("  Hash:         0x%08X\n", intHash);
                                    printf("  PrevHash:     0x%08X\n", intPrevHash);
                                    printf("  PayloadLen:   %d\n", intPayloadLen);
                                    printf("  PaddedLen:    %d\n", intPaddedLen);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (byPayload)    { free(byPayload); }
    if (byPadded)     { free(byPadded); }
    if (byRollingRom) { free(byRollingRom); }
    if (byEncRaw)     { free(byEncRaw); }
    if (byEncZoscii)  { free(byEncZoscii); }
    if (byFinal)      { free(byFinal); }

    return intResult;
}