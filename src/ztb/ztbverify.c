// Cyborg ZTB Chain Verifier v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
//
// Verifies the integrity of a ZTB chain (trunk or branch).
// Usage: ztbverify <genesis_rom> <trunk_id>           - Verify trunk and all branches
// Usage: ztbverify <genesis_rom> <trunk_id> -t        - Verify trunk only
// Usage: ztbverify <genesis_rom> <trunk_id> -b <branch_id> - Verify specific branch only
// Usage: ztbverify <genesis_rom> <trunk_id> -bb       - Verify all branches only (not trunk)

#include "ztbcommon.c"

// --- Verification Statistics ---
typedef struct {
    int total_blocks;
    int verified_blocks;
    int failed_blocks;
    int branches_found;
} VerifyStats;

// --- Verify Single Block ---
int verify_single_block(const char *strGenesisRomFile_a,
                        const BlockInfo *arrChainHistory_a, int intChainCount_a,
                        const BlockInfo *arrTrunkHistory_a, int intTrunkCount_a,
                        int intTargetIndex_a)
{
    int intValid = 0;
    uint8_t *byRollingRom = NULL;
    uint8_t *byEncoded = NULL;
    uint8_t *byDecoded = NULL;

    BlockInfo *objTarget = NULL;
    int intI;

    for (intI = 0; intI < intChainCount_a; intI++)
    {
        if (arrChainHistory_a[intI].index == intTargetIndex_a)
        {
            objTarget = (BlockInfo *)&arrChainHistory_a[intI];
        }
    }

    if (objTarget)
    {
        byRollingRom = malloc(ROM_SIZE);
        if (byRollingRom)
        {
            if (build_rolling_rom(strGenesisRomFile_a,
                                  arrChainHistory_a, intChainCount_a,
                                  arrTrunkHistory_a, intTrunkCount_a,
                                  intTargetIndex_a, byRollingRom))
            {
                FILE *f = fopen(objTarget->filename, "rb");
                if (f)
                {
                    fseek(f, 0, SEEK_END);
                    size_t intEncodedLen = ftell(f);
                    fseek(f, 0, SEEK_SET);

                    byEncoded = malloc(intEncodedLen);
                    if (byEncoded)
                    {
                        if (fread(byEncoded, 1, intEncodedLen, f) == intEncodedLen)
                        {
                            size_t intDecodedLen;
                            byDecoded = zoscii_decode_block(byRollingRom, byEncoded,
                                                            intEncodedLen, &intDecodedLen);

                            if (byDecoded && intDecodedLen >= sizeof(ZTB_BlockHeader))
                            {
                                ZTB_BlockHeader *objHeader = (ZTB_BlockHeader*)byDecoded;
                                size_t intPayloadSpace = intDecodedLen - sizeof(ZTB_BlockHeader);

                                if (objHeader->padded_len <= intPayloadSpace)
                                {
                                    uint8_t *byPayload = byDecoded + sizeof(ZTB_BlockHeader);
                                    uint32_t intCalcChecksum = calculate_checksum(byPayload, objHeader->padded_len);
                                    intValid = (intCalcChecksum == objHeader->checksum) ? 1 : 0;
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
        }
    }

    if (byRollingRom) { free(byRollingRom); }
    if (byEncoded) { free(byEncoded); }
    if (byDecoded) { free(byDecoded); }

    return intValid;
}

// --- Verify Chain BACKWARDS ---
int verify_chain_backwards(const char *strGenesisRomFile_a, const char *strChainId_a,
                           const char *strTrunkId_a, VerifyStats *objStats_a)
{
    int intSuccess = 0;
    BlockInfo *arrHistory = NULL;
    BlockInfo *arrTrunkHistory = NULL;

    arrHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
    if (arrHistory)
    {
        int intBlockCount = scan_chain_blocks(strChainId_a, arrHistory, MAX_BLOCKS_TO_SCAN);

        if (intBlockCount > 0)
        {
            int intTrunkCount = 0;

            if (strTrunkId_a && strcmp(strTrunkId_a, NULL_GUID) != 0)
            {
                arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
                if (arrTrunkHistory)
                {
                    intTrunkCount = scan_chain_blocks(strTrunkId_a, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);
                }
            }

            intSuccess = 1;
            int intI;

            for (intI = intBlockCount - 1; intI >= 0; intI--)
            {
                printf("  Block %d (%s)... ", arrHistory[intI].index, arrHistory[intI].block_id);

                if (verify_single_block(strGenesisRomFile_a, arrHistory, intBlockCount,
                                        arrTrunkHistory, intTrunkCount, arrHistory[intI].index))
                {
                    printf("[PASS]\n");
                    objStats_a->verified_blocks++;
                }
                else
                {
                    printf("[FAIL]\n");
                    objStats_a->failed_blocks++;
                    intSuccess = 0;
                }

                objStats_a->total_blocks++;
            }
        }
    }

    if (arrHistory) { free(arrHistory); }
    if (arrTrunkHistory) { free(arrTrunkHistory); }

    return intSuccess;
}

// --- Main ---
int main(int argc, char *argv[])
{
    int intResult = 0;

    printf("ZTB Chain Verifier v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc < 3 || argc > 5)
    {
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id>                - Verify trunk and all branches\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -t             - Verify trunk only\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -b <branch_id> - Verify specific branch only\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -bb            - Verify all branches only (not trunk)\n", argv[0]);
        intResult = 1;
    }

    if (intResult == 0)
    {
        const char *strGenesisRomFile_a = argv[1];
        const char *strTrunkId_a = argv[2];

        int intVerifyTrunk = 1;
        int intVerifyBranches = 1;
        char strSpecificBranch[GUID_LEN] = "";

        if (argc >= 4)
        {
            if (strcmp(argv[3], "-t") == 0)
            {
                intVerifyTrunk = 1;
                intVerifyBranches = 0;
            }
            else if (strcmp(argv[3], "-bb") == 0)
            {
                intVerifyTrunk = 0;
                intVerifyBranches = 1;
            }
            else if (strcmp(argv[3], "-b") == 0)
            {
                if (argc != 5)
                {
                    fprintf(stderr, "Error: -b requires a branch_id argument\n");
                    intResult = 1;
                }
                else
                {
                    intVerifyTrunk = 0;
                    intVerifyBranches = 0;
                    strncpy(strSpecificBranch, argv[4], GUID_LEN - 1);
                    strSpecificBranch[GUID_LEN - 1] = '\0';
                }
            }
            else
            {
                fprintf(stderr, "Error: Unknown option '%s'\n", argv[3]);
                intResult = 1;
            }
        }

        if (intResult == 0)
        {
            VerifyStats objStats = {0, 0, 0, 0};

            if (intVerifyTrunk)
            {
                printf("=== Verifying Trunk: %s ===\n", strTrunkId_a);
                if (!verify_chain_backwards(strGenesisRomFile_a, strTrunkId_a, NULL, &objStats))
                {
                    printf("\n- TRUNK VERIFICATION FAILED\n");
                    intResult = 1;
                }
                else
                {
                    printf("+ Trunk verified\n\n");
                }
            }

            BranchInfo arrBranches[100];
            int intBranchCount = 0;

            if (intResult == 0 && (intVerifyBranches || strSpecificBranch[0] != '\0'))
            {
                intBranchCount = discover_branches_from_trunk(strGenesisRomFile_a, strTrunkId_a,
                                                              arrBranches, 100);
                objStats.branches_found = intBranchCount;
            }

            if (intResult == 0 && strSpecificBranch[0] != '\0')
            {
                int intFound = 0;
                int intI;
                for (intI = 0; intI < intBranchCount; intI++)
                {
                    if (strcmp(arrBranches[intI].branch_id, strSpecificBranch) == 0)
                    {
                        intFound = 1;
                    }
                }

                if (!intFound)
                {
                    fprintf(stderr, "Error: Branch '%s' not found or not linked to trunk '%s'\n",
                            strSpecificBranch, strTrunkId_a);
                    intResult = 1;
                }
                else
                {
                    printf("=== Verifying Branch: %s ===\n", strSpecificBranch);
                    if (!verify_chain_backwards(strGenesisRomFile_a, strSpecificBranch, strTrunkId_a, &objStats))
                    {
                        printf("\n- BRANCH VERIFICATION FAILED\n");
                        intResult = 1;
                    }
                    else
                    {
                        printf("+ Branch verified\n\n");
                    }
                }
            }
            else if (intResult == 0 && intVerifyBranches && intBranchCount > 0)
            {
                printf("=== Found %d branch(es) ===\n\n", intBranchCount);

                int intI;
                for (intI = 0; intI < intBranchCount && intResult == 0; intI++)
                {
                    printf("--- Verifying Branch: %s ---\n", arrBranches[intI].branch_id);

                    if (!verify_chain_backwards(strGenesisRomFile_a, arrBranches[intI].branch_id, strTrunkId_a, &objStats))
                    {
                        printf("\n- BRANCH VERIFICATION FAILED\n");
                        intResult = 1;
                    }
                    else
                    {
                        printf("+ Branch verified\n\n");
                    }
                }
            }
            else if (intResult == 0 && intVerifyBranches && intBranchCount == 0)
            {
                printf("=== No branches found ===\n\n");
            }

            printf("=== Verification Summary ===\n");
            printf("Total blocks verified: %d\n", objStats.verified_blocks);
            printf("Failed verifications:  %d\n", objStats.failed_blocks);
            if (intVerifyBranches || strSpecificBranch[0] != '\0')
            {
                printf("Branches found:        %d\n", objStats.branches_found);
            }

            if (objStats.failed_blocks == 0 && intResult == 0)
            {
                printf("\n+++ ALL VERIFICATIONS PASSED +++\n");
            }
            else
            {
                printf("\n- VERIFICATION FAILED\n");
                intResult = 1;
            }
        }
    }

    return intResult;
}