# ZOSCII Library Files

## Standard Library (Windows/Linux)

**Files:**
- `zoscii-encoder.h` - Header file with function declarations
- `zoscii-encoder.c` - Implementation

**Usage:**
```c
#include "zoscii-encoder.h"

// Load your ROM data
unsigned char romData[16384];
// ... load ROM ...

// Define memory blocks
MemoryBlock blocks[] = {
    {0, 16384}
};

// Initialize random seed
srand(time(NULL));

// Encode a message
ZOSCIIResult result = toZOSCII(romData, "Hello World", blocks, 1, NULL, 42);

// Use result.addresses...

// Free when done
freeZOSCIIResult(&result);
```

## Amiga Library

**Files:**
- `zoscii-amiga.h` - Amiga-specific header
- `zoscii-amiga.c` - Implementation with enhanced memory checking

**Differences from standard:**
- More robust error checking
- Returns empty result on allocation failure
- Better suited for memory-constrained Amiga systems

**Usage:** Same as standard library

## CP/M Version

**Files:**
- `cyborgzoscii-cpm.c` - Complete standalone program

**Note:** The CP/M version is NOT structured as an includable library due to:
- Severe memory constraints (64KB total TPA)
- Hardcoded filenames for Small-C compatibility
- Custom memory management for CP/M's TPA layout
- Designed as a complete standalone tool

The CP/M version should be used as-is as a standalone encoding tool.

## Character Converters

Both standard and Amiga libraries include:
- `petsciiToAscii()` - Convert PETSCII to ASCII
- `ebcdicToAscii()` - Convert EBCDIC to ASCII

Pass these as the `ConverterFunc` parameter, or `NULL` for no conversion.

## Compilation Examples

**Standard:**
```bash
gcc -c zoscii.c -o zoscii.o
gcc myprogram.c zoscii.o -o myprogram
```

**Amiga:**
```bash
m68k-amigaos-gcc -c zoscii-amiga.c -o zoscii-amiga.o
m68k-amigaos-gcc myprogram.c zoscii-amiga.o -o myprogram
```

**CP/M:**
```bash
# Use Small-C compiler on CP/M system
cc cyborgzoscii-cpm.c
```