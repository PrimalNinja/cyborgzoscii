// Cyborg ZTB Verify v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Verifies a block or walks back verifying the whole chain.
// Matches ZTBChain.Verify exactly.
//
// Usage: ztbverify <workdir> <tip_block_id> [-walk]

#include "ztbcommon.c"

// Verify a single block. Returns 1 if valid, 0 if not.
static int verify_block(const char *strWorkDir_a, const char *strBlockID_a)
{
    int intValid      = 0;
    uint8_t *byBlock  = NULL;
    uint8_t *byRollingRom = NULL;
    uint8_t *byDecoded    = NULL;

    int intBlockLen = 0;
    byBlock = load_block(strWorkDir_a, strBlockID_a, &intBlockLen);

    if (byBlock && intBlockLen >= HEADER_RAW_SIZE)
    {
        uint8_t byBlockType = byBlock[RAW_OFF_BLOCK_TYPE];
        char strPrevID[GUID_LEN];
        read_fixed_string(byBlock, RAW_OFF_PREV_ID, 36, strPrevID);

        // Truncation block is not encoded — treat as valid (matches C# FetchBlock)
        if (byBlockType == BLOCK_TYPE_TRUNCATION)
        {
            intValid = 1;
        }
        else
        {
            byRollingRom = build_rolling_rom(strWorkDir_a, strPrevID);
            if (byRollingRom)
            {
                int intEncLen = intBlockLen - HEADER_RAW_SIZE;
                int intDecLen = 0;
                byDecoded     = zoscii_decode(byRollingRom, byBlock,
                                              HEADER_RAW_SIZE, intEncLen, &intDecLen);
                if (byDecoded && intDecLen >= ENC_HEADER_SIZE)
                {
                    uint8_t byHashType     = byDecoded[ENC_OFF_HASH_TYPE];
                    uint32_t intStoredHash = (uint32_t)(byDecoded[ENC_OFF_HASH] |
                                            (byDecoded[ENC_OFF_HASH + 1] << 8)  |
                                            (byDecoded[ENC_OFF_HASH + 2] << 16) |
                                            (byDecoded[ENC_OFF_HASH + 3] << 24));
                    uint32_t intStoredPrevHash = (uint32_t)(byDecoded[ENC_OFF_PREV_HASH] |
                                                (byDecoded[ENC_OFF_PREV_HASH + 1] << 8)  |
                                                (byDecoded[ENC_OFF_PREV_HASH + 2] << 16) |
                                                (byDecoded[ENC_OFF_PREV_HASH + 3] << 24));

                    // Current-block hash covers the payload ONLY (see ztbaddblock.c)
                    int intPaddedPayloadLen = intDecLen - ENC_OFF_PAYLOAD;
                    uint32_t intCalcHash = hash_bytes((int)byHashType,
                                                      byDecoded + ENC_OFF_PAYLOAD,
                                                      0, intPaddedPayloadLen);
                    int blnHashOK = (intCalcHash == intStoredHash);

                    int blnPrevHashOK = 1;
                    if (intStoredPrevHash != 0 && strcmp(strPrevID, NULL_GUID) != 0)
                    {
                        int intPrevLen  = 0;
                        uint8_t *byPrev = load_block(strWorkDir_a, strPrevID, &intPrevLen);
                        if (byPrev)
                        {
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

                    if (blnHashOK && blnPrevHashOK) { intValid = 1; }
                }
            }
        }
    }

    if (byBlock)      { free(byBlock); }
    if (byRollingRom) { free(byRollingRom); }
    if (byDecoded)    { free(byDecoded); }

    return intValid;
}

int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Verify v20260618\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Usage: %s <workdir> <tip_block_id> [-walk]\n", argv[0]);
        fprintf(stderr, "  -walk   Walk back through the chain verifying all blocks\n");
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strWorkDir_a = argv[1];
        const char *strTipID_a   = argv[2];
        int blnWalk              = (argc == 4 && strcmp(argv[3], "-walk") == 0);

        int intVerified = 0;
        int intFailed   = 0;

        // Walk matches ZTBChain.Verify: start at tip, follow prev_block_id
        char strCurrentID[GUID_LEN];
        strncpy(strCurrentID, strTipID_a, GUID_LEN - 1);
        strCurrentID[GUID_LEN - 1] = '\0';

        while (strcmp(strCurrentID, NULL_GUID) != 0 && strlen(strCurrentID) > 0)
        {
            printf("  Verifying %s... ", strCurrentID);

            int blnOK = verify_block(strWorkDir_a, strCurrentID);
            if (blnOK)
            {
                printf("[PASS]\n");
                intVerified++;
            }
            else
            {
                printf("[FAIL]\n");
                intFailed++;
                // Matches C#: stop on first failure
                strcpy(strCurrentID, NULL_GUID);
            }

            if (blnOK)
            {
                if (!blnWalk)
                {
                    // Single block only
                    strcpy(strCurrentID, NULL_GUID);
                }
                else
                {
                    // Load block to get prev_block_id for next iteration
                    int intBlockLen = 0;
                    uint8_t *byBlock = load_block(strWorkDir_a, strCurrentID, &intBlockLen);
                    if (byBlock && intBlockLen >= HEADER_RAW_SIZE)
                    {
                        uint8_t byBlockType = byBlock[RAW_OFF_BLOCK_TYPE];
                        read_fixed_string(byBlock, RAW_OFF_PREV_ID, 36, strCurrentID);
                        free(byBlock);
                        // Stop at truncation block (matches C# Verify)
                        if (byBlockType == BLOCK_TYPE_TRUNCATION)
                        {
                            strcpy(strCurrentID, NULL_GUID);
                        }
                    }
                    else
                    {
                        if (byBlock) { free(byBlock); }
                        strcpy(strCurrentID, NULL_GUID);
                    }
                }
            }
        }

        printf("\n=== Verify Summary ===\n");
        printf("Verified: %d\n", intVerified);
        printf("Failed:   %d\n", intFailed);

        if (intFailed == 0 && intVerified > 0)
        {
            printf("+++ ALL VERIFICATIONS PASSED +++\n");
        }
        else
        {
            printf("--- VERIFICATION FAILED ---\n");
            intResult = 1;
        }
    }

    return intResult;
}