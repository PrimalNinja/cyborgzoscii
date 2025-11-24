// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version - Library Header

#ifndef ZOSCII_H
#define ZOSCII_H

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
                      ConverterFunc cbConverter_a, int intUnmappableChar_a);

// Function to convert PETSCII character codes to ASCII character codes
int petsciiToAscii(int intPetsciiChar_a, int intUnmappableChar_a);

// Function to convert EBCDIC character codes to ASCII character codes
int ebcdicToAscii(int intEbcdicChar_a, int intUnmappableChar_a);

// Function to free the result structure
void freeZOSCIIResult(ZOSCIIResult* result);

#endif /* ZOSCII_H */