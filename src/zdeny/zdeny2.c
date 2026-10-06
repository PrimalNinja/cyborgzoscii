// Cyborg ZOSCII v20261003 - Double-Encoding Plausible Deniability Generator
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version
// Creates TWO ROMs that decode an existing double-encoded file to any desired message
//
// Model:  encoded file = 16-bit addresses into ROM 2  -> bytes of S
//         S            = 16-bit addresses into ROM 1  -> message
//         (message N bytes -> S is 2N bytes -> encoded file is 4N bytes)
//
// All addresses are little-endian, as everywhere in ZOSCII.
//
// Randomness: same method as zencode - srand() seeded with a hash of the ROM
// XORed with the timer, then rand(). Standard C only. This is a plausibility
// decoder, not a cipher, so the RNG only needs to vary.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

#define ZOSCII_ROM_SIZE 65536  // 64KB standard ROM size
#define MAX_ATTEMPTS 200

// Seed exactly as zencode does: ROM hash XOR timer (the last ROM loaded wins)
static void seedFromROM(const uint8_t* ptrROM_a, size_t szLen_a)
{
    uint32_t intHash = 0;
    for (size_t szI = 0; szI < szLen_a; szI++)
    {
        intHash = (intHash * 33) + ptrROM_a[szI];
    }
    intHash ^= (uint32_t)time(NULL);
    srand(intHash);
}

static uint8_t rndByte(void)
{
    return (uint8_t)(rand() % 256);
}

// Two bytes -> one 16-bit little-endian address (used for the encoded file AND for S)
static uint16_t makeWord(uint8_t byLow_a, uint8_t byHigh_a)
{
    return (uint16_t)(byLow_a | ((uint16_t)byHigh_a << 8));
}

static uint8_t* readWholeFile(const char* strFilename_a, const char* strWhat_a, size_t* ptrLenOut_a)
{
    long intSize = 0;
    uint8_t* ptrBuffer = NULL;
    FILE* ptrFile = fopen(strFilename_a, "rb");

    if (!ptrFile)
    {
        fprintf(stderr, "Failed to open %s '%s'\n", strWhat_a, strFilename_a);
        perror("fopen");
        return NULL;
    }

    fseek(ptrFile, 0, SEEK_END);
    intSize = ftell(ptrFile);
    fseek(ptrFile, 0, SEEK_SET);

    if (intSize <= 0)
    {
        fprintf(stderr, "Error: %s is empty\n", strWhat_a);
        fclose(ptrFile);
        return NULL;
    }

    ptrBuffer = (uint8_t*)malloc((size_t)intSize);
    if (!ptrBuffer || fread(ptrBuffer, 1, (size_t)intSize, ptrFile) != (size_t)intSize)
    {
        fprintf(stderr, "Error: failed to read %s\n", strWhat_a);
        free(ptrBuffer);
        fclose(ptrFile);
        return NULL;
    }

    fclose(ptrFile);
    *ptrLenOut_a = (size_t)intSize;
    return ptrBuffer;
}

// Fill a 64KB ROM from a template file ("-" = random ROM, no template)
static bool loadROM(const char* strTemplate_a, uint8_t* ptrROM_a, const char* strLabel_a)
{
    size_t szRead = 0;
    FILE* ptrFile = NULL;

    if (strcmp(strTemplate_a, "-") == 0)
    {
        for (size_t szI = 0; szI < ZOSCII_ROM_SIZE; szI++)
        {
            ptrROM_a[szI] = rndByte();
        }
        printf("%s: random (no template)\n", strLabel_a);
        return true;
    }

    ptrFile = fopen(strTemplate_a, "rb");
    if (!ptrFile)
    {
        fprintf(stderr, "Failed to open %s '%s'\n", strLabel_a, strTemplate_a);
        perror("fopen");
        return false;
    }

    szRead = fread(ptrROM_a, 1, ZOSCII_ROM_SIZE, ptrFile);
    fclose(ptrFile);

    if (szRead == 0)
    {
        fprintf(stderr, "Error: %s '%s' is empty\n", strLabel_a, strTemplate_a);
        return false;
    }

    printf("%s: %s (%zu bytes used)\n", strLabel_a, strTemplate_a, szRead);

    // If template is smaller than 64KB, pad by repeating the pattern
    if (szRead < ZOSCII_ROM_SIZE)
    {
        printf("  smaller than 64KB, repeating pattern for padding\n");
        for (size_t szI = szRead; szI < ZOSCII_ROM_SIZE; szI++)
        {
            ptrROM_a[szI] = ptrROM_a[szI % szRead];
        }
    }
    return true;
}

static bool writeROM(const char* strFilename_a, const uint8_t* ptrROM_a)
{
    FILE* ptrFile = fopen(strFilename_a, "wb");

    if (!ptrFile)
    {
        fprintf(stderr, "Failed to create output ROM '%s'\n", strFilename_a);
        perror("fopen");
        return false;
    }
    if (fwrite(ptrROM_a, 1, ZOSCII_ROM_SIZE, ptrFile) != ZOSCII_ROM_SIZE)
    {
        fprintf(stderr, "Error: failed to write '%s'\n", strFilename_a);
        fclose(ptrFile);
        return false;
    }
    fclose(ptrFile);
    return true;
}

static bool createDeniabilityROMs(const char* strTemplate1_a, const char* strTemplate2_a,
                                  const char* strEncodedFile_a, const char* strMessageFile_a,
                                  const char* strOutputROM1_a, const char* strOutputROM2_a)
{
    uint8_t* arrAssigned = NULL;
    uint8_t* arrSkip = NULL;
    size_t szSkipped = 0;
    bool blnBest = false;
    uint32_t* arrCount = NULL;
    uint8_t* arrS = NULL;
    uint8_t* arrUsed = NULL;
    uint8_t* arrV1 = NULL;
    uint8_t* arrVal = NULL;
    uint16_t* arrW = NULL;
    bool blnOk = false;
    bool blnSuccess = false;
    int intAttempt = 0;
    int intMapped1 = 0;
    int intMapped2 = 0;
    int intMismatch = 0;
    int intRerolls = 0;
    size_t szEnc = 0;
    size_t szI = 0;
    size_t szMsg = 0;
    size_t szN = 0;
    size_t szPadded = 0;
    size_t szW2 = 0;
    uint8_t* ptrEnc = NULL;
    uint8_t* ptrMsg = NULL;
    uint8_t* ptrROM1 = NULL;
    uint8_t* ptrROM2 = NULL;

    ptrMsg = readWholeFile(strMessageFile_a, "message file", &szMsg);
    if (!ptrMsg)
    {
        goto done;
    }
    printf("Loaded message: %zu bytes from %s\n", szMsg, strMessageFile_a);

    ptrEnc = readWholeFile(strEncodedFile_a, "encoded file", &szEnc);
    if (!ptrEnc)
    {
        goto done;
    }
    szN = szEnc / 4;     // message bytes the encoded file holds
    szW2 = szN * 2;      // layer-2 addresses
    printf("Encoded file: %s (%zu bytes, %zu addresses, %zu message bytes)\n", strEncodedFile_a, szEnc, szEnc / 2, szN);
    if (szEnc % 4)
    {
        printf("Warning: encoded file is not a multiple of 4 bytes, ignoring the last %zu byte(s)\n", szEnc % 4);
    }
    if (szN == 0)
    {
        fprintf(stderr, "Error: Encoded file is too short for a double-encoded message\n");
        goto done;
    }

    if (szMsg > szN)
    {
        printf("Warning: Message length (%zu) exceeds message capacity (%zu)\n", szMsg, szN);
        printf("         Truncating message to %zu bytes\n", szN);
        szMsg = szN;
    }
    else if (szMsg < szN)
    {
        // Pad with spaces so every position decodes to something clean (no junk tail)
        uint8_t* ptrPadded = (uint8_t*)realloc(ptrMsg, szN);
        if (!ptrPadded)
        {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            goto done;
        }
        ptrMsg = ptrPadded;
        szPadded = szN - szMsg;
        memset(ptrMsg + szMsg, ' ', szPadded);
        printf("Message is %zu bytes but the encoded file holds %zu.\n", szMsg, szN);
        printf("         Padding positions %zu onward with spaces.\n", szMsg);
        szMsg = szN;
    }

    arrW = (uint16_t*)malloc(szW2 * sizeof(uint16_t));
    arrS = (uint8_t*)malloc(szW2);
    arrCount = (uint32_t*)calloc(ZOSCII_ROM_SIZE, sizeof(uint32_t));
    arrVal = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    arrAssigned = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    arrUsed = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    arrV1 = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    arrSkip = (uint8_t*)calloc(szMsg, 1);
    ptrROM1 = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    ptrROM2 = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    if (!arrW || !arrS || !arrCount || !arrVal || !arrAssigned || !arrUsed || !arrV1 || !arrSkip || !ptrROM1 || !ptrROM2)
    {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        goto done;
    }

    for (szI = 0; szI < szW2; szI++)
    {
        arrW[szI] = makeWord(ptrEnc[2 * szI], ptrEnc[2 * szI + 1]);
        arrCount[arrW[szI]]++;
    }

    srand((uint32_t)time(NULL));   // timer only, so a '-' (random) ROM can be filled
    if (!loadROM(strTemplate1_a, ptrROM1, "Template ROM 1 (inner)"))
    {
        goto done;
    }
    seedFromROM(ptrROM1, ZOSCII_ROM_SIZE);
    if (!loadROM(strTemplate2_a, ptrROM2, "Template ROM 2 (outer)"))
    {
        goto done;
    }
    seedFromROM(ptrROM2, ZOSCII_ROM_SIZE);

    // Choose S: equal layer-2 addresses force equal S bytes; every other S byte is free
    // Passes 1..MAX_ATTEMPTS must satisfy every position. If none does, one final
    // best-effort pass skips the positions it cannot satisfy (they decode as garbage,
    // like zdeny) instead of failing.
    for (intAttempt = 0; intAttempt <= MAX_ATTEMPTS && !blnOk; )
    {
        blnBest = (intAttempt == MAX_ATTEMPTS);
        intAttempt++;
        memset(arrAssigned, 0, ZOSCII_ROM_SIZE);
        memset(arrUsed, 0, ZOSCII_ROM_SIZE);
        for (szI = 0; szI < szW2; szI++)
        {
            uint16_t intA = arrW[szI];
            if (!arrAssigned[intA])
            {
                arrVal[intA] = rndByte();
                arrAssigned[intA] = 1;
            }
            arrS[szI] = arrVal[intA];
        }

        blnOk = true;
        szSkipped = 0;
        memset(arrSkip, 0, szMsg);
        for (szI = 0; szI < szMsg && blnOk; szI++)
        {
            // an S word may repeat, but only for the same message byte
            uint16_t intS = makeWord(arrS[2 * szI], arrS[2 * szI + 1]);
            int intTries = 0;
            bool blnSkip = false;
            while (arrUsed[intS] && arrV1[intS] != ptrMsg[szI])
            {
                intRerolls++;
                if (arrCount[arrW[2 * szI + 1]] == 1)
                {
                    arrS[2 * szI + 1] = rndByte();
                }
                else if (arrCount[arrW[2 * szI]] == 1)
                {
                    arrS[2 * szI] = rndByte();
                }
                else
                {
                    if (blnBest) { blnSkip = true; } else { blnOk = false; }   // both bytes tied to repeats
                    break;
                }
                intS = makeWord(arrS[2 * szI], arrS[2 * szI + 1]);
                if (++intTries > 100000)
                {
                    if (blnBest) { blnSkip = true; } else { blnOk = false; }
                    break;
                }
            }
            if (blnSkip)
            {
                arrSkip[szI] = 1;
                szSkipped++;
            }
            else if (blnOk)
            {
                arrUsed[intS] = 1;
                arrV1[intS] = ptrMsg[szI];
            }
        }
    }

    if (!blnOk)
    {
        fprintf(stderr, "Error: could not build a ROM pair for this message after %d attempts\n", MAX_ATTEMPTS);
        fprintf(stderr, "       Repeated addresses in the encoded file force two S words equal while\n");
        fprintf(stderr, "       the message needs different characters there.\n");
        goto done;
    }

    // Overwrite ROM cells
    for (szI = 0; szI < szMsg; szI++)
    {
        if (!arrSkip[szI])
        {
            ptrROM1[makeWord(arrS[2 * szI], arrS[2 * szI + 1])] = ptrMsg[szI];
        }
    }
    for (szI = 0; szI < szW2; szI++)
    {
        ptrROM2[arrW[szI]] = arrS[szI];
    }
    for (szI = 0; szI < ZOSCII_ROM_SIZE; szI++)
    {
        intMapped1 += arrUsed[szI];
        intMapped2 += arrAssigned[szI];
    }

    // Decode in memory with the finished ROMs before writing anything
    for (szI = 0; szI < szMsg; szI++)
    {
        if (arrSkip[szI])
        {
            continue;
        }
        uint16_t intS = makeWord(ptrROM2[arrW[2 * szI]], ptrROM2[arrW[2 * szI + 1]]);
        if (ptrROM1[intS] != ptrMsg[szI])
        {
            intMismatch++;
        }
    }
    if (intMismatch)
    {
        fprintf(stderr, "Error: self-check failed (%d mismatches), nothing written\n", intMismatch);
        goto done;
    }

    if (!writeROM(strOutputROM1_a, ptrROM1) || !writeROM(strOutputROM2_a, ptrROM2))
    {
        goto done;
    }

    printf("\n");
    printf("ZOSCII Double-Encoding Plausible Deniability ROMs Created\n");
    printf("==========================================================\n");
    printf("Template ROM 1:    %s\n", strTemplate1_a);
    printf("Template ROM 2:    %s\n", strTemplate2_a);
    printf("Encoded file:      %s\n", strEncodedFile_a);
    printf("Message file:      %s (%zu bytes", strMessageFile_a, szMsg - szPadded);
    if (szPadded)
    {
        printf(" + %zu spaces of padding", szPadded);
    }
    printf(")\n");
    printf("Output ROM 1:      %s (inner)\n", strOutputROM1_a);
    printf("Output ROM 2:      %s (outer)\n", strOutputROM2_a);
    printf("ROM size:          %d bytes (64KB) each\n", ZOSCII_ROM_SIZE);
    printf("ROM 1 cells set:   %d\n", intMapped1);
    printf("ROM 2 cells set:   %d\n", intMapped2);
    printf("Conflicts re-rolled: %d (over %d build attempt(s))\n", intRerolls, intAttempt);
    if (szSkipped)
    {
        printf("Unsatisfied:       %zu of %zu positions (%.1f%%) could not be mapped and decode as garbage\n",
               szSkipped, szMsg, 100.0 * (double)szSkipped / (double)szMsg);
    }
    printf("Self-check:        the other %zu positions decode to the message\n", szMsg - szSkipped);
    printf("\n");
    printf("Verification:\n");
    printf("  zdecode \"%s\" \"%s\" \"%s\" decoded.txt\n", strOutputROM1_a, strOutputROM2_a, strEncodedFile_a);
    printf("\n");
    printf("Security Note:\n");
    printf("  Only %d (ROM 1) and %d (ROM 2) bytes differ from the templates.\n", intMapped1, intMapped2);
    printf("  Anyone holding the original templates can diff them against these ROMs.\n");

    blnSuccess = true;

done:
    free(arrAssigned);
    free(arrCount);
    free(arrS);
    free(arrUsed);
    free(arrV1);
    free(arrSkip);
    free(arrVal);
    free(arrW);
    free(ptrEnc);
    free(ptrMsg);
    free(ptrROM1);
    free(ptrROM2);
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    bool blnOk = false;
    int intResult = 1;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Double-Encoding Plausible Deniability Generator v20261003\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");
    printf("==================================================\n\n");

    if (intArgC_a == 7)
    {
        blnOk = createDeniabilityROMs(strArgv_a[1], strArgv_a[2], strArgv_a[3], strArgv_a[4], strArgv_a[5], strArgv_a[6]);

        if (blnOk)
        {
            printf("\nSUCCESS: Plausible deniability ROMs created.\n");
            printf("         Use '%s' + '%s' to decode '%s' as your claimed message file.\n",
                   strArgv_a[5], strArgv_a[6], strArgv_a[3]);
            intResult = 0;
        }
        else
        {
            fprintf(stderr, "\nERROR: Failed to create deniability ROMs\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <template_rom1> <template_rom2> <encoded_file> <message_file> <output_rom1> <output_rom2>\n", strArgv_a[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Parameters:\n");
        fprintf(stderr, "  template_rom1 - Real image/file for the INNER ROM (e.g., selfie.jpg), or - for a random ROM\n");
        fprintf(stderr, "  template_rom2 - Real image/file for the OUTER ROM (e.g., holiday.jpg), or - for a random ROM\n");
        fprintf(stderr, "  encoded_file  - Existing double-encoded ZOSCII file (the 'evidence'), raw binary\n");
        fprintf(stderr, "  message_file  - File containing the message you want to claim\n");
        fprintf(stderr, "  output_rom1   - Inner ROM file to create (fake key 1)\n");
        fprintf(stderr, "  output_rom2   - Outer ROM file to create (fake key 2)\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s selfie.jpg holiday.jpg real.zoc fake.txt deny1.rom deny2.rom\n", strArgv_a[0]);
        fprintf(stderr, "  zdecode deny1.rom deny2.rom real.zoc decoded.txt\n");
    }

    return intResult;
}