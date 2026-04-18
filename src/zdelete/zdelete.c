// Cyborg ZOSCII v20260418
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version - Secure Delete

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

#define CHUNK_SIZE 4096

static bool fileExists(const char* strPath_a)
{
    bool blnResult = false;
    FILE* ptrFile = NULL;
    
    ptrFile = fopen(strPath_a, "rb");
    if (ptrFile)
    {
        fclose(ptrFile);
        blnResult = true;
    }
    
    return blnResult;
}

static bool secureDelete(const char* strPath_a)
{
    uint8_t* arr00 = NULL;
    uint8_t* arrFF = NULL;
    bool blnResult = false;
    int intI = 0;
    long intLength = 0;
    long intToWrite = 0;
    long intWritten = 0;
    FILE* ptrFile = NULL;
    
    if (!fileExists(strPath_a))
    {
        fprintf(stderr, "File not found: %s\n", strPath_a);
        return false;
    }
    
    // Get file size
    ptrFile = fopen(strPath_a, "rb");
    if (!ptrFile)
    {
        fprintf(stderr, "Cannot open file: %s\n", strPath_a);
        return false;
    }
    
    fseek(ptrFile, 0, SEEK_END);
    intLength = ftell(ptrFile);
    fclose(ptrFile);
    
    if (intLength <= 0)
    {
        // Empty file or error, just delete
        remove(strPath_a);
        return true;
    }
    
    // Allocate buffers
    arrFF = (uint8_t*)malloc(CHUNK_SIZE);
    arr00 = (uint8_t*)malloc(CHUNK_SIZE);
    
    if (!arrFF || !arr00)
    {
        fprintf(stderr, "Memory allocation failed\n");
        if (arrFF) free(arrFF);
        if (arr00) free(arr00);
        return false;
    }
    
    // Initialize buffers
    for (intI = 0; intI < CHUNK_SIZE; intI++)
    {
        arrFF[intI] = 0xFF;
        arr00[intI] = 0x00;
    }
    
    // Pass 1: overwrite with 0xFF
    ptrFile = fopen(strPath_a, "rb+");
    if (!ptrFile)
    {
        fprintf(stderr, "Cannot open file for writing: %s\n", strPath_a);
        free(arrFF);
        free(arr00);
        return false;
    }
    
    intWritten = 0;
    while (intWritten < intLength)
    {
        intToWrite = (intLength - intWritten < CHUNK_SIZE) ? (intLength - intWritten) : CHUNK_SIZE;
        if (fwrite(arrFF, 1, intToWrite, ptrFile) != (size_t)intToWrite)
        {
            fprintf(stderr, "Write failed during 0xFF pass\n");
            fclose(ptrFile);
            free(arrFF);
            free(arr00);
            return false;
        }
        intWritten += intToWrite;
    }
    fflush(ptrFile);
    
    // Pass 2: overwrite with 0x00
    fseek(ptrFile, 0, SEEK_SET);
    intWritten = 0;
    while (intWritten < intLength)
    {
        intToWrite = (intLength - intWritten < CHUNK_SIZE) ? (intLength - intWritten) : CHUNK_SIZE;
        if (fwrite(arr00, 1, intToWrite, ptrFile) != (size_t)intToWrite)
        {
            fprintf(stderr, "Write failed during 0x00 pass\n");
            fclose(ptrFile);
            free(arrFF);
            free(arr00);
            return false;
        }
        intWritten += intToWrite;
    }
    fflush(ptrFile);
    fclose(ptrFile);
    
    // Delete the file
    if (remove(strPath_a) == 0)
    {
        blnResult = true;
    }
    else
    {
        fprintf(stderr, "Failed to delete file after overwrite: %s\n", strPath_a);
    }
    
    free(arrFF);
    free(arr00);
    return blnResult;
}

int main(int argc_a, char* strArgv_a[])
{
    bool blnDeleted = false;
    int intI = 0;
    int intResult = 1;
    
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Secure Delete v20260418\n");
    printf("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc_a < 2)
    {
        fprintf(stderr, "Usage: %s <file1> [file2] [file3] ...\n", strArgv_a[0]);
        fprintf(stderr, "  Securely deletes one or more files by overwriting with 0xFF then 0x00\n");
        return intResult;
    }
    
    for (intI = 1; intI < argc_a; intI++)
    {
        printf("Deleting: %s\n", strArgv_a[intI]);
        blnDeleted = secureDelete(strArgv_a[intI]);
        
        if (blnDeleted)
        {
            printf("  OK\n");
        }
        else
        {
            printf("  FAILED\n");
            intResult = 1;  // At least one failure
        }
    }
    
    if (intResult == 1 && argc_a == 2)
    {
        return intResult;
    }
    
    return 0;
}