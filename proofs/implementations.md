# ZOSCII: Zero Overhead Secure Code Information Interchange

I(M; A) = 0

100% secure Information Theoretic encoding. Quantum Proof Now!

ZOSCII principle, you lookup bytes of a data package (file or message) to a random ROM (from an outside observer's point of view) to random instances of each byte value.  Basically random numbers pointing to random numbers. Unlimited use of the ROM with no secrets known.

Official site: zoscii.com
Official github: https://github.com/PrimalNinja/cyborgzoscii
License: MIT License

# Available for?

Implementations in the GitHub for: CP/M, Windows, Linus, AmigaOS, C, C#, JS, Python, Commodore 64, Commodore Vic 20, Intel 4004 etc...

# Why I(M; A) = 0 and not I(M; C) = 0?

ZOSCII isn't encryption, the C is for a piece of ciphertext. Since ZOSCII has no ciphertext, we use A for address (which is what ZOSCII has)

# Why is ZOSCII not encryption?

The simplest explanation is that encryption requires decryption, and ld a, (hl) is not decryption but reading a byte of memory.  To decode a single byte of ZOSCII on a Z80 is simply... ld a, (hl) - so there is no decryption, therefore no encryption.  It's encoding.

# Isn't ZOSCII just a OTP?

No, a OTP is encryption, it XORs fixed positions of a message to fixed positions of a cipher. This means that a OTP has limited re-use (they call it one-time pad - also a misnomer, someone can read it too obviously, now it's used at least twice).

ZOSCII uses random instances within a random ROM, creating a file of address indrections that have no mathematical pattern and can be stored publicly with no guaranteed knowledge of the message.  A simple 5 byte message has 1,000,000,000,000 plausible combinations.  ZOSCII keyspace is also exponential based on message length, meaning encoding of e.g. 2.5mb Gone with the Wind which everyone in Australia is LEGALLY allowed to copy and poke their tongue out at the USA while doing so - ends up with 10^5500000+ keyspace.

# Benefits of ZOSCII:

https://zoscii.com/why-en.html

# How big should the ROM be?

We recommend a good entropy 64KB for the ROM for ZOSCII (most encoders use the first 64KB of whatever ROM file you choose - photos are usually perfect and are usually analysed by the analyser).  Why 64KB? because bytes turn to words - why waste address space?  We do NOT require perfect entropy or randomness for 64KB, in fact being imperfect is a benefit because it makes for different encodings to be more random in a different way.  But we can never be an unseeded random generator which might generate the same sequence repeatedly.

Why use less than 64KB then?  The reason is to cater for smaller computers as noted below.  Really I would recommend 16KB or 8KB, but 4KB can still give perfect secrecy if you ensure the randomness is good enough and the entropy is perfect.

# What is microZOSCII?

MicroZOSCII is a variant of ZOSCII using 4bit bytes or nibbles (on most computers). Aren't bytes always 8 bit? No! They are ALMOST 100% of the time 8 bit. Using nibbles allows microZOSCII to have a 256byte or less ROM with the same combinatoric probability as full ZOSCII - but the entropy and randomness are very important. microZOSCII is primarily useful for bootstrapping a full 64KB ROM from a small 2 x 54 character string, or 3 x 54 character string - these combined 108 characters or 162 characters allow for strage in session cookies, phone contact lists, instant messager lists or split among personnel on a submarine for a simple boostrap to be QRCode scanned, typed in, NFC scanned etc. 

MicroZOSCII is under UNINTELLIGENCE licence here: https://github.com/PrimalNinja/cyborgzoscii-u

## Vic 20 (with 32KB expansion):

10 PRINT CHR$(147)
20 PRINT "ZOSCII VIC-20"
30 DIM C(25),A(25,10)
40 DIM R(255)
50 PRINT "MAKING ROM"
60 FOR I=0 TO 255
70 R(I)=INT(RND(1)*26)
80 NEXT
90 PRINT "INDEXING"
100 FOR I=0 TO 255
110 B=R(I)
120 IF C(B)<11 THEN A(B,C(B))=I:C(B)=C(B)+1
130 NEXT
140 PRINT "READY A-Z"
150 INPUT "MSG";M$
160 IF M$="" THEN 150
170 PRINT "ENC:";
180 REM
190 FOR N=1 TO LEN(M$)
200 B=ASC(MID$(M$,N,1))-65
210 IF B<0 OR B>25 THEN PRINT "?";:GOTO 240
220 IF C(B)=0 THEN PRINT "?";:GOTO 240
230 PRINT A(B,INT(RND(1)*C(B)));" ";
240 NEXT:PRINT
250 GOTO 150

## Commodore 64:

5 POKE 52,128:POKE 56,128:CLR:REM PROTECT ABOVE $8000
10 PRINT CHR$(147);CHR$(5);"--- ZOSCII 2D C64 (4K) ---"
15 REM ** ROM AT $C000, ARRAYS AT $8000-$BFFF **
20 DIM C%(255),AD%(255,5)
25 REM ** ARRAYS NOW ALLOCATED ABOVE $8000 **
30 RO=49152:REM ROM AT $C000
40 PRINT "1. GENERATING 4KB ENTROPY..."
45 REM ** SEED RNG WITH TIMER **
50 X=RND(-TI)
55 REM ** FILL ROM WITH RANDOM BYTES **
60 FOR I=0 TO 4095:POKE RO+I,INT(RND(1)*256):NEXT
70 PRINT "2. INDEXING..."
80 FOR I=0 TO 4095:B=PEEK(RO+I)
90 IF C%(B)<6 THEN AD%(B,C%(B))=I:C%(B)=C%(B)+1
100 IF (I AND 255)=0 THEN PRINT ".";
110 NEXT
120 PRINT:PRINT "3. LINK READY. I(M;A)=0"
130 INPUT "MSG";M$:IF M$="" THEN END
140 PRINT "ENCODED:"
150 FOR N=1 TO LEN(M$)
160 B=ASC(MID$(M$,N,1))
170 IF C%(B)=0 THEN PRINT "?";:GOTO 200
180 S=INT(RND(1)*C%(B)):E=AD%(B,S)
190 PRINT E;" ";
200 NEXT:PRINT:GOTO 130

## Amstrad CPC:

10 MEMORY &7FFF:MODE 1:DEFINT a-z:RANDOMIZE TIME
20 DIM c(255),ad(255,39):REM 40 instances per byte
30 raw=&8000:REM 8KB ROM starts here
40 PRINT "--- ZOSCII 2D CPC ENGINE ---"
50 PRINT "1. GENERATING 8KB ENTROPY..."
60 FOR i=0 TO 8191:b=INT(RND*256):POKE raw+i,b:NEXT
70 PRINT "2. INDEXING TO 2D ARRAY..."
80 FOR i=0 TO 8191
90  by=PEEK(raw+i)
100 IF c(by)<40 THEN ad(by,c(by))=i:c(by)=c(by)+1
110 NEXT
120 PRINT "3. LINK READY. $I(M;A)=0$"
130 REM --- Main Interactive Loop ---
140 PRINT:INPUT "MSG: ",m$:IF m$="" THEN END
150 L=LEN(m$):DIM en(L):REM Redimensioned per message
160 PRINT "ENCODED: ";
170 FOR n=1 TO L:by=ASC(MID$(m$,n,1))
180  IF c(by)=0 THEN PRINT "SKIP:";CHR$(by);:GOTO 210
190  slot=INT(RND*c(by))
200  en(n)=ad(by,slot):PRINT HEX$(en(n),3);" ";
210 NEXT:PRINT
220 PRINT "DECODED: ";
230 FOR n=1 TO L:PRINT CHR$(PEEK(raw+en(n)));:NEXT
240 PRINT:ERASE en:GOTO 140

## Simplest JS (not fastest):

encode = (r,m) => [...m].map(c => [...r].map((b,i)=>b==c?i:[]).flat().sort(()=>Math.random()-.5)[0]);

decode = (r,a) => a.map(a => r[a]).join('');

## Faster JS (for 1 encoding, not repeated encodings):

// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Function to convert string or binary data to ZOSCII address sequence
// arrBinaryData_a: Uint8Array containing the ROM/binary data  
// mixedInputData_a: String or Uint8Array containing the data to convert
// arrMemoryBlocks_a: array of {start: startAddress, size: blockSize} objects
// cbConverter_a: encoding conversion function (e.g., petsciiToAscii, ebcdicToAscii) or null
// intUnmappableChar_a: the native character code to be used if it cannot be mapped to ASCII
// Returns: {addresses: array, inputCounts: array, romCounts: array}

function toZOSCII(arrBinaryData_a, mixedInputData_a, arrMemoryBlocks_a, cbConverter_a, intUnmappableChar_a)
{
    var intStartTime = new Date().getTime();
    
    var intI;
    var intBlock;
    var intResultIndex = 0;
    var intResultCount = 0;
    var intDebugMissing = 0;
    
    var arrByteCounts = new Array(256);
    var arrByteAddresses = new Array(256);
    var arrOffsets = new Array(256);
    var arrInputCounts = new Array(256);
    var intAddress;
    var intByte;
    var intIndex;
    var objBlock;
    
    // Convert input to consistent format
    var arrInputData_a;
    var blnIsString = false;
    
    if (typeof mixedInputData_a === 'string') 
    {
        // Handle string input - convert to UTF-8 bytes
        arrInputData_a = new TextEncoder().encode(mixedInputData_a);
        blnIsString = true;
    } 
    else 
    {
        // Handle Uint8Array input
        arrInputData_a = mixedInputData_a;
        blnIsString = false;
    }
    
    // Initialize counters
    for (intI = 0; intI < 256; intI++)
    {
        arrByteCounts[intI] = 0;
        arrInputCounts[intI] = 0;
    }
    
    // Pass 1: Count occurrences by iterating through blocks
    for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
    {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
        {
            intByte = arrBinaryData_a[intAddress];
            arrByteCounts[intByte]++;
        }
    }
    
    // Pass 2: Pre-allocate exact-sized arrays
    for (intI = 0; intI < 256; intI++)
    {
        arrByteAddresses[intI] = new Array(arrByteCounts[intI]);
        arrOffsets[intI] = 0;
    }
    
    // Pass 3: Populate arrays by iterating through blocks
    for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
    {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
        {
            intByte = arrBinaryData_a[intAddress];
            arrByteAddresses[intByte][arrOffsets[intByte]] = intAddress;
            arrOffsets[intByte]++;
        }
    }
    
    // Build result array with random addresses - pre-allocate and avoid push()
    for (intI = 0; intI < arrInputData_a.length; intI++)
    {
        intIndex = arrInputData_a[intI];  // Direct byte value
        
        // Apply encoding conversion if provided
        if (cbConverter_a)
        {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }
        
        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
        {
            intResultCount++;
        }
        else
        {
            intDebugMissing++;
            if (intDebugMissing <= 10)
            {
                var strHexByte = arrInputData_a[intI].toString(16).toUpperCase();
                if (strHexByte.length < 2) strHexByte = "0" + strHexByte;
                
                if (blnIsString) 
                {
                    console.log("Missing character: '" + String.fromCharCode(arrInputData_a[intI]) + "' (code " + arrInputData_a[intI] + "/0x" + strHexByte + " -> " + intIndex + ")");
                } 
                else 
                {
                    console.log("Missing byte: " + arrInputData_a[intI] + " (0x" + strHexByte + " -> " + intIndex + ")");
                }
            }
        }
    }

    if (blnIsString) 
    {
        console.log("Characters found in ROM: " + intResultCount);
        console.log("Characters missing from ROM: " + intDebugMissing);
    } 
    else 
    {
        console.log("Bytes found in ROM: " + intResultCount);
        console.log("Bytes missing from ROM: " + intDebugMissing);
    }

    var arrResult = new Array(intResultCount);

    for (intI = 0; intI < arrInputData_a.length; intI++)
    {
        intIndex = arrInputData_a[intI];  // Direct byte value
        
        // Apply encoding conversion if provided
        if (cbConverter_a)
        {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }

        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
        {
            arrInputCounts[intIndex]++;
            var intRandomPick = Math.floor(Math.random() * arrByteAddresses[intIndex].length);
            arrResult[intResultIndex] = arrByteAddresses[intIndex][intRandomPick];
            intResultIndex++;
        }
    }

    var intEndTime = new Date().getTime();
    var intElapsedMs = intEndTime - intStartTime;
    
    console.log("ZOSCII Performance:");
    console.log("- Binary size: " + arrBinaryData_a.length + " bytes");
    console.log("- Input length: " + arrInputData_a.length + (blnIsString ? " characters" : " bytes"));
    console.log("- Memory blocks: " + arrMemoryBlocks_a.length);
    console.log("- Execution time: " + intElapsedMs + "ms");
    console.log("- Output addresses: " + arrResult.length);
    
    return {
        addresses: arrResult,
        inputCounts: arrInputCounts,
        romCounts: arrByteCounts
    };
}

## Modern C:

// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

typedef struct 
{
    uint32_t* addresses;
    uint32_t count;
} ByteAddresses;

int main(int intArgC_a, char *arrArgs_a[]) 
{
#ifdef _WIN32
    // Set binary mode for stdin/stdout if needed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Encoder\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    int intBittage = 16;  // default
    int intOffset = 0;
    
    if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-32") == 0) 
	{
        intBittage = 32;
        intOffset = 1;
    } 
	else if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-16") == 0) 
	{
        intBittage = 16;
        intOffset = 1;
    }
    
    if (intArgC_a != 4 + intOffset) 
	{
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile> <encodedoutput>\n", arrArgs_a[0]);
        return 1;
    }
    
    srand(time(NULL));
    
    FILE* fROM = fopen(arrArgs_a[1 + intOffset], "rb");
    if (!fROM) 
	{
        perror("Error opening ROM file");
        return 1;
    }
    
    fseek(fROM, 0, SEEK_END);
    long lngROMSize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    
    // Check ROM size limit based on bit width
    long lngMaxSize = (intBittage == 16) ? 65536 : 4294967296L;
    if (lngROMSize > lngMaxSize) 
	{
        lngROMSize = lngMaxSize;
    }
    
    uint8_t* pROMData = malloc(lngROMSize);
    fread(pROMData, 1, lngROMSize, fROM);
    fclose(fROM);
    
    // Build address lookup tables
    ByteAddresses arrLookup[256];
    uint32_t arrROMCounts[256] = {0};
    
    // Count occurrences
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        arrROMCounts[pROMData[lngI]]++;
    }
    
    // Allocate address arrays
    for (int intI = 0; intI < 256; intI++) 
	{
        arrLookup[intI].addresses = malloc(arrROMCounts[intI] * sizeof(uint32_t));
        arrLookup[intI].count = 0;
    }
    
    // Populate address arrays
    for (long lngI = 0; lngI < lngROMSize; lngI++) 
	{
        uint8_t by = pROMData[lngI];
        arrLookup[by].addresses[arrLookup[by].count++] = lngI;
    }
    
    FILE* fInput = fopen(arrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening input file");
        return 1;
    }
    
    FILE* fOutput = fopen(arrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        return 1;
    }
    
    int ch;
    while ((ch = fgetc(fInput)) != EOF) 
	{
        uint8_t by = (uint8_t)ch;
        if (arrLookup[by].count > 0) 
		{
            uint32_t intRandomIdx = rand() % arrLookup[by].count;
            uint32_t intAddress = arrLookup[by].addresses[intRandomIdx];
            
            if (intBittage == 16) 
			{
                uint16_t intAddress16 = (uint16_t)intAddress;
                fwrite(&intAddress16, sizeof(uint16_t), 1, fOutput);
            } 
			else 
			{
                fwrite(&intAddress, sizeof(uint32_t), 1, fOutput);
            }
        }
    }
    
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    for (int intI = 0; intI < 256; intI++) 
	{
        free(arrLookup[intI].addresses);
    }
    
    return 0;
}

// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

int main(int intArgC_a, char* arrArgs_a[]) 
{
#ifdef _WIN32
    // Set binary mode for stdin/stdout if needed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    printf("ZOSCII Decoder\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    int intBittage = 16;  // default
    int intOffset = 0;
    
    if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-32") == 0) 
	{
        intBittage = 32;
        intOffset = 1;
    } 
	else if (intArgC_a >= 2 && strcmp(arrArgs_a[1], "-16") == 0) 
	{
        intBittage = 16;
        intOffset = 1;
    }
    
    if (intArgC_a != 4 + intOffset) 
	{
        fprintf(stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", arrArgs_a[0]);
        return 1;
    }
    
    FILE* fROM = fopen(arrArgs_a[1 + intOffset], "rb");
    if (!fROM) 
	{
        perror("Error opening ROM file");
        return 1;
    }
    
    fseek(fROM, 0, SEEK_END);
    long lngROMSize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    
    // Check ROM size limit based on bit width
    long lngMaxSize = (intBittage == 16) ? 65536 : 4294967296L;
    if (lngROMSize > lngMaxSize) 
	{
        lngROMSize = lngMaxSize;
    }
    
    uint8_t* pROMData = malloc(lngROMSize);
    fread(pROMData, 1, lngROMSize, fROM);
    fclose(fROM);
    
    FILE* fInput = fopen(arrArgs_a[2 + intOffset], "rb");
    if (!fInput) 
	{
        perror("Error opening encoded input file");
        return 1;
    }
    
    FILE* fOutput = fopen(arrArgs_a[3 + intOffset], "wb");
    if (!fOutput) 
	{
        perror("Error opening output file");
        return 1;
    }
    
    if (intBittage == 16) 
	{
        uint16_t intAddress16;
        while (fread(&intAddress16, sizeof(uint16_t), 1, fInput) == 1) 
		{
            if (intAddress16 < lngROMSize) 
			{
                fputc(pROMData[intAddress16], fOutput);
            }
        }
    } 
	else 
	{
        uint32_t intAddress;
        while (fread(&intAddress, sizeof(uint32_t), 1, fInput) == 1) 
		{
            if (intAddress < lngROMSize) 
			{
                fputc(pROMData[intAddress], fOutput);
            }
        }
    }
    
    fclose(fInput);
    fclose(fOutput);
    free(pROMData);
    
    return 0;
}

## CUDA (with Tri-Sword):

%%writefile zoscii_trisword.cu
#include <cuda_runtime.h>
#include <torch/extension.h>
#include <stdint.h>

// Tri-Sword ZOSCII Handle - Sovereign Engine Structure
struct ZOSCIIHandle {
    int8_t* ptr_rom;
    int32_t* ptr_all_positions;
    int32_t* ptr_rand_states;
    int int_max_batch;
    int int_total_positions;
};

// Constant Memory for High-Speed Silicon Lookup
__constant__ int32_t ARR_POSITION_OFFSETS[256];
__constant__ int32_t ARR_POSITION_LENGTHS[256];

// Xorshift32: Branchless Entropy Generation
__device__ inline int32_t fast_rand(int32_t& int_state) {
    int_state ^= int_state << 13;
    int_state ^= int_state >> 17; 
    int_state ^= int_state << 5;
    return int_state & 0x7FFFFFFF;
}

// ZOSCII Encode: Warp-Centric Logic
__global__ void zoscii_encode_kernel(
    int8_t* ptr_messages,
    int32_t* ptr_encoded,
    int32_t* ptr_rand_states,
    int32_t* ptr_lookup_table,
    int int_batch_size,
    int int_msg_len
) {
    // 64-bit Addressing to bypass 2.1B boundary
    size_t sz_warp_id = (size_t)((blockIdx.x * blockDim.x + threadIdx.x) / 32);
    int int_lane_id = threadIdx.x % 32;
    
    if (sz_warp_id >= (size_t)int_batch_size) return;
    
    int32_t int_rand_state = ptr_rand_states[sz_warp_id];
    int int_per_thread = (int_msg_len + 31) / 32;
    int int_start = int_lane_id * int_per_thread;
    int int_end = min(int_start + int_per_thread, int_msg_len);
    
    size_t sz_base_idx = sz_warp_id * (size_t)int_msg_len;
    
    for (int i = int_start; i < int_end; i++) {
        int int_byte = ptr_messages[sz_base_idx + i] & 0xFF;
        int int_off = ARR_POSITION_OFFSETS[int_byte];
        int int_len = ARR_POSITION_LENGTHS[int_byte];
        
        if (int_len > 0) {
            int32_t int_val = fast_rand(int_rand_state);
            ptr_encoded[sz_base_idx + i] = ptr_lookup_table[int_off + (int_val % int_len)];
        }
    }
    ptr_rand_states[sz_warp_id] = int_rand_state;
}

// ZOSCII Decode: Silicon "Slash" Path
__global__ void zoscii_decode_kernel(
    int32_t* ptr_encoded,
    int8_t* ptr_decoded,
    int8_t* ptr_rom,
    int int_batch_size,
    int int_msg_len
) {
    size_t sz_idx = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    size_t sz_total = (size_t)int_batch_size * int_msg_len;
    
    for (size_t i = sz_idx; i < sz_total; i += (size_t)gridDim.x * blockDim.x) {
        ptr_decoded[i] = ptr_rom[ptr_encoded[i] & 0xFFFF];
    }
}

intptr_t sharpen(int int_max_batch, torch::Tensor obj_rom_tensor, int int_rom_size) {
    ZOSCIIHandle* obj_h = new ZOSCIIHandle;
    obj_h->int_max_batch = int_max_batch;
    int8_t* ptr_rom_cpu = obj_rom_tensor.data_ptr<int8_t>();

    cudaMalloc(&obj_h->ptr_rom, int_rom_size);
    cudaMemcpy(obj_h->ptr_rom, ptr_rom_cpu, int_rom_size, cudaMemcpyHostToDevice);

    int32_t arr_offsets[256] = {0};
    int32_t arr_lengths[256] = {0};
    for (int i = 0; i < int_rom_size; i++) { arr_lengths[ptr_rom_cpu[i] & 0xFF]++; }

    int int_total_accum = 0;
    for (int i = 0; i < 256; i++) {
        arr_offsets[i] = int_total_accum;
        int_total_accum += arr_lengths[i];
    }
    obj_h->int_total_positions = int_total_accum;

    int32_t* arr_pos_cpu = new int32_t[int_total_accum];
    int32_t arr_curr[256] = {0};
    for (int i = 0; i < int_rom_size; i++) {
        int b = ptr_rom_cpu[i] & 0xFF;
        arr_pos_cpu[arr_offsets[b] + arr_curr[b]++] = i;
    }

    cudaMemcpyToSymbol(ARR_POSITION_OFFSETS, arr_offsets, 256 * sizeof(int32_t));
    cudaMemcpyToSymbol(ARR_POSITION_LENGTHS, arr_lengths, 256 * sizeof(int32_t));
    cudaMalloc(&obj_h->ptr_all_positions, int_total_accum * sizeof(int32_t));
    cudaMemcpy(obj_h->ptr_all_positions, arr_pos_cpu, int_total_accum * sizeof(int32_t), cudaMemcpyHostToDevice);
    
    cudaMalloc(&obj_h->ptr_rand_states, int_max_batch * sizeof(int32_t));
    int32_t* arr_seeds = new int32_t[int_max_batch];
    for (int i = 0; i < int_max_batch; i++) { arr_seeds[i] = i * 0x9e3779b1 + 12345; }
    cudaMemcpy(obj_h->ptr_rand_states, arr_seeds, int_max_batch * sizeof(int32_t), cudaMemcpyHostToDevice);

    delete[] arr_pos_cpu;
    delete[] arr_seeds;
    return reinterpret_cast<intptr_t>(obj_h);
}

void sheath(intptr_t int_handle) {
    ZOSCIIHandle* obj_h = reinterpret_cast<ZOSCIIHandle*>(int_handle);
    if (obj_h) {
        cudaFree(obj_h->ptr_rom);
        cudaFree(obj_h->ptr_all_positions);
        cudaFree(obj_h->ptr_rand_states);
        delete obj_h;
    }
}

torch::Tensor slash_encode(intptr_t int_handle, torch::Tensor obj_msg) {
    ZOSCIIHandle* obj_h = reinterpret_cast<ZOSCIIHandle*>(int_handle);
    int int_batch = obj_msg.size(0);
    int int_len = obj_msg.size(1);
    auto obj_enc = torch::zeros({int_batch, int_len}, torch::dtype(torch::kInt32).device(obj_msg.device()));

    int int_threads = 256;
    int int_blocks = (int)((int_batch * 32LL + int_threads - 1) / int_threads);

    zoscii_encode_kernel<<<int_blocks, int_threads>>>(
        obj_msg.data_ptr<int8_t>(),
        obj_enc.data_ptr<int32_t>(),
        obj_h->ptr_rand_states,
        obj_h->ptr_all_positions,
        int_batch,
        int_len
    );
    return obj_enc;
}

torch::Tensor slash_decode(intptr_t int_handle, torch::Tensor obj_enc) {
    ZOSCIIHandle* obj_h = reinterpret_cast<ZOSCIIHandle*>(int_handle);
    int int_batch = obj_enc.size(0);
    int int_len = obj_enc.size(1);
    auto obj_dec = torch::zeros({int_batch, int_len}, torch::dtype(torch::kInt8).device(obj_enc.device()));

    size_t sz_total = (size_t)int_batch * (size_t)int_len;
    int int_threads = 256;
    int int_blocks = (int)((sz_total + int_threads - 1) / int_threads);

    zoscii_decode_kernel<<<int_blocks, int_threads>>>(
        obj_enc.data_ptr<int32_t>(),
        obj_dec.data_ptr<int8_t>(),
        obj_h->ptr_rom,
        int_batch,
        int_len
    );
    return obj_dec;
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("sharpen", &sharpen);
    m.def("sheath", &sheath);
    m.def("slash_encode", &slash_encode);
    m.def("slash_decode", &slash_decode);
}


# Performance

======================================================================
⚡ JAVASCRIPT CRYPTO PERFORMANCE COMPARISON
   (CryptoJS 4.1.1 vs ZOSCII)
======================================================================
📊 PERFORMANCE SUMMARY (Operations/Second):
System              Encode/Encrypt   Decode/Decrypt   Size Ratio
─────────────────────────────────────────────────────────────────
ZOSCII                217,391        416,666   2.00x
AES-256                11,363         12,820   1.42x
SHA-256                53,763   (one-way only)   N/A
🎯 KEY FINDINGS:
ENCODING/ENCRYPTION SPEED:
  • ZOSCII encode is 19.1x FASTER than AES-256 encrypt
DECODING/DECRYPTION SPEED:
  • ZOSCII decode is 32.5x FASTER than AES-256 decrypt
💡 OBSERVATIONS:
  • ZOSCII decode (416,666 ops/sec) is 1.9x faster than encode
  • This is because decode is simple ROM lookup, encode requires random selection
  • AES encrypt/decrypt speeds are similar (balanced cipher operations)
✓ Correctness: ZOSCII=PASS, AES=PASS

content# python zoscii_testharness.py 
JIT compiling ZOSCII Tri-Sword Engine...
ninja: no work to do.

==========================================================================================
ZOSCII 64KB TRI-SWORD PERFORMANCE AUDIT
==========================================================================================
Batch      | Encode (ms)  | Decode (ms)  | Encode MB/s  | Decode MB/s  | JS Encode x  | Correct
--------------------------------------------------------------------------------------------------------------
ZOSCII Tri-Sword initialized with 5,000 batch capacity
5000       |       0.75 |       0.31 |     3419.4 |     8169.1 |      16140x | ✓
ZOSCII Tri-Sword initialized with 50,000 batch capacity
50000      |       6.13 |       2.01 |     4175.9 |    12765.1 |      19710x | ✓
ZOSCII Tri-Sword initialized with 500,000 batch capacity
500000     |      58.16 |      17.79 |     4401.7 |    14392.6 |      20776x | ✓
ZOSCII Tri-Sword initialized with 2,000,000 batch capacity
2000000    |     128.13 |      41.91 |     7992.1 |    24434.3 |      37723x | ✓
ZOSCII Tri-Sword initialized with 3,500,000 batch capacity
3500000    |     335.29 |      71.47 |     5344.6 |    25072.1 |      25227x | ✓

Randomness Verification:
ZOSCII Tri-Sword initialized with 1,000 batch capacity
  Same positions: 2/512 (Ideal: <5)
  True ZOSCII: ✓


Note: 8bit Computer BASIC's are realtime and fast enough for 100% secure chat.  For binary files they are of course slower - especially if streaming from floppy disc - but still faster than every compression tool available to them.





# 4004 (Experimental - not assembled not debugged - need 4004 expert to help!)

Notes: I have no doubt this will be realtime as per interpreted BASICs above - it may be necessary to use an actual physical ROM or ROMsimulator for 4004 if RAM is not enough.

; ============================================
; ZOSCII ENCODER for Intel 4004 - UNTESTED
; Input: Message in INPUT_BUFFER (RAM)
; Output: Address pairs in OUTPUT_BUFFER (RAM)
; Uses: Sequential search with 8-bit instance counter
; ============================================

; Memory map:
; RAM 0x40-0x7F: Input buffer (64 bytes max)
; RAM 0x80-0xFF: Output buffer (128 bytes = 64 address pairs)
; RAM 0x30-0x31: LFSR state for PRNG

; Register allocation:
; R0,R1: Temporary (used by FIN)
; R2,R3: ROM pointer (P1) - 12-bit address
; R4,R5: Input RAM pointer (P2) - 8-bit
; R6,R7: Output RAM pointer (P3) - 8-bit
; R8,R9: Instance counter (8-bit: high, low)
; R10,R11: Target instance (8-bit: high, low)
; R12: Message length counter
; R13: Target character to find
; R14,R15: Scratch

; ============================================
; MAIN ENCODER ENTRY POINT
; ============================================
ENCODE_SETUP:
    ; Initialize message length (must be ≤64)
    FIM P6, 0x40     ; Load 0x40 into R12:R13
    LD R12           ; Get high nibble (4)
    XCH R12          ; R12 = 4 (represents 64 in loop logic)
    
    ; Initialize input pointer (P2 = R4:R5)
    FIM P2, 0x40     ; Input buffer at 0x40
    
    ; Initialize output pointer (P3 = R6:R7)
    FIM P3, 0x80     ; Output buffer at 0x80
    
    ; Initialize PRNG seed
    JMS PRNG_INIT

; ============================================
; MAIN ENCODING LOOP
; ============================================
ENCODE_LOOP:
    ; Get next character from input buffer
    SRC P2           ; Select RAM at R4:R5
    RDM              ; Read character
    XCH R13          ; R13 = target character
    
    ; Get random instance number (0-255)
    JMS PRNG_NEXT    ; Returns 8-bit value in R10:R11
    
    ; Initialize ROM search pointer
    FIM P1, 0x00     ; Start at ROM address 0x000
    
    ; Initialize instance counter to 0
    LDM 0
    XCH R8           ; R8 = 0 (high nibble)
    XCH R9           ; R9 = 0 (low nibble)

; ============================================
; SEQUENTIAL SEARCH LOOP
; ============================================
SEARCH_LOOP:
    ; Read byte from ROM at current address
    SRC P1           ; Set ROM address from R2:R3
    FIN P0           ; Fetch ROM byte into R0:R1
    LD R1            ; Get the data byte (low nibble)
    
    ; Compare with target character
    CLC              ; Clear carry
    SUB R13          ; ACC = ROM[addr] - target
    JCN NZ, NOT_MATCH ; If not zero, not a match
    
    ; Found a match - check if it's the Nth instance
    LD R9            ; Get instance counter low
    CLC
    SUB R11          ; Compare with target instance low
    JCN NZ, NOT_NTH  ; Low nibbles don't match
    
    LD R8            ; Get instance counter high
    CLC
    SUB R10          ; Compare with target instance high
    JCN Z, FOUND_IT  ; Both nibbles match - this is the one!
    
NOT_NTH:
    ; Increment instance counter (8-bit)
    ISZ R9, SEARCH_LOOP ; Inc low nibble, continue if not zero
    ISZ R8, SEARCH_LOOP ; Inc high nibble (carry)
    ; If we overflow 255 instances, wrap to 0 and continue
    JUN SEARCH_LOOP

NOT_MATCH:
    ; Advance ROM pointer (12-bit increment)
    ISZ R2, SEARCH_LOOP ; Inc low nibble
    JCN C, INC_MID      ; If carried out
    JUN SEARCH_LOOP

INC_MID:
    ISZ R3, SEARCH_LOOP ; Inc middle nibble
    ; Note: 4004 has 4K ROM (0x000-0xFFF)
    ; If we exceed 4K, we've failed (should not happen)
    ; Production code should handle this error
    JUN SEARCH_LOOP

; ============================================
; FOUND TARGET - STORE ADDRESS
; ============================================
FOUND_IT:
    ; Store 12-bit ROM address as two bytes
    ; Low byte: R2 (low nibble of address)
    SRC P3           ; Select output RAM
    LD R2            ; Get address low nibble
    WRM              ; Write to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT1    ; Inc low nibble
    ISZ R7, CONT1    ; Inc high on carry
    
CONT1:
    ; High byte: R3 (middle nibble of address)
    SRC P3           ; Update selection (pointer changed)
    LD R3            ; Get address middle nibble
    WRM              ; Write to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT2
    ISZ R7, CONT2
    
CONT2:
    ; Increment input pointer with carry
    ISZ R4, CONT3
    ISZ R5, CONT3
    
CONT3:
    ; Decrement message counter and loop
    ISZ R12, ENCODE_LOOP ; 4004 ISZ skips if result is ZERO
    
    BBL 0            ; Return success

; ============================================
; PRNG: 8-bit Linear Feedback Shift Register
; Uses Galois LFSR with polynomial x^8+x^6+x^5+x^4+1
; Taps at bits 6,5,4
; ============================================
PRNG_INIT:
    ; Initialize LFSR state in RAM 0x30-0x31
    FIM P3, 0x30     ; Point to LFSR storage
    SRC P3
    LDM 0x0A         ; Seed high nibble
    WRM
    
    ISZ R6, PRNGI2
PRNGI2:
    SRC P3
    LDM 0x05         ; Seed low nibble (0xA5)
    WRM
    
    BBL 0

PRNG_NEXT:
    ; Read current state
    FIM P3, 0x30
    SRC P3
    RDM              ; Read high nibble
    XCH R14          ; Store in R14
    
    ISZ R6, PRNGN2
PRNGN2:
    SRC P3
    RDM              ; Read low nibble
    XCH R15          ; Store in R15
    
    ; Perform 8 shift iterations for full byte
    LDM 8
    XCH R13          ; R13 = iteration counter
    
PRNG_SHIFT:
    ; Get LSB of R15
    LD R15
    RAR              ; Rotate right through carry
    JCN C, PRNG_TAP  ; If LSB was 1, apply taps
    
    ; Shift right (R14:R15 >> 1)
    LD R14
    RAR              ; Shift high nibble right
    XCH R14
    LD R15
    RAR              ; Shift low nibble right
    XCH R15
    JUN PRNG_CONT
    
PRNG_TAP:
    ; Apply XOR taps (0x60 = bits 6,5)
    LD R14
    RAR              ; Shift high nibble
    XCH R14
    CLC
    LDM 6            ; Tap bits
    ADD R14
    XCH R14          ; XOR high nibble
    
    LD R15
    RAR
    XCH R15

PRNG_CONT:
    ISZ R13, PRNG_SHIFT
    
    ; Store updated state
    FIM P3, 0x30
    SRC P3
    LD R14
    WRM
    ISZ R6, PRNGN3
PRNGN3:
    SRC P3
    LD R15
    WRM
    
    ; Return value in R10:R11
    LD R14
    XCH R10
    LD R15
    XCH R11
    
    BBL 0
	
; ============================================
; ZOSCII DECODER for Intel 4004 - UNTESTED
; Input: Address pairs in OUTPUT_BUFFER (RAM)
; Output: Plaintext in INPUT_BUFFER (RAM)
; The "airtight" hardware indirection implementation
; ============================================

; Memory map:
; RAM 0x40-0x7F: Output plaintext buffer (64 bytes)
; RAM 0x80-0xFF: Input address buffer (128 bytes = 64 pairs)

; Register allocation:
; R0,R1: Temporary (used by FIN)
; R2,R3: ROM pointer (P1) - reconstructed 12-bit address
; R4,R5: Input address pointer (P2) - 8-bit
; R6,R7: Output plaintext pointer (P3) - 8-bit
; R12: Pair counter (number of address pairs to decode)
; R13,R14,R15: Scratch

; ============================================
; MAIN DECODER ENTRY POINT
; ============================================
DECODE_SETUP:
    ; Initialize pair counter (64 pairs max)
    FIM P6, 0x40     ; Load 0x40 into R12:R13
    LD R12           ; Get high nibble (4)
    XCH R12          ; R12 = 4 (represents 64 pairs)
    
    ; Initialize input pointer (address buffer)
    FIM P2, 0x80     ; Input addresses at 0x80
    
    ; Initialize output pointer (plaintext buffer)
    FIM P3, 0x40     ; Output plaintext at 0x40

; ============================================
; MAIN DECODING LOOP
; ============================================
DECODE_LOOP:
    ; Read address LOW nibble from RAM
    SRC P2           ; Select input address buffer
    RDM              ; Read low nibble
    XCH R2           ; Store in ROM pointer low
    
    ; Increment input pointer with carry
    ISZ R4, CONT1
    ISZ R5, CONT1
    
CONT1:
    ; Read address MID nibble from RAM
    SRC P2           ; Re-select (pointer changed)
    RDM              ; Read middle nibble
    XCH R3           ; Store in ROM pointer mid
    
    ; Note: 4004 has 12-bit addressing (3 nibbles)
    ; We're only storing/reading 2 nibbles (8 bits)
    ; This limits us to 256 ROM addresses
    ; For full 4K ROM, need to store 3 nibbles
    
    ; Increment input pointer with carry
    ISZ R4, CONT2
    ISZ R5, CONT2

CONT2:
    ; ========================================
    ; CORE HARDWARE INDIRECTION
    ; This is the cryptographic primitive:
    ; ROM[address] cannot be predicted without
    ; physical access to the ROM contents
    ; ========================================
    SRC P1           ; Set ROM address from R2:R3
    FIN P0           ; Hardware fetch: ROM[addr] → R0:R1
    
    ; The byte we want is in R1 (low nibble of ROM word)
    LD R1            ; Get the plaintext character
    
    ; Store plaintext byte to output buffer
    SRC P3           ; Select output buffer
    WRM              ; Write character to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT3
    ISZ R7, CONT3

CONT3:
    ; Decrement pair counter and loop
    ; ISZ on 4004 skips next instruction if result is ZERO
    ISZ R12, DECODE_LOOP
    
    BBL 0            ; Return success

; ============================================
; ENHANCED VERSION: Full 12-bit address support
; (Comment out above, uncomment this for 4K ROM)
; ============================================
;DECODE_LOOP_12BIT:
;    ; Read address LOW nibble
;    SRC P2
;    RDM
;    XCH R2
;    ISZ R4, DL1
;    ISZ R5, DL1
;    
;DL1:
;    ; Read address MID nibble  
;    SRC P2
;    RDM
;    XCH R3
;    ISZ R4, DL2
;    ISZ R5, DL2
;
;DL2:
;    ; Read address HIGH nibble (only lower 4 bits used)
;    SRC P2
;    RDM
;    XCH R14          ; Temporarily store high nibble
;    ISZ R4, DL3
;    ISZ R5, DL3
;
;DL3:
;    ; The 4004 SRC command only uses R2:R3 for addressing
;    ; To support 12-bit (4K), need custom ROM access
;    ; This would require bank-switching or external logic
;    
;    ; Standard approach: Use R2:R3 (8-bit = 256 addresses)
;    SRC P1
;    FIN P0
;    LD R1
;    
;    SRC P3
;    WRM
;    ISZ R6, DL4
;    ISZ R7, DL4
;
;DL4:
;    ISZ R12, DECODE_LOOP_12BIT
;    BBL 0

; ============================================
; ERROR HANDLING (Optional)
; ============================================
DECODE_ERROR:
    ; If pointer overflow or other error
    LDM 0x0F         ; Error code
    BBL 0x0F         ; Return with error

; ============================================
; UTILITY: Clear output buffer before decode
; ============================================
CLEAR_OUTPUT:
    FIM P3, 0x40     ; Output buffer start
    LDM 0x04         ; Counter high nibble (64 bytes)
    XCH R12
    
CLR_LOOP:
    SRC P3
    LDM 0            ; Zero
    WRM              ; Write to RAM
    ISZ R6, CLR_LOOP
    ISZ R7, CLR2
CLR2:
    ISZ R12, CLR_LOOP
    BBL 0

; ============================================
; NOTES ON 4004 ARCHITECTURE
; ============================================
; 1. ROM addressing is 12-bit (0x000-0xFFF = 4096 bytes)
;    but SRC only uses R2:R3 (8 bits = 256 addresses)
;    
; 2. FIN instruction: Fetches 8-bit word from ROM
;    into register pair (R0:R1, R2:R3, etc.)
;    Format: [R(n)] = high nibble, [R(n+1)] = low nibble
;
; 3. For full 4K ROM access, need external banking logic
;    or store addresses as 3 nibbles (12 bits)
;
; 4. The security comes from hardware indirection:
;    Without reading the actual ROM chip, an adversary
;    cannot determine ROM[addr] from addr alone
;
; 5. This implementation uses 8-bit addresses (256 slots)
;    which is sufficient for most character sets
; ============================================

