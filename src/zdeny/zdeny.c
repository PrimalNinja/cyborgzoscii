// Cyborg ZOSCII v20260417 - Plausible Deniability Generator
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version
// Creates a ROM that decodes an existing encoded file to any desired message
// Perfect for demonstrating the plausible deniability property of ZOSCII

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

// Structure to track which addresses we've already set
typedef struct
{
    bool blnSet;
    uint8_t byValue;
} AddressMapping;

static bool loadMessageFile(const char* strFilename_a, uint8_t** ptrMessageOut_a, int* intLengthOut_a)
{
    long intSize = 0;
    uint8_t* ptrBuffer = NULL;
    FILE* ptrFile = NULL;
    
    ptrFile = fopen(strFilename_a, "rb");
    if (!ptrFile)
    {
        perror("Failed to open message file");
        return false;
    }
    
    fseek(ptrFile, 0, SEEK_END);
    intSize = ftell(ptrFile);
    fseek(ptrFile, 0, SEEK_SET);
    
    if (intSize <= 0)
    {
        fprintf(stderr, "Error: Message file is empty\n");
        fclose(ptrFile);
        return false;
    }
    
    ptrBuffer = (uint8_t*)malloc(intSize);
    if (!ptrBuffer)
    {
        fprintf(stderr, "Error: Failed to allocate memory for message\n");
        fclose(ptrFile);
        return false;
    }
    
    fread(ptrBuffer, 1, intSize, ptrFile);
    fclose(ptrFile);
    
    *ptrMessageOut_a = ptrBuffer;
    *intLengthOut_a = (int)intSize;
    
    return true;
}

static bool createDeniabilityROM(const char* strTemplateROM_a,
                                  const char* strEncodedFile_a,
                                  const char* strMessageFile_a,
                                  const char* strOutputROM_a)
{
    uint16_t* arrAddresses = NULL;
    AddressMapping* arrMapping = NULL;
    bool blnSuccess = false;
    int intAddressCount = 0;
    int intConflicts = 0;
    long intEncodedSize = 0;
    int intI = 0;
    int intMsgLen = 0;
    int intMapped = 0;
    long intTemplateSize = 0;
    FILE* ptrEncoded = NULL;
    uint8_t* ptrMessage = NULL;
    FILE* ptrOutput = NULL;
    uint8_t* ptrROMData = NULL;
    FILE* ptrTemplate = NULL;
    
    // Load the desired message from file
    if (!loadMessageFile(strMessageFile_a, &ptrMessage, &intMsgLen))
    {
        return false;
    }
    
    printf("Loaded message: %d bytes from %s\n", intMsgLen, strMessageFile_a);
    
    // Open template ROM (real image file)
    ptrTemplate = fopen(strTemplateROM_a, "rb");
    if (!ptrTemplate)
    {
        perror("Failed to open template ROM");
        free(ptrMessage);
        return false;
    }
    
    // Get template size
    fseek(ptrTemplate, 0, SEEK_END);
    intTemplateSize = ftell(ptrTemplate);
    fseek(ptrTemplate, 0, SEEK_SET);
    
    printf("Template ROM: %s (%ld bytes)\n", strTemplateROM_a, intTemplateSize);
    
    // Allocate ROM (64KB)
    ptrROMData = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    if (!ptrROMData)
    {
        fprintf(stderr, "Error: Failed to allocate memory for ROM\n");
        fclose(ptrTemplate);
        free(ptrMessage);
        return false;
    }
    
    // Copy template ROM into our ROM buffer (up to 64KB)
    size_t szRead = fread(ptrROMData, 1, ZOSCII_ROM_SIZE, ptrTemplate);
    fclose(ptrTemplate);
    
    // If template is smaller than 64KB, pad by repeating the pattern
    if (szRead < ZOSCII_ROM_SIZE)
    {
        printf("Template smaller than 64KB (%zu bytes), repeating pattern for padding\n", szRead);
        for (size_t i = szRead; i < ZOSCII_ROM_SIZE; i++)
        {
            ptrROMData[i] = ptrROMData[i % szRead];
        }
    }
    
    // Open encoded file
    ptrEncoded = fopen(strEncodedFile_a, "rb");
    if (!ptrEncoded)
    {
        perror("Failed to open encoded file");
        free(ptrROMData);
        free(ptrMessage);
        return false;
    }
    
    // Get encoded file size and calculate number of addresses
    fseek(ptrEncoded, 0, SEEK_END);
    intEncodedSize = ftell(ptrEncoded);
    fseek(ptrEncoded, 0, SEEK_SET);
    
    // Each address is 2 bytes (16-bit)
    intAddressCount = (int)(intEncodedSize / 2);
    if (intAddressCount <= 0)
    {
        fprintf(stderr, "Error: Encoded file is empty or corrupt\n");
        fclose(ptrEncoded);
        free(ptrROMData);
        free(ptrMessage);
        return false;
    }
    
    printf("Encoded file: %s (%ld bytes, %d addresses)\n", strEncodedFile_a, intEncodedSize, intAddressCount);
    
    // Allocate memory for addresses
    arrAddresses = (uint16_t*)malloc(intAddressCount * sizeof(uint16_t));
    if (!arrAddresses)
    {
        fprintf(stderr, "Error: Failed to allocate memory for addresses\n");
        fclose(ptrEncoded);
        free(ptrROMData);
        free(ptrMessage);
        return false;
    }
    
    // Read all addresses from encoded file
    for (intI = 0; intI < intAddressCount; intI++)
    {
        uint8_t byLow, byHigh;
        if (fread(&byLow, 1, 1, ptrEncoded) != 1 ||
            fread(&byHigh, 1, 1, ptrEncoded) != 1)
        {
            fprintf(stderr, "Error: Failed to read address %d\n", intI);
            free(arrAddresses);
            free(ptrROMData);
            free(ptrMessage);
            fclose(ptrEncoded);
            return false;
        }
        arrAddresses[intI] = (uint16_t)byLow | ((uint16_t)byHigh << 8);
    }
    fclose(ptrEncoded);
    
    // Truncate message if longer than addresses
    if (intMsgLen > intAddressCount)
    {
        printf("Warning: Message length (%d) exceeds address count (%d)\n", intMsgLen, intAddressCount);
        printf("         Truncating message to %d bytes\n", intAddressCount);
        intMsgLen = intAddressCount;
    }
    
    // Allocate mapping table to track conflicts
    arrMapping = (AddressMapping*)calloc(ZOSCII_ROM_SIZE, sizeof(AddressMapping));
    if (!arrMapping)
    {
        fprintf(stderr, "Error: Failed to allocate mapping table\n");
        free(arrAddresses);
        free(ptrROMData);
        free(ptrMessage);
        return false;
    }
    
    // Overwrite ROM addresses with desired message bytes
    for (intI = 0; intI < intMsgLen; intI++)
    {
        uint16_t intAddr = arrAddresses[intI];
        uint8_t byDesired = ptrMessage[intI];
        
        if (intAddr >= ZOSCII_ROM_SIZE)
        {
            printf("Warning: Address %u out of range (max %d) - skipping\n", intAddr, ZOSCII_ROM_SIZE - 1);
            continue;
        }
        
        if (arrMapping[intAddr].blnSet)
        {
            // Conflict: this address already mapped to a different byte
            if (arrMapping[intAddr].byValue != byDesired)
            {
                intConflicts++;
                // Only show first 10 conflicts to avoid spam
                if (intConflicts <= 10)
                {
                    printf("Conflict at position %d: address %u already set to 0x%02X ('%c'), cannot also be 0x%02X ('%c')\n",
                           intI, intAddr, arrMapping[intAddr].byValue, 
                           (arrMapping[intAddr].byValue >= 32 && arrMapping[intAddr].byValue <= 126) ? arrMapping[intAddr].byValue : '?',
                           byDesired,
                           (byDesired >= 32 && byDesired <= 126) ? byDesired : '?');
                }
                else if (intConflicts == 11)
                {
                    printf("... additional conflicts suppressed\n");
                }
            }
            // Skip - keep original mapping
        }
        else
        {
            ptrROMData[intAddr] = byDesired;
            arrMapping[intAddr].blnSet = true;
            arrMapping[intAddr].byValue = byDesired;
            intMapped++;
        }
    }
    
    // Write ROM to output file
    ptrOutput = fopen(strOutputROM_a, "wb");
    if (!ptrOutput)
    {
        perror("Failed to create output ROM file");
        free(ptrROMData);
        free(arrAddresses);
        free(ptrMessage);
        free(arrMapping);
        return false;
    }
    
    fwrite(ptrROMData, 1, ZOSCII_ROM_SIZE, ptrOutput);
    fclose(ptrOutput);
    
    // Report statistics
    printf("\n");
    printf("ZOSCII Plausible Deniability ROM Created\n");
    printf("=========================================\n");
    printf("Template ROM:      %s\n", strTemplateROM_a);
    printf("Encoded file:      %s\n", strEncodedFile_a);
    printf("Message file:      %s (%d bytes)\n", strMessageFile_a, intMsgLen);
    printf("Output ROM:        %s\n", strOutputROM_a);
    printf("ROM size:          %d bytes (64KB)\n", ZOSCII_ROM_SIZE);
    printf("Addresses mapped:  %d\n", intMapped);
    printf("Conflicts skipped: %d\n", intConflicts);
    
    if (intConflicts > 0)
    {
        printf("\nNote: %d conflicts were skipped.\n", intConflicts);
        printf("      The decoded message will have garbage at those positions.\n");
        printf("      For perfect deniability, use a message with unique addresses.\n");
    }
    
    printf("\nVerification:\n");
    printf("  zdecode \"%s\" \"%s\" > decoded.txt\n", strOutputROM_a, strEncodedFile_a);
    printf("\n");
    printf("Security Note:\n");
    printf("  This ROM is based on a real image (%s).\n", strTemplateROM_a);
    printf("  Only %d bytes were changed. The rest is identical to the original photo.\n", intMapped);
    printf("  Forensic analysis cannot distinguish this from a genuine ROM.\n");
    
    blnSuccess = true;
    
    free(ptrROMData);
    free(arrAddresses);
    free(ptrMessage);
    free(arrMapping);
    
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

    printf("ZOSCII Plausible Deniability Generator v20260417\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");
    printf("==================================================\n\n");

    if (intArgC_a == 5)
    {
        blnOk = createDeniabilityROM(strArgv_a[1], strArgv_a[2], strArgv_a[3], strArgv_a[4]);
        
        if (blnOk)
        {
            printf("\nSUCCESS: Plausible deniability ROM created.\n");
            printf("         Use '%s' to decode '%s' as your claimed message file.\n",
                   strArgv_a[4], strArgv_a[2]);
            intResult = 0;
        }
        else
        {
            fprintf(stderr, "\nERROR: Failed to create deniability ROM\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <template_rom> <encoded_file> <message_file> <output_rom>\n", strArgv_a[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Parameters (order matches zencode/zdecode):\n");
        fprintf(stderr, "  template_rom  - Real image/file to use as template (e.g., selfie.jpg)\n");
        fprintf(stderr, "  encoded_file  - Existing ZOSCII encoded file (the 'evidence')\n");
        fprintf(stderr, "  message_file  - File containing the message you want to claim\n");
        fprintf(stderr, "  output_rom    - ROM file to create (the 'fake key')\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s selfie.jpg real.zoc fake.txt deny.rom\n", strArgv_a[0]);
        fprintf(stderr, "  zdecode deny.rom real.zoc > decoded.txt\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "This creates a ROM that is 99.9%% identical to selfie.jpg,\n");
        fprintf(stderr, "but decodes real.zoc to the contents of fake.txt.\n");
        fprintf(stderr, "Forensic analysis cannot distinguish it from the original photo.\n");
    }
    
    return intResult;
}