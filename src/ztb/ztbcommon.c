// Cyborg ZTB Common Functions v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include "ztbcommon.h"

// --- CRC32 (matches clsZTB.CalculateCRC32) ---
uint32_t calculate_crc32(const uint8_t *byData_a, int intOffset_a, int intLen_a)
{
    uint32_t intCrc = 0xFFFFFFFF;
    int intI        = intOffset_a;
    int intEnd      = intOffset_a + intLen_a;

    while (intI < intEnd)
    {
        intCrc ^= byData_a[intI];
        int intJ = 0;
        while (intJ < 8)
        {
            if (intCrc & 1) { intCrc = (intCrc >> 1) ^ 0xEDB88320; }
            else            { intCrc =  intCrc >> 1; }
            intJ++;
        }
        intI++;
    }

    return intCrc ^ 0xFFFFFFFF;
}

// --- XorShift32 (matches clsZTB.XorShift32) ---
uint32_t xorshift32(uint32_t intState_a)
{
    uint32_t intX = intState_a;
    intX ^= intX << 13;
    intX ^= intX >> 17;
    intX ^= intX << 5;
    return intX;
}

// --- Hash bytes (matches clsZTB.HashBytes) ---
// Only CRC32 variants implemented; rolling hash is a stub returning 0.
uint32_t hash_bytes(int intHashType_a, const uint8_t *byData_a, int intOffset_a,
                    int intLen_a)
{
    uint32_t intResult = 0;
    int intHashLen     = intLen_a;

    if (intHashType_a == HASH_TYPE_CRC32_1KB || intHashType_a == HASH_TYPE_ROLL_1KB)
    {
        if (intHashLen > 1024) { intHashLen = 1024; }
    }

    if (intHashType_a == HASH_TYPE_CRC32_FULL || intHashType_a == HASH_TYPE_CRC32_1KB)
    {
        intResult = calculate_crc32(byData_a, intOffset_a, intHashLen);
    }
    // HASH_TYPE_ROLL_FULL / HASH_TYPE_ROLL_1KB: stub, returns 0

    return intResult;
}

// --- Read fixed-length ASCII string from byte array ---
void read_fixed_string(const uint8_t *byData_a, int intOffset_a, int intLen_a,
                       char *strOut_a)
{
    int intEnd = intOffset_a;
    while (intEnd < intOffset_a + intLen_a && byData_a[intEnd] != 0)
    {
        intEnd++;
    }
    int intCopy = intEnd - intOffset_a;
    memcpy(strOut_a, byData_a + intOffset_a, intCopy);
    strOut_a[intCopy] = '\0';
}

// --- Write fixed-length ASCII string into byte array (zero-padded) ---
void write_fixed_string(uint8_t *byData_a, int intOffset_a, int intLen_a,
                        const char *strValue_a)
{
    int intSrcLen = strValue_a ? (int)strlen(strValue_a) : 0;
    int intCopy   = intSrcLen < intLen_a ? intSrcLen : intLen_a;
    if (intCopy > 0) { memcpy(byData_a + intOffset_a, strValue_a, intCopy); }
    int intI = intOffset_a + intCopy;
    while (intI < intOffset_a + intLen_a)
    {
        byData_a[intI] = 0;
        intI++;
    }
}

// --- ZOSCII encode (matches ZEncode.Bytes) ---
// Each raw byte is replaced by a 2-byte little-endian ROM address whose value equals that byte.
// A random address is chosen among all addresses in the ROM that hold that value.
// The RNG seed used here mirrors the C# pattern: seeded from time millis per call.
uint8_t* zoscii_encode(const uint8_t *byRom_a, const uint8_t *byData_a,
                       int intLen_a, int *intEncodedLen_a)
{
    *intEncodedLen_a = 0;

    // Build lookup: for each byte value, collect all ROM addresses that hold it
    int arrCounts[256];
    memset(arrCounts, 0, sizeof(arrCounts));

    int intIdx;
    for (intIdx = 0; intIdx < ROM_SIZE; intIdx++)
    {
        arrCounts[byRom_a[intIdx]]++;
    }

    uint32_t *arrLookup[256];
    int intV;
    for (intV = 0; intV < 256; intV++)
    {
        arrLookup[intV] = NULL;
        if (arrCounts[intV] > 0)
        {
            arrLookup[intV] = (uint32_t*)malloc(arrCounts[intV] * sizeof(uint32_t));
            if (!arrLookup[intV])
            {
                int intFree;
                for (intFree = 0; intFree < intV; intFree++)
                {
                    if (arrLookup[intFree]) { free(arrLookup[intFree]); }
                }
                return NULL;
            }
            arrCounts[intV] = 0;
        }
    }

    for (intIdx = 0; intIdx < ROM_SIZE; intIdx++)
    {
        uint8_t byVal = byRom_a[intIdx];
        arrLookup[byVal][arrCounts[byVal]++] = (uint32_t)intIdx;
    }

    int intOutLen  = intLen_a * 2;
    uint8_t *byOut = (uint8_t*)malloc(intOutLen);

    if (!byOut)
    {
        for (intV = 0; intV < 256; intV++) { if (arrLookup[intV]) { free(arrLookup[intV]); } }
        return NULL;
    }

    // Seed xorshift from current time (matches C# DateTimeOffset.UtcNow millis seed)
    uint32_t intSeed = (uint32_t)time(NULL);
    intSeed = xorshift32(intSeed);

    int intFailed = 0;
    int intI;
    for (intI = 0; intI < intLen_a && !intFailed; intI++)
    {
        uint8_t byVal = byData_a[intI];
        if (arrCounts[byVal] == 0)
        {
            fprintf(stderr, "Error: byte 0x%02X not in ROM\n", byVal);
            intFailed = 1;
        }
        else
        {
            intSeed = xorshift32(intSeed);
            uint32_t intRandIdx  = intSeed % (uint32_t)arrCounts[byVal];
            uint32_t intAddr     = arrLookup[byVal][intRandIdx];
            byOut[intI * 2]     = (uint8_t)(intAddr & 0xFF);
            byOut[intI * 2 + 1] = (uint8_t)((intAddr >> 8) & 0xFF);
        }
    }

    for (intV = 0; intV < 256; intV++) { if (arrLookup[intV]) { free(arrLookup[intV]); } }

    if (intFailed)
    {
        free(byOut);
        return NULL;
    }

    *intEncodedLen_a = intOutLen;
    return byOut;
}

// --- ZOSCII decode (matches ZDecode.Bytes) ---
// Each 2-byte little-endian address is looked up in the ROM to recover the original byte.
uint8_t* zoscii_decode(const uint8_t *byRom_a, const uint8_t *byData_a,
                       int intOffset_a, int intLen_a, int *intDecodedLen_a)
{
    *intDecodedLen_a = intLen_a / 2;
    uint8_t *byOut   = (uint8_t*)malloc(*intDecodedLen_a);

    if (!byOut) { return NULL; }

    int intFailed = 0;
    int intI;
    for (intI = 0; intI < *intDecodedLen_a && !intFailed; intI++)
    {
        int intSrc      = intOffset_a + intI * 2;
        uint32_t intPtr = (uint32_t)(byData_a[intSrc] | (byData_a[intSrc + 1] << 8));
        if (intPtr >= (uint32_t)ROM_SIZE)
        {
            fprintf(stderr, "Error: ROM pointer %u out of range\n", (unsigned)intPtr);
            intFailed = 1;
        }
        else
        {
            byOut[intI] = byRom_a[intPtr];
        }
    }

    if (intFailed) { free(byOut); return NULL; }
    return byOut;
}

// --- Load block file: <workdir>/<blockID>.ztb ---
uint8_t* load_block(const char *strWorkDir_a, const char *strBlockID_a, int *intLen_a)
{
    char strPath[FILENAME_MAX];
    snprintf(strPath, FILENAME_MAX, "%s/%s.ztb", strWorkDir_a, strBlockID_a);

    uint8_t *byResult = NULL;
    FILE *f = fopen(strPath, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        long lngSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (lngSize > 0)
        {
            byResult = (uint8_t*)malloc(lngSize);
            if (byResult)
            {
                if (fread(byResult, 1, lngSize, f) == (size_t)lngSize)
                {
                    *intLen_a = (int)lngSize;
                }
                else
                {
                    free(byResult);
                    byResult  = NULL;
                    *intLen_a = 0;
                }
            }
        }
        fclose(f);
    }

    return byResult;
}

// --- Find genesis: scan workdir for a .ztb file of exactly ROM_SIZE bytes ---
uint8_t* find_genesis(const char *strWorkDir_a)
{
    uint8_t *byResult = NULL;

#ifdef _WIN32
    char strPattern[FILENAME_MAX];
    snprintf(strPattern, FILENAME_MAX, "%s/*.ztb", strWorkDir_a);
    WIN32_FIND_DATAA objFind;
    HANDLE hFind = FindFirstFileA(strPattern, &objFind);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (objFind.nFileSizeHigh == 0 && objFind.nFileSizeLow == ROM_SIZE)
            {
                char strPath[FILENAME_MAX];
                snprintf(strPath, FILENAME_MAX, "%s/%s", strWorkDir_a, objFind.cFileName);
                FILE *f = fopen(strPath, "rb");
                if (f)
                {
                    byResult = (uint8_t*)malloc(ROM_SIZE);
                    if (byResult)
                    {
                        if (fread(byResult, 1, ROM_SIZE, f) != ROM_SIZE)
                        {
                            free(byResult);
                            byResult = NULL;
                        }
                    }
                    fclose(f);
                }
            }
        }
        while (byResult == NULL && FindNextFileA(hFind, &objFind) != 0);
        FindClose(hFind);
    }
#else
    DIR *d = opendir(strWorkDir_a);
    if (d)
    {
        struct dirent *dir;
        while (byResult == NULL && (dir = readdir(d)) != NULL)
        {
            int intNameLen = (int)strlen(dir->d_name);
            if (intNameLen > 4 && strcmp(dir->d_name + intNameLen - 4, ".ztb") == 0)
            {
                char strPath[FILENAME_MAX];
                snprintf(strPath, FILENAME_MAX, "%s/%s", strWorkDir_a, dir->d_name);
                struct stat objStat;
                if (stat(strPath, &objStat) == 0 && objStat.st_size == ROM_SIZE)
                {
                    FILE *f = fopen(strPath, "rb");
                    if (f)
                    {
                        byResult = (uint8_t*)malloc(ROM_SIZE);
                        if (byResult)
                        {
                            if (fread(byResult, 1, ROM_SIZE, f) != ROM_SIZE)
                            {
                                free(byResult);
                                byResult = NULL;
                            }
                        }
                        fclose(f);
                    }
                }
            }
        }
        closedir(d);
    }
#endif

    return byResult;
}

// --- Build rolling ROM (matches clsZTB.BuildRollingROM exactly) ---
// Walks back from strPrevBlockID_a via prev_block_id in the raw header,
// collecting up to MAX_HISTORY_BLOCKS blocks.
// Copies first ROM_ENTRY_SIZE bytes of each block into the ROM buffer.
// If a truncation block is found at the bottom, its payload (bytes 111..111+65535) is used
// as the fill source instead of the genesis.
// Remainder is filled from the fill source (truncation payload or genesis).
uint8_t* build_rolling_rom(const char *strWorkDir_a, const char *strPrevBlockID_a)
{
    uint8_t *arrROM        = (uint8_t*)malloc(ROM_SIZE);
    if (!arrROM) { return NULL; }
    memset(arrROM, 0, ROM_SIZE);

    int intBytesCopied   = 0;
    int intSamples       = 0;
    uint8_t *byTruncPayload = NULL;

    if (strPrevBlockID_a != NULL && strcmp(strPrevBlockID_a, NULL_GUID) != 0)
    {
        // Walk back up to MAX_HISTORY_BLOCKS, collecting block bytes oldest-first
        // We walk forward first to collect, then copy oldest first — match C# WalkBack
        // which does Insert(0, ...) so result is oldest-first.
        uint8_t *arrHistory[MAX_HISTORY_BLOCKS];
        int      arrHistoryLen[MAX_HISTORY_BLOCKS];
        int      intHistCount = 0;

        char strCurrentID[GUID_LEN];
        strncpy(strCurrentID, strPrevBlockID_a, GUID_LEN - 1);
        strCurrentID[GUID_LEN - 1] = '\0';

        while (strcmp(strCurrentID, NULL_GUID) != 0 &&
               strlen(strCurrentID) > 0 &&
               intHistCount < MAX_HISTORY_BLOCKS)
        {
            int intBlockLen = 0;
            uint8_t *byBlock = load_block(strWorkDir_a, strCurrentID, &intBlockLen);
            if (!byBlock || intBlockLen < HEADER_RAW_SIZE) 
            { 
                if (byBlock) { free(byBlock); }
                break; 
            }

            // Insert at front (shift existing entries right) to match C# Insert(0, ...)
            int intShift;
            for (intShift = intHistCount; intShift > 0; intShift--)
            {
                arrHistory[intShift]    = arrHistory[intShift - 1];
                arrHistoryLen[intShift] = arrHistoryLen[intShift - 1];
            }
            arrHistory[0]    = byBlock;
            arrHistoryLen[0] = intBlockLen;
            intHistCount++;

            // Stop walking at truncation block
            if (byBlock[RAW_OFF_BLOCK_TYPE] == BLOCK_TYPE_TRUNCATION) { break; }

            read_fixed_string(byBlock, RAW_OFF_PREV_ID, 36, strCurrentID);
        }

        // Check if oldest block is truncation — extract its ROM payload
        if (intHistCount > 0 &&
            arrHistory[0][RAW_OFF_BLOCK_TYPE] == BLOCK_TYPE_TRUNCATION)
        {
            if (arrHistoryLen[0] >= HEADER_RAW_SIZE + ROM_SIZE)
            {
                byTruncPayload = (uint8_t*)malloc(ROM_SIZE);
                if (byTruncPayload)
                {
                    memcpy(byTruncPayload, arrHistory[0] + HEADER_RAW_SIZE, ROM_SIZE);
                }
            }
            free(arrHistory[0]);
            // Shift remaining entries down
            int intShift;
            for (intShift = 0; intShift < intHistCount - 1; intShift++)
            {
                arrHistory[intShift]    = arrHistory[intShift + 1];
                arrHistoryLen[intShift] = arrHistoryLen[intShift + 1];
            }
            intHistCount--;
        }

        // Copy first ROM_ENTRY_SIZE bytes of each block into ROM
        int intI;
        for (intI = 0; intI < intHistCount; intI++)
        {
            if (intBytesCopied + ROM_ENTRY_SIZE > ROM_SIZE) { break; }
            if (intSamples >= MAX_HISTORY_BLOCKS)           { break; }
            int intCopy = arrHistoryLen[intI] < ROM_ENTRY_SIZE
                          ? arrHistoryLen[intI] : ROM_ENTRY_SIZE;
            memcpy(arrROM + intBytesCopied, arrHistory[intI], intCopy);
            intBytesCopied += ROM_ENTRY_SIZE;
            intSamples++;
        }

        for (intI = 0; intI < intHistCount; intI++) { free(arrHistory[intI]); }
    }

    // Fill remainder from truncation payload or genesis
    if (intBytesCopied < ROM_SIZE)
    {
        uint8_t *byFill = byTruncPayload;
        if (byFill == NULL)
        {
            byFill = find_genesis(strWorkDir_a);
        }

        if (byFill != NULL && byFill != byTruncPayload)
        {
            // byFill is genesis — check length
            memcpy(arrROM + intBytesCopied, byFill, ROM_SIZE - intBytesCopied);
            free(byFill);
        }
        else if (byFill != NULL)
        {
            memcpy(arrROM + intBytesCopied, byFill, ROM_SIZE - intBytesCopied);
        }
        else
        {
            free(arrROM);
            if (byTruncPayload) { free(byTruncPayload); }
            return NULL;
        }
    }

    if (byTruncPayload) { free(byTruncPayload); }
    return arrROM;
}