// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Structure to represent memory blocks
typedef struct {
    int start;
    int size;
} MemoryBlock;

// Structure to represent the result
typedef struct {
    int* addresses;
    int address_count;
    int* input_counts;
    int* rom_counts;
} ZOSCIIResult;

// Function pointer type for character conversion
typedef int (*ConverterFunc)(int, int);

// Function to convert string to ZOSCII address sequence
ZOSCIIResult toZOSCII(unsigned char* arrBinaryData_a, const char* strInputString_a, 
                      MemoryBlock* arrMemoryBlocks_a, int memoryBlockCount,
                      ConverterFunc cbConverter_a, int intUnmappableChar_a) {
    
    clock_t intStartTime = clock();
    
    int intI;
    int intBlock;
    int intResultIndex = 0;
    int intResultCount = 0;
    int intDebugMissing = 0;
    
    int arrByteCounts[256] = {0};
    int** arrByteAddresses = malloc(256 * sizeof(int*));
    int arrOffsets[256] = {0};
    int arrInputCounts[256] = {0};
    int intAddress;
    int intByte;
    int intIndex;
    MemoryBlock objBlock;
    
    // Initialize byte address arrays to NULL
    for (intI = 0; intI < 256; intI++) {
        arrByteAddresses[intI] = NULL;
    }
    
    // Pass 1: Count occurrences by iterating through blocks
    for (intBlock = 0; intBlock < memoryBlockCount; intBlock++) {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++) {
            intByte = arrBinaryData_a[intAddress];
            arrByteCounts[intByte]++;
        }
    }
    
    // Pass 2: Pre-allocate exact-sized arrays
    for (intI = 0; intI < 256; intI++) {
        if (arrByteCounts[intI] > 0) {
            arrByteAddresses[intI] = malloc(arrByteCounts[intI] * sizeof(int));
        }
    }
    
    // Pass 3: Populate arrays by iterating through blocks
    for (intBlock = 0; intBlock < memoryBlockCount; intBlock++) {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++) {
            intByte = arrBinaryData_a[intAddress];
            arrByteAddresses[intByte][arrOffsets[intByte]] = intAddress;
            arrOffsets[intByte]++;
        }
    }
    
    int strLength = strlen(strInputString_a);
    
    // Count valid characters for result array size
    for (intI = 0; intI < strLength; intI++) {
        intIndex = (unsigned char)strInputString_a[intI];
        if (cbConverter_a) {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }
        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteCounts[intIndex] > 0) {
            intResultCount++;
        } else {
            intDebugMissing++;
            if (intDebugMissing <= 10) {
                printf("Missing character: '%c' (code %d -> %d)\n", 
                       strInputString_a[intI], (unsigned char)strInputString_a[intI], intIndex);
            }
        }
    }

    printf("Characters found in ROM: %d\n", intResultCount);
    printf("Characters missing from ROM: %d\n", intDebugMissing);

    int* arrResult = malloc(intResultCount * sizeof(int));

    for (intI = 0; intI < strLength; intI++) {
        intIndex = (unsigned char)strInputString_a[intI];
        if (cbConverter_a) {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }

        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteCounts[intIndex] > 0) {
            arrInputCounts[intIndex]++;
            int intRandomPick = rand() % arrByteCounts[intIndex];
            arrResult[intResultIndex] = arrByteAddresses[intIndex][intRandomPick];
            intResultIndex++;
        }
    }

    clock_t intEndTime = clock();
    double intElapsedMs = ((double)(intEndTime - intStartTime) / CLOCKS_PER_SEC) * 1000.0;
    
    printf("ZOSCII Performance:\n");
    printf("- Input length: %d chars\n", strLength);
    printf("- Memory blocks: %d\n", memoryBlockCount);
    printf("- Execution time: %.2fms\n", intElapsedMs);
    printf("- Output addresses: %d\n", intResultCount);
    
    // Prepare result structure
    ZOSCIIResult result;
    result.addresses = arrResult;
    result.address_count = intResultCount;
    result.input_counts = malloc(256 * sizeof(int));
    result.rom_counts = malloc(256 * sizeof(int));
    
    memcpy(result.input_counts, arrInputCounts, 256 * sizeof(int));
    memcpy(result.rom_counts, arrByteCounts, 256 * sizeof(int));
    
    // Clean up temporary arrays
    for (intI = 0; intI < 256; intI++) {
        if (arrByteAddresses[intI]) {
            free(arrByteAddresses[intI]);
        }
    }
    free(arrByteAddresses);
    
    return result;
}

// Function to convert PETSCII character codes to ASCII character codes
int petsciiToAscii(int intPetsciiChar_a, int intUnmappableChar_a) {
    static int arrPetsciiToAsciiMap[256] = {
        // 0-31: Control characters
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        // 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

        // 96-255: Everything else mapped to unmappable
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    
    if (intPetsciiChar_a < 0 || intPetsciiChar_a > 255) {
        return intUnmappableChar_a;
    }
    
    int result = arrPetsciiToAsciiMap[intPetsciiChar_a];
    return (result == -1) ? intUnmappableChar_a : result;
}

// Function to convert EBCDIC character codes to ASCII character codes
int ebcdicToAscii(int intEbcdicChar_a, int intUnmappableChar_a) {
    static int arrEbcdicToAsciiMap[256] = {
        // 0-63: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 64-79: Space and some punctuation
        32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 46, 60, 40, 43, 124,
        
        // 80-95: & and punctuation
        38, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, 36, -1, 41, 59, -1,
        
        // 96-111: - and punctuation
        45, 47, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 37, 95, 62, 63,
        
        // 112-127: More punctuation
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 96, 58, 35, 64, 39, 61, 34,
        
        // 128: Control
        -1,
        
        // 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        // 138-144: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        // 145-153: j-r
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        // 154-161: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        // 170-192: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1,
        
        // 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        // 202-208: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        // 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        // 218-225: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        // 234-239: Control/special
        -1, -1, -1, -1, -1, -1,
        
        // 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        // 250-255: Control/special
        -1, -1, -1, -1, -1, -1
    };
    
    if (intEbcdicChar_a < 0 || intEbcdicChar_a > 255) {
        return intUnmappableChar_a;
    }
    
    int result = arrEbcdicToAsciiMap[intEbcdicChar_a];
    return (result == -1) ? intUnmappableChar_a : result;
}

// Function to free the result structure
void freeZOSCIIResult(ZOSCIIResult* result) {
    if (result->addresses) {
        free(result->addresses);
    }
    if (result->input_counts) {
        free(result->input_counts);
    }
    if (result->rom_counts) {
        free(result->rom_counts);
    }
}

// Example usage
int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Example binary data (you would load your actual ROM data here)
    unsigned char binaryData[1000] = {0}; // Initialize with actual data

    // Example memory blocks
    MemoryBlock blocks[] = {
        {0, 500},
        {500, 500}
    };
    
    int blockCount = sizeof(blocks) / sizeof(blocks[0]);
    
    // Fill binary data with some example ASCII characters for testing
    for (int i = 0; i < 1000; i++) {
        binaryData[i] = 65 + (i % 26); // Fill with A-Z pattern
    }
    
    // Test string
    const char* testString = "HELLO WORLD";
    
    // Call the function with PETSCII converter
    ZOSCIIResult result = toZOSCII(binaryData, testString, blocks, blockCount, 
                                   petsciiToAscii, 42); // Using '*' (42) as unmappable char
    
    printf("\nResult addresses:\n");
    for (int i = 0; i < result.address_count; i++) {
        printf("Address %d: %d\n", i, result.addresses[i]);
    }
    
    printf("\nInput character counts:\n");
    for (int i = 0; i < 256; i++) {
        if (result.input_counts[i] > 0) {
            printf("Character %d ('%c'): %d occurrences\n", i, 
                   (i >= 32 && i <= 126) ? i : '?', result.input_counts[i]);
        }
    }
    
    // Clean up
    freeZOSCIIResult(&result);
    
    return 0;
}
