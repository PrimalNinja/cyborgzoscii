// Cyborg ZTB Common Functions v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include "ztbcommon.h"

#ifndef _WIN32
    #include <dirent.h>
#endif

// --- Global RNG State ---
uint32_t g_rng_state = 0;

// --- Initialize RNG from ROM content (like zencode.c) ---
void init_rng(void)
{
    // Seeded later via init_rng_from_rom() when ROM is available.
    // This fallback is used by ztbcreate before any ROM exists.
    uint32_t intSeed = (uint32_t)time(NULL);
    intSeed ^= (uint32_t)clock();
#ifdef _WIN32
    intSeed ^= (uint32_t)GetCurrentProcessId();
    intSeed ^= (uint32_t)GetTickCount();
#else
    intSeed ^= (uint32_t)getpid();
#endif
    g_rng_state = intSeed;
    int intI;
    for (intI = 0; intI < 10; intI++) { get_random(); }
}

// --- Initialize RNG from ROM data (matches zencode.c buildLookupTable seed) ---
void init_rng_from_rom(const uint8_t *byRomData_a, long lngRomSize_a)
{
    uint32_t intRomHash = 0;
    long intI;
    for (intI = 0; intI < lngRomSize_a; intI++)
    {
        intRomHash = (intRomHash * 33) + byRomData_a[intI];
    }
    intRomHash ^= (uint32_t)time(NULL);
    g_rng_state = intRomHash;
    // Warm up
    int intJ;
    for (intJ = 0; intJ < 10; intJ++) { get_random(); }
}

// --- Simple xorshift32 RNG ---
uint32_t get_random(void)
{
    g_rng_state ^= g_rng_state << 13;
    g_rng_state ^= g_rng_state >> 17;
    g_rng_state ^= g_rng_state << 5;
    return g_rng_state;
}

// --- GUID Generation ---
void generate_guid(char *strBuffer_a)
{
    snprintf(strBuffer_a, GUID_LEN,
             "%08X-%04X-%04X-%04X-%04X%08X",
             get_random(),
             get_random() & 0xFFFF,
             (get_random() & 0x0FFF) | 0x4000,
             (get_random() & 0x3FFF) | 0x8000,
             get_random() & 0xFFFF,
             get_random());
}

// --- CRC32 Checksum ---
uint32_t calculate_checksum(const uint8_t *byData_a, size_t intLen_a)
{
    uint32_t intCrc = 0xFFFFFFFF;
    size_t intI;
    int intJ;
    for (intI = 0; intI < intLen_a; intI++)
    {
        intCrc ^= byData_a[intI];
        for (intJ = 0; intJ < 8; intJ++)
        {
            if (intCrc & 1)
            {
                intCrc = (intCrc >> 1) ^ 0xEDB88320;
            }
            else
            {
                intCrc = intCrc >> 1;
            }
        }
    }
    return intCrc ^ 0xFFFFFFFF;
}

// --- Load ROM File ---
uint8_t* load_rom(const char *strFilename_a)
{
    uint8_t *byRomData = NULL;
    FILE *f = fopen(strFilename_a, "rb");
    if (f)
    {
        byRomData = malloc(ROM_SIZE);
        if (byRomData)
        {
            if (fread(byRomData, 1, ROM_SIZE, f) != ROM_SIZE)
            {
                free(byRomData);
                byRomData = NULL;
            }
        }
        fclose(f);
    }
    return byRomData;
}

// --- Load ROM and seed RNG from its content ---
uint8_t* load_rom_and_seed_rng(const char *strFilename_a)
{
    uint8_t *byRomData = load_rom(strFilename_a);
    if (byRomData)
    {
        init_rng_from_rom(byRomData, ROM_SIZE);
    }
    return byRomData;
}

// --- Block Comparison for Sorting ---
int compare_blocks(const void *varA_a, const void *varB_a)
{
    return ((BlockInfo *)varA_a)->index - ((BlockInfo *)varB_a)->index;
}

// --- Parse a .ztb filename and add to block list if it matches the chain ---
static int parse_ztb_filename(const char *strFilename_a, const char *strChainId_a,
                               BlockInfo *arrList_a, int intCount_a)
{
    int intAdded = 0;
    if (strstr(strFilename_a, strChainId_a) == strFilename_a &&
        strstr(strFilename_a, ".ztb"))
    {
        char *strUnderscore1 = strchr(strFilename_a, '_');
        if (strUnderscore1)
        {
            int intIndex = atoi(strUnderscore1 + 1);
            char *strUnderscore2 = strchr(strUnderscore1 + 1, '_');
            if (strUnderscore2)
            {
                char *strDot = strstr(strUnderscore2 + 1, ".ztb");
                if (strDot)
                {
                    size_t intIdLen = strDot - (strUnderscore2 + 1);
                    if (intIdLen >= GUID_LEN) { intIdLen = GUID_LEN - 1; }
                    strncpy(arrList_a[intCount_a].block_id, strUnderscore2 + 1, intIdLen);
                    arrList_a[intCount_a].block_id[intIdLen] = '\0';
                    arrList_a[intCount_a].index = intIndex;
                    strncpy(arrList_a[intCount_a].filename, strFilename_a, FILENAME_MAX - 1);
                    arrList_a[intCount_a].filename[FILENAME_MAX - 1] = '\0';
                    intAdded = 1;
                }
            }
        }
    }
    return intAdded;
}

// --- Collect unique chain IDs from .ztb filenames, excluding a given chain ---
static int collect_candidate_chain_ids(const char *strExcludeChainId_a,
                                        char arrCandidates_a[][GUID_LEN],
                                        int intMaxCandidates_a)
{
    int intCount = 0;
#ifdef _WIN32
    WIN32_FIND_DATAA objFindData;
    HANDLE hFind = FindFirstFileA("*.ztb", &objFindData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            const char *strName = objFindData.cFileName;
#else
    DIR *d = opendir(".");
    if (d)
    {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL)
        {
            const char *strName = dir->d_name;
#endif
            if (strstr(strName, ".ztb"))
            {
                if (!strExcludeChainId_a || strstr(strName, strExcludeChainId_a) != strName)
                {
                    char *strUnderscore = strchr(strName, '_');
                    if (strUnderscore)
                    {
                        size_t intIdLen = strUnderscore - strName;
                        if (intIdLen < GUID_LEN)
                        {
                            char strCandId[GUID_LEN];
                            strncpy(strCandId, strName, intIdLen);
                            strCandId[intIdLen] = '\0';
                            int intFound = 0;
                            int intI;
                            for (intI = 0; intI < intCount; intI++)
                            {
                                if (strcmp(arrCandidates_a[intI], strCandId) == 0) { intFound = 1; }
                            }
                            if (!intFound && intCount < intMaxCandidates_a)
                            {
                                strcpy(arrCandidates_a[intCount], strCandId);
                                intCount++;
                            }
                        }
                    }
                }
            }
#ifdef _WIN32
        } while (FindNextFileA(hFind, &objFindData) != 0);
        FindClose(hFind);
    }
#else
        }
        closedir(d);
    }
#endif
    return intCount;
}

// --- Scan Chain Blocks ---
int scan_chain_blocks(const char *strChainId_a, BlockInfo *arrList_a, int intMaxBlocks_a)
{
    int intCount = 0;
#ifdef _WIN32
    WIN32_FIND_DATAA objFindData;
    HANDLE hFind = FindFirstFileA("*.ztb", &objFindData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (intCount < intMaxBlocks_a)
            {
                if (parse_ztb_filename(objFindData.cFileName, strChainId_a, arrList_a, intCount))
                {
                    intCount++;
                }
            }
        } while (FindNextFileA(hFind, &objFindData) != 0);
        FindClose(hFind);
    }
#else
    DIR *d = opendir(".");
    if (d)
    {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL && intCount < intMaxBlocks_a)
        {
            if (parse_ztb_filename(dir->d_name, strChainId_a, arrList_a, intCount))
            {
                intCount++;
            }
        }
        closedir(d);
    }
#endif
    if (intCount > 0)
    {
        qsort(arrList_a, intCount, sizeof(BlockInfo), compare_blocks);
    }
    return intCount;
}

// --- ZOSCII Encode Entire Block ---
uint8_t* zoscii_encode_block(const uint8_t *byRomData_a, const uint8_t *byRawBlock_a,
                             size_t intRawLen_a, size_t *intEncodedLen_a)
{
    uint8_t *byEncoded = NULL;
    int intI;
    size_t intIdx;
    uint32_t arrCounts[256] = {0};
    for (intIdx = 0; intIdx < ROM_SIZE; intIdx++) { arrCounts[byRomData_a[intIdx]]++; }

    uint32_t **arrLookup = malloc(256 * sizeof(uint32_t*));
    if (arrLookup)
    {
        for (intI = 0; intI < 256; intI++)
        {
            arrLookup[intI] = malloc(arrCounts[intI] * sizeof(uint32_t));
            arrCounts[intI] = 0;
        }
        for (intIdx = 0; intIdx < ROM_SIZE; intIdx++)
        {
            uint8_t byVal = byRomData_a[intIdx];
            arrLookup[byVal][arrCounts[byVal]++] = intIdx;
        }

        *intEncodedLen_a = intRawLen_a * 2;
        byEncoded = malloc(*intEncodedLen_a);
        if (byEncoded)
        {
            int intEncodeFailed = 0;
            for (intIdx = 0; intIdx < intRawLen_a && !intEncodeFailed; intIdx++)
            {
                uint8_t byVal = byRawBlock_a[intIdx];
                if (arrCounts[byVal] == 0)
                {
                    fprintf(stderr, "Error: Byte 0x%02X not in ROM\n", byVal);
                    intEncodeFailed = 1;
                }
                else
                {
                    uint32_t intRandIdx = get_random() % arrCounts[byVal];
                    uint32_t intAddr = arrLookup[byVal][intRandIdx];
                    uint16_t intPtr = (uint16_t)intAddr;
                    byEncoded[intIdx * 2] = intPtr & 0xFF;
                    byEncoded[intIdx * 2 + 1] = (intPtr >> 8) & 0xFF;
                }
            }
            if (intEncodeFailed) { free(byEncoded); byEncoded = NULL; }
        }
        for (intI = 0; intI < 256; intI++) { free(arrLookup[intI]); }
        free(arrLookup);
    }
    return byEncoded;
}

// --- ZOSCII Decode Entire Block ---
uint8_t* zoscii_decode_block(const uint8_t *byRomData_a, const uint8_t *byEncodedBlock_a,
                             size_t intEncodedLen_a, size_t *intDecodedLen_a)
{
    uint8_t *byDecoded = NULL;
    *intDecodedLen_a = intEncodedLen_a / 2;
    byDecoded = malloc(*intDecodedLen_a);
    if (byDecoded)
    {
        int intDecodeFailed = 0;
        size_t intI;
        for (intI = 0; intI < *intDecodedLen_a && !intDecodeFailed; intI++)
        {
            uint16_t intPtr = byEncodedBlock_a[intI * 2] | (byEncodedBlock_a[intI * 2 + 1] << 8);
            if (intPtr >= ROM_SIZE)
            {
                fprintf(stderr, "Error: Pointer %u out of bounds\n", intPtr);
                intDecodeFailed = 1;
            }
            else
            {
                byDecoded[intI] = byRomData_a[intPtr];
            }
        }
        if (intDecodeFailed) { free(byDecoded); byDecoded = NULL; }
    }
    return byDecoded;
}

// --- Build Rolling ROM ---
int build_rolling_rom(const char *strGenesisRomFile_a,
                      const BlockInfo *arrChainHistory_a, int intChainCount_a,
                      const BlockInfo *arrTrunkHistory_a, int intTrunkCount_a,
                      int intTargetIndex_a, uint8_t *byRollingRom_a)
{
    uint8_t *byGenesisRom = load_rom(strGenesisRomFile_a);
    if (!byGenesisRom) { fprintf(stderr, "Error: Cannot load genesis ROM\n"); return 0; }

    size_t intBytesCopied = 0;
    int intSamplesTaken = 0;

    if (intChainCount_a > 0)
    {
        int intAvailable = 0;
        int intI;
        for (intI = 0; intI < intChainCount_a; intI++)
        {
            if (arrChainHistory_a[intI].index < intTargetIndex_a) { intAvailable++; }
        }
        int intToSample = (intAvailable < MAX_HISTORY_BLOCKS) ? intAvailable : MAX_HISTORY_BLOCKS;
        int intStartIdx = intAvailable - intToSample;
        int intCurrent = 0;

        for (intI = 0; intI < intChainCount_a; intI++)
        {
            if (arrChainHistory_a[intI].index >= intTargetIndex_a) { intCurrent++; }
            else if (intCurrent < intStartIdx) { intCurrent++; }
            else if (intBytesCopied + ROM_ENTRY_SIZE > ROM_SIZE) { break; }
            else if (intSamplesTaken >= MAX_HISTORY_BLOCKS) { break; }
            else
            {
                FILE *f = fopen(arrChainHistory_a[intI].filename, "rb");
                if (f)
                {
                    uint8_t byEncodedSample[ROM_ENTRY_SIZE];
                    size_t intReadBytes = fread(byEncodedSample, 1, ROM_ENTRY_SIZE, f);
                    if (intReadBytes >= ROM_ENTRY_SIZE)
                    {
                        memcpy(byRollingRom_a + intBytesCopied, byEncodedSample, ROM_ENTRY_SIZE);
                        intBytesCopied += ROM_ENTRY_SIZE;
                        intSamplesTaken++;
                    }
                    fclose(f);
                }
                intCurrent++;
            }
        }
    }

    if (intSamplesTaken < MAX_HISTORY_BLOCKS && intTrunkCount_a > 0)
    {
        int intRemaining = MAX_HISTORY_BLOCKS - intSamplesTaken;
        int intTrunkSamples = (intTrunkCount_a < intRemaining) ? intTrunkCount_a : intRemaining;
        int intStartIdx = intTrunkCount_a - intTrunkSamples;
        int intI;
        for (intI = intStartIdx; intI < intTrunkCount_a; intI++)
        {
            if (intBytesCopied + ROM_ENTRY_SIZE > ROM_SIZE) { break; }
            else if (intSamplesTaken >= MAX_HISTORY_BLOCKS) { break; }
            else
            {
                FILE *f = fopen(arrTrunkHistory_a[intI].filename, "rb");
                if (f)
                {
                    uint8_t byEncodedSample[ROM_ENTRY_SIZE];
                    size_t intReadBytes = fread(byEncodedSample, 1, ROM_ENTRY_SIZE, f);
                    if (intReadBytes >= ROM_ENTRY_SIZE)
                    {
                        memcpy(byRollingRom_a + intBytesCopied, byEncodedSample, ROM_ENTRY_SIZE);
                        intBytesCopied += ROM_ENTRY_SIZE;
                        intSamplesTaken++;
                    }
                    fclose(f);
                }
            }
        }
    }

    if (intBytesCopied < ROM_SIZE)
    {
        memcpy(byRollingRom_a + intBytesCopied, byGenesisRom, ROM_SIZE - intBytesCopied);
    }

    free(byGenesisRom);
    return 1;
}

// --- Try to decode block 1 with a given ROM and verify CRC32 ---
static int try_decode_block1(const uint8_t *byRom_a, const char *strFilename_a,
                              ZTB_BlockHeader *objHeaderOut_a)
{
    int intValid = 0;
    FILE *f = fopen(strFilename_a, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t intFileLen = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (intFileLen > CRC32_PREFIX_SIZE)
        {
            uint8_t arrStoredCrc[CRC32_PREFIX_SIZE];
            if (fread(arrStoredCrc, 1, CRC32_PREFIX_SIZE, f) == CRC32_PREFIX_SIZE)
            {
                uint32_t intStoredCrc = arrStoredCrc[0] | (arrStoredCrc[1] << 8) |
                                        (arrStoredCrc[2] << 16) | (arrStoredCrc[3] << 24);

                size_t intEncodedLen = intFileLen - CRC32_PREFIX_SIZE;
                uint8_t *byEncoded = malloc(intEncodedLen);
                if (byEncoded)
                {
                    if (fread(byEncoded, 1, intEncodedLen, f) == intEncodedLen)
                    {
                        uint32_t intCalcCrc = calculate_checksum(byEncoded, intEncodedLen);
                        if (intCalcCrc == intStoredCrc)
                        {
                            size_t intDecodedLen;
                            uint8_t *byDecoded = zoscii_decode_block(byRom_a, byEncoded,
                                                                      intEncodedLen, &intDecodedLen);
                            if (byDecoded && intDecodedLen >= sizeof(ZTB_BlockHeader))
                            {
                                memcpy(objHeaderOut_a, byDecoded, sizeof(ZTB_BlockHeader));
                                intValid = 1;
                                free(byDecoded);
                            }
                        }
                    }
                    free(byEncoded);
                }
            }
        }
        fclose(f);
    }
    return intValid;
}

// --- Detect Branch Status ---
int detect_branch_status(const char *strGenesisRomFile_a,
                        const BlockInfo *arrChainHistory_a, int intChainCount_a,
                        const char *strChainId_a, char *strTrunkIdOut_a)
{
    int intIsBranch = 0;
    strcpy(strTrunkIdOut_a, NULL_GUID);
    if (intChainCount_a == 0) { return 0; }

    // Try decoding block 1 with pure genesis ROM (works if this is a trunk)
    {
        uint8_t *byGenesisRom = load_rom(strGenesisRomFile_a);
        if (byGenesisRom)
        {
            ZTB_BlockHeader objHeader;
            if (try_decode_block1(byGenesisRom, arrChainHistory_a[0].filename, &objHeader))
            {
                if (objHeader.is_branch == 1)
                {
                    intIsBranch = 1;
                    strncpy(strTrunkIdOut_a, objHeader.trunk_id, GUID_LEN - 1);
                    strTrunkIdOut_a[GUID_LEN - 1] = '\0';
                }
            }
            free(byGenesisRom);
        }
    }

    // If genesis decode didn't confirm branch, try candidate trunks
    if (!intIsBranch)
    {
        char arrCandidates[100][GUID_LEN];
        int intCandidateCount = collect_candidate_chain_ids(strChainId_a, arrCandidates, 100);
        int intC;
        for (intC = 0; intC < intCandidateCount && !intIsBranch; intC++)
        {
            BlockInfo *arrCandTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (arrCandTrunkHistory)
            {
                int intCandTrunkCount = scan_chain_blocks(arrCandidates[intC],
                                                          arrCandTrunkHistory, MAX_BLOCKS_TO_SCAN);
                if (intCandTrunkCount > 0)
                {
                    uint8_t *byRollingRom = malloc(ROM_SIZE);
                    if (byRollingRom)
                    {
                        if (build_rolling_rom(strGenesisRomFile_a, NULL, 0,
                                              arrCandTrunkHistory, intCandTrunkCount, 1, byRollingRom))
                        {
                            ZTB_BlockHeader objHeader;
                            if (try_decode_block1(byRollingRom, arrChainHistory_a[0].filename, &objHeader))
                            {
                                if (objHeader.is_branch == 1 &&
                                    strcmp(objHeader.trunk_id, arrCandidates[intC]) == 0)
                                {
                                    intIsBranch = 1;
                                    strncpy(strTrunkIdOut_a, objHeader.trunk_id, GUID_LEN - 1);
                                    strTrunkIdOut_a[GUID_LEN - 1] = '\0';
                                }
                            }
                        }
                        free(byRollingRom);
                    }
                }
                free(arrCandTrunkHistory);
            }
        }
    }

    return intIsBranch;
}

// --- Discover Branches from Trunk ---
int discover_branches_from_trunk(const char *strGenesisRomFile_a, const char *strTrunkId_a,
                                 BranchInfo *arrBranches_a, int intMaxBranches_a)
{
    int intBranchCount = 0;

    char arrCandidates[100][GUID_LEN];
    int intCandidateCount = collect_candidate_chain_ids(strTrunkId_a, arrCandidates, 100);

    if (intCandidateCount > 0)
    {
        BlockInfo *arrTrunkHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
        int intTrunkCount = 0;
        if (arrTrunkHistory)
        {
            intTrunkCount = scan_chain_blocks(strTrunkId_a, arrTrunkHistory, MAX_BLOCKS_TO_SCAN);
        }

        int intC;
        for (intC = 0; intC < intCandidateCount && intBranchCount < intMaxBranches_a; intC++)
        {
            BlockInfo *arrCandHistory = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
            if (arrCandHistory)
            {
                int intCandCount = scan_chain_blocks(arrCandidates[intC],
                                                     arrCandHistory, MAX_BLOCKS_TO_SCAN);
                if (intCandCount > 0)
                {
                    uint8_t *byRollingRom = malloc(ROM_SIZE);
                    if (byRollingRom)
                    {
                        if (build_rolling_rom(strGenesisRomFile_a, NULL, 0,
                                              arrTrunkHistory, intTrunkCount, 1, byRollingRom))
                        {
                            ZTB_BlockHeader objHeader;
                            if (try_decode_block1(byRollingRom, arrCandHistory[0].filename, &objHeader))
                            {
                                if (objHeader.is_branch == 1 &&
                                    strcmp(objHeader.trunk_id, strTrunkId_a) == 0)
                                {
                                    strncpy(arrBranches_a[intBranchCount].branch_id,
                                            arrCandidates[intC], GUID_LEN - 1);
                                    arrBranches_a[intBranchCount].branch_id[GUID_LEN - 1] = '\0';
                                    intBranchCount++;
                                }
                            }
                        }
                        free(byRollingRom);
                    }
                }
                free(arrCandHistory);
            }
        }

        if (arrTrunkHistory) { free(arrTrunkHistory); }
    }

    return intBranchCount;
}