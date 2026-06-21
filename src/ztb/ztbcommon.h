// Cyborg ZTB Common Definitions v20260618
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

#ifndef ZTB_COMMON_H
#define ZTB_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #define FILENAME_MAX 260
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

// --- Constants (match clsZTB exactly) ---
#define ROM_SIZE            65536
#define MIN_PAYLOAD_SIZE    512
#define MAX_HISTORY_BLOCKS  64
#define ROM_ENTRY_SIZE      1024
#define NULL_GUID           "00000000-0000-0000-0000-000000000000"
#define GUID_LEN            37

// --- Block type values (match ZTBBlockType enum) ---
#define BLOCK_TYPE_GENESIS      0
#define BLOCK_TYPE_NORMAL       1
#define BLOCK_TYPE_CHECKPOINT   2
#define BLOCK_TYPE_TRUNCATION   3
#define BLOCK_TYPE_FINALISE     4
#define BLOCK_TYPE_BRIDGE       5

// --- Hash type values (match ZTBHashType enum) ---
#define HASH_TYPE_CRC32_FULL    0
#define HASH_TYPE_CRC32_1KB     1
#define HASH_TYPE_ROLL_FULL     2
#define HASH_TYPE_ROLL_1KB      3

// --- Raw header layout (111 bytes, unencoded, matches clsZTB offsets) ---
// byte  0:      block_type
// byte  1:      block_version (=1)
// byte  2:      is_branch
// bytes 3-38:   trunk_id  (36 bytes ASCII)
// bytes 39-74:  block_id  (36 bytes ASCII)
// bytes 75-110: prev_block_id (36 bytes ASCII)
#define HEADER_RAW_SIZE     111
#define RAW_OFF_BLOCK_TYPE  0
#define RAW_OFF_BLOCK_VER   1
#define RAW_OFF_IS_BRANCH   2
#define RAW_OFF_TRUNK_ID    3
#define RAW_OFF_BLOCK_ID    39
#define RAW_OFF_PREV_ID     75
#define BLOCK_VERSION       1

// --- Encoded section layout (after ZOSCII decode) ---
// byte  0:      hash_type
// bytes 1-4:    hash (uint32 LE)
// bytes 5-8:    prev_hash (uint32 LE)
// bytes 9-12:   payload_len (uint32 LE)
// bytes 13-16:  padded_len (uint32 LE)
// bytes 17+:    padded payload
#define ENC_OFF_HASH_TYPE   0
#define ENC_OFF_HASH        1
#define ENC_OFF_PREV_HASH   5
#define ENC_OFF_PAYLOAD_LEN 9
#define ENC_OFF_PADDED_LEN  13
#define ENC_OFF_PAYLOAD     17
#define ENC_HEADER_SIZE     17

// --- CRC32 ---
uint32_t calculate_crc32(const uint8_t *byData_a, int intOffset_a, int intLen_a);

// --- XorShift32 (matches clsZTB.XorShift32) ---
uint32_t xorshift32(uint32_t intState_a);

// --- ZOSCII encode/decode ---
uint8_t* zoscii_encode(const uint8_t *byRom_a, const uint8_t *byData_a,
                       int intLen_a, int *intEncodedLen_a);
uint8_t* zoscii_decode(const uint8_t *byRom_a, const uint8_t *byData_a,
                       int intOffset_a, int intLen_a, int *intDecodedLen_a);

// --- Load a block file: <workdir>/<blockID>.ztb ---
uint8_t* load_block(const char *strWorkDir_a, const char *strBlockID_a, int *intLen_a);

// --- Find genesis: scan workdir for a .ztb file of exactly ROM_SIZE bytes ---
uint8_t* find_genesis(const char *strWorkDir_a);

// --- Build rolling ROM (matches clsZTB.BuildRollingROM) ---
uint8_t* build_rolling_rom(const char *strWorkDir_a, const char *strPrevBlockID_a);

// --- Read fixed string from raw header ---
void read_fixed_string(const uint8_t *byData_a, int intOffset_a, int intLen_a,
                       char *strOut_a);

// --- Write fixed string into raw header ---
void write_fixed_string(uint8_t *byData_a, int intOffset_a, int intLen_a,
                        const char *strValue_a);

// --- Hash bytes (CRC32 only; rolling hash stub) ---
uint32_t hash_bytes(int intHashType_a, const uint8_t *byData_a, int intOffset_a,
                    int intLen_a);

#endif // ZTB_COMMON_H