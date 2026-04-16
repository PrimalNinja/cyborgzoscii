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
#define ZOSCII_ROM_LOAD_MAX 131072L

// Structure to track which addresses we've already set
typedef struct
{
    bool blnSet;
    uint8_t byValue;
} AddressMapping;

static bool loadMessageFile(const char* strFilename_a, uint8_t** ptrMessageOut_a, int* intLengthOut_a)
{
    FILE* ptrFile = NULL;
    uint8_t* ptrBuffer = NULL;
    long lngSize = 0;
    
    ptrFile = fopen(strFilename_a, "rb");
    if (!ptrFile)
    {
        perror("Failed to open message file");
        return false;
    }
    
    fseek(ptrFile, 0, SEEK_END);
    lngSize = ftell(ptrFile);
    fseek(ptrFile, 0, SEEK_SET);
    
    if (lngSize <= 0)
    {
        fprintf(stderr, "Error: Message file is empty\n");
        fclose(ptrFile);
        return false;
    }
    
    ptrBuffer = (uint8_t*)malloc(lngSize);
    if (!ptrBuffer)
    {
        fprintf(stderr, "Error: Failed to allocate memory for message\n");
        fclose(ptrFile);
        return false;
    }
    
    fread(ptrBuffer, 1, lngSize, ptrFile);
    fclose(ptrFile);
    
    *ptrMessageOut_a = ptrBuffer;
    *intLengthOut_a = (int)lngSize;
    
    return true;
}

static bool createDeniabilityRom(const char* strEncodedFile_a, const char* strMessageFile_a, const char* strOutputRom_a)
{
    bool blnSuccess = false;
    FILE* ptrEncoded = NULL;
    FILE* ptrOutput = NULL;
    uint8_t* ptrRom = NULL;
    uint8_t* ptrMessage = NULL;
    AddressMapping* arrMapping = NULL;
    uint16_t* arrAddresses = NULL;
    long lngEncodedSize = 0;
    int intAddressCount = 0;
    int intMsgLen = 0;
    int intMapped = 0;
    int intConflicts = 0;
    int intI = 0;
    
    // Load the desired message from file
    if (!loadMessageFile(strMessageFile_a, &ptrMessage, &intMsgLen))
    {
        return false;
    }
    
    printf("Loaded message: %d bytes from %s\n", intMsgLen, strMessageFile_a);
    
    // Open encoded file
    ptrEncoded = fopen(strEncodedFile_a, "rb");
    if (!ptrEncoded)
    {
        perror("Failed to open encoded file");
        free(ptrMessage);
        return false;
    }
    
    // Get encoded file size and calculate number of addresses
    fseek(ptrEncoded, 0, SEEK_END);
    lngEncodedSize = ftell(ptrEncoded);
    fseek(ptrEncoded, 0, SEEK_SET);
    
    // Each address is 2 bytes (16-bit)
    intAddressCount = (int)(lngEncodedSize / 2);
    if (intAddressCount <= 0)
    {
        fprintf(stderr, "Error: Encoded file is empty or corrupt\n");
        fclose(ptrEncoded);
        free(ptrMessage);
        return false;
    }
    
    printf("Encoded file: %d addresses (%ld bytes)\n", intAddressCount, lngEncodedSize);
    
    // Allocate memory for addresses
    arrAddresses = (uint16_t*)malloc(intAddressCount * sizeof(uint16_t));
    if (!arrAddresses)
    {
        fprintf(stderr, "Error: Failed to allocate memory for addresses\n");
        fclose(ptrEncoded);
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
    
    // Allocate ROM (64KB)
    ptrRom = (uint8_t*)malloc(ZOSCII_ROM_SIZE);
    if (!ptrRom)
    {
        fprintf(stderr, "Error: Failed to allocate memory for ROM\n");
        free(arrAddresses);
        free(ptrMessage);
        return false;
    }
    
    // Initialize ROM with random bytes (for maximum entropy)
    srand((unsigned int)time(NULL));
    for (intI = 0; intI < ZOSCII_ROM_SIZE; intI++)
    {
        ptrRom[intI] = (uint8_t)(rand() % 256);
    }
    
    // Allocate mapping table to track conflicts
    arrMapping = (AddressMapping*)calloc(ZOSCII_ROM_SIZE, sizeof(AddressMapping));
    if (!arrMapping)
    {
        fprintf(stderr, "Error: Failed to allocate mapping table\n");
        free(ptrRom);
        free(arrAddresses);
        free(ptrMessage);
        return false;
    }
    
    // Set ROM addresses to desired message bytes
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
                    printf("Conflict at position %d: address %u already set to 0x%02X, cannot also be 0x%02X\n",
                           intI, intAddr, arrMapping[intAddr].byValue, byDesired);
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
            ptrRom[intAddr] = byDesired;
            arrMapping[intAddr].blnSet = true;
            arrMapping[intAddr].byValue = byDesired;
            intMapped++;
        }
    }
    
    // Write ROM to output file
    ptrOutput = fopen(strOutputRom_a, "wb");
    if (!ptrOutput)
    {
        perror("Failed to create output ROM file");
        free(ptrRom);
        free(arrAddresses);
        free(ptrMessage);
        free(arrMapping);
        return false;
    }
    
    fwrite(ptrRom, 1, ZOSCII_ROM_SIZE, ptrOutput);
    fclose(ptrOutput);
    
    // Report statistics
    printf("\n");
    printf("ZOSCII Plausible Deniability ROM Created\n");
    printf("=========================================\n");
    printf("Encoded file:      %s\n", strEncodedFile_a);
    printf("Message file:      %s (%d bytes)\n", strMessageFile_a, intMsgLen);
    printf("ROM file:          %s\n", strOutputRom_a);
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
    printf("  zdecode \"%s\" \"%s\" > decoded.bin\n", strOutputRom_a, strEncodedFile_a);
    printf("\n");
    printf("Security Note:\n");
    printf("  Any ROM can decode any encoded file to SOMETHING.\n");
    printf("  This ROM proves your file contains EXACTLY your claimed message.\n");
    printf("  The original ROM is mathematically indistinguishable from this one.\n");
    
    blnSuccess = true;
    
    free(ptrRom);
    free(arrAddresses);
    free(ptrMessage);
    free(arrMapping);
    
    return blnSuccess;
}

int main(int intArgC_a, char* strArgv_a[])
{
    int intResult = 1;
    bool blnOk = false;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Plausible Deniability Generator v20260417\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");
    printf("================================================\n\n");

    if (intArgC_a == 4)
    {
        blnOk = createDeniabilityRom(strArgv_a[1], strArgv_a[2], strArgv_a[3]);
        
        if (blnOk)
        {
            printf("\nSUCCESS: Plausible deniability ROM created.\n");
            printf("         Use '%s' to decode '%s' as your claimed message file.\n",
                   strArgv_a[3], strArgv_a[1]);
            intResult = 0;
        }
        else
        {
            fprintf(stderr, "\nERROR: Failed to create deniability ROM\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <encoded_file> <message_file> <output_rom>\n", strArgv_a[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Creates a ROM that decodes <encoded_file> to the contents of <message_file>\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Parameters:\n");
        fprintf(stderr, "  encoded_file  - Existing ZOSCII encoded file (the 'evidence')\n");
        fprintf(stderr, "  message_file  - File containing the message you want to claim\n");
        fprintf(stderr, "  output_rom    - ROM file to create (the 'fake key')\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  echo -n \"My shopping list\" > innocent.txt\n");
        fprintf(stderr, "  %s evidence.bin innocent.txt fake.rom\n", strArgv_a[0]);
        fprintf(stderr, "  zdecode fake.rom evidence.bin > decoded.txt\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "This demonstrates the plausible deniability property of ZOSCII:\n");
        fprintf(stderr, "  Any encoded file can be decoded to ANY message with the right ROM.\n");
        fprintf(stderr, "  No ROM is mathematically more 'correct' than any other.\n");
    }
    
    return intResult;
}