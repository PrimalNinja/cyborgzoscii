// Cyborg ZTB Fetch Block v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Fetches and decodes a block. Matches ZTBChain.FetchBlock exactly.
//
// Usage: ztbfetch <workdir> <block_id>

#include "ztbcommon.c"

int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Fetch Block v20260618\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <workdir> <block_id>\n", argv[0]);
        intResult = 1;
    }

    uint8_t *byBlock      = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byDecoded    = NULL;

    if (intResult == 0)
    {
        const char *strWorkDir_a = argv[1];
        const char *strBlockID_a = argv[2];

        // --- 1. Load block ---
        int intBlockLen = 0;
        byBlock = load_block(strWorkDir_a, strBlockID_a, &intBlockLen);
        if (!byBlock || intBlockLen < HEADER_RAW_SIZE)
        {
            fprintf(stderr, "Error: Cannot load block '%s'\n", strBlockID_a);
            intResult = 1;
        }

        if (intResult == 0)
        {
            // --- 2. Read raw header ---
            uint8_t byBlockType  = byBlock[RAW_OFF_BLOCK_TYPE];
            uint8_t byIsBranch   = byBlock[RAW_OFF_IS_BRANCH];
            char strTrunkID[GUID_LEN];
            char strBlockID[GUID_LEN];
            char strPrevID[GUID_LEN];

            read_fixed_string(byBlock, RAW_OFF_TRUNK_ID, 36, strTrunkID);
            read_fixed_string(byBlock, RAW_OFF_BLOCK_ID, 36, strBlockID);
            read_fixed_string(byBlock, RAW_OFF_PREV_ID,  36, strPrevID);

            printf("--- Block Header ---\n");
            printf("Block ID:     %s\n", strBlockID);
            printf("Prev ID:      %s\n", strPrevID);
            printf("Trunk ID:     %s\n", strTrunkID);
            printf("Is Branch:    %s\n", byIsBranch ? "Yes" : "No");
            printf("Block Type:   %d\n", byBlockType);

            // Truncation block is not encoded — return directly (matches C# FetchBlock)
            if (byBlockType == BLOCK_TYPE_TRUNCATION)
            {
                printf("(Truncation block — payload is raw rolling ROM)\n");
            }
            else
            {
                // --- 3. Build rolling ROM ---
                byRollingRom = build_rolling_rom(strWorkDir_a, strPrevID);
                if (!byRollingRom)
                {
                    fprintf(stderr, "Error: Cannot build rolling ROM\n");
                    intResult = 1;
                }

                if (intResult == 0)
                {
                    // --- 4. ZOSCII decode encoded section ---
                    int intEncLen    = intBlockLen - HEADER_RAW_SIZE;
                    int intDecLen    = 0;
                    byDecoded        = zoscii_decode(byRollingRom, byBlock,
                                                     HEADER_RAW_SIZE, intEncLen, &intDecLen);
                    if (!byDecoded || intDecLen < ENC_HEADER_SIZE)
                    {
                        fprintf(stderr, "Error: ZOSCII decode failed\n");
                        intResult = 1;
                    }

                    if (intResult == 0)
                    {
                        // --- 5. Read encoded section fields ---
                        uint8_t byHashType     = byDecoded[ENC_OFF_HASH_TYPE];
                        uint32_t intStoredHash = (uint32_t)(byDecoded[ENC_OFF_HASH] |
                                                (byDecoded[ENC_OFF_HASH + 1] << 8)  |
                                                (byDecoded[ENC_OFF_HASH + 2] << 16) |
                                                (byDecoded[ENC_OFF_HASH + 3] << 24));
                        uint32_t intStoredPrevHash = (uint32_t)(byDecoded[ENC_OFF_PREV_HASH] |
                                                    (byDecoded[ENC_OFF_PREV_HASH + 1] << 8)  |
                                                    (byDecoded[ENC_OFF_PREV_HASH + 2] << 16) |
                                                    (byDecoded[ENC_OFF_PREV_HASH + 3] << 24));
                        uint32_t intPayloadLen = (uint32_t)(byDecoded[ENC_OFF_PAYLOAD_LEN] |
                                                (byDecoded[ENC_OFF_PAYLOAD_LEN + 1] << 8)  |
                                                (byDecoded[ENC_OFF_PAYLOAD_LEN + 2] << 16) |
                                                (byDecoded[ENC_OFF_PAYLOAD_LEN + 3] << 24));
                        uint32_t intPaddedLen  = (uint32_t)(byDecoded[ENC_OFF_PADDED_LEN] |
                                                (byDecoded[ENC_OFF_PADDED_LEN + 1] << 8)  |
                                                (byDecoded[ENC_OFF_PADDED_LEN + 2] << 16) |
                                                (byDecoded[ENC_OFF_PADDED_LEN + 3] << 24));

                        // --- 6. Verify hash --- current-block hash covers the payload ONLY,
                        // not the header or encoded-section metadata (see ztbaddblock.c).
                        int intPaddedPayloadLen = intDecLen - ENC_OFF_PAYLOAD;
                        uint32_t intCalcHash = hash_bytes((int)byHashType,
                                                          byDecoded + ENC_OFF_PAYLOAD,
                                                          0, intPaddedPayloadLen);
                        int blnHashOK = (intCalcHash == intStoredHash);

                        // Verify prev hash
                        int blnPrevHashOK = 1;
                        if (intStoredPrevHash != 0 &&
                            strcmp(strPrevID, NULL_GUID) != 0)
                        {
                            int intPrevLen   = 0;
                            uint8_t *byPrev  = load_block(strWorkDir_a, strPrevID, &intPrevLen);
                            if (byPrev)
                            {
                                // Skip prev hash check if prev is truncation (matches C#)
                                int blnPrevIsTrunc = (intPrevLen >= HEADER_RAW_SIZE &&
                                                      byPrev[RAW_OFF_BLOCK_TYPE] == BLOCK_TYPE_TRUNCATION);
                                if (!blnPrevIsTrunc)
                                {
                                    uint32_t intCalcPrev = hash_bytes((int)byHashType,
                                                                       byPrev, 0, intPrevLen);
                                    blnPrevHashOK = (intCalcPrev == intStoredPrevHash);
                                }
                                free(byPrev);
                            }
                        }

                        printf("Hash Type:    %d\n", byHashType);
                        printf("Hash:         0x%08X %s\n", intStoredHash,
                               blnHashOK ? "(OK)" : "(FAIL)");
                        printf("PrevHash:     0x%08X %s\n", intStoredPrevHash,
                               blnPrevHashOK ? "(OK)" : "(FAIL)");
                        printf("Payload Len:  %u\n", intPayloadLen);
                        printf("Padded Len:   %u\n", intPaddedLen);

                        if (!blnHashOK || !blnPrevHashOK)
                        {
                            fprintf(stderr, "\n!!! INTEGRITY FAILURE !!!\n");
                            intResult = 1;
                        }
                        else
                        {
                            printf("\n--- Payload (%u bytes) ---\n", intPayloadLen);
                            if (intDecLen >= (int)(ENC_OFF_PAYLOAD + intPayloadLen))
                            {
                                fwrite(byDecoded + ENC_OFF_PAYLOAD, 1, intPayloadLen, stdout);
                            }
                            printf("\n--- End Payload ---\n");
                        }
                    }
                }
            }
        }
    }

    if (byBlock)      { free(byBlock); }
    if (byRollingRom) { free(byRollingRom); }
    if (byDecoded)    { free(byDecoded); }

    return intResult;
}