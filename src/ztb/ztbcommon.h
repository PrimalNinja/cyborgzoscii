// Cyborg ZTB Common Definitions v20260420
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
    #include <io.h>
    #include <fcntl.h>
    #include <windows.h>
    #include <process.h>
    #define FILENAME_MAX 260
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <limits.h>
#endif

// --- Constants ---
#define ROM_SIZE 65536
#define GUID_LEN 37
#define MAX_HISTORY_BLOCKS 64
#define BYTES_PER_SAMPLE 1024
#define MAX_BLOCKS_TO_SCAN 2048
#define MIN_PAYLOAD_SIZE 512
#define NULL_GUID "00000000-0000-0000-0000-000000000000"
#define ROM_ENTRY_SIZE 1024
#define CRC32_SIZE 4                    // 4 bytes raw CRC32 value
#define CRC32_ENCODED_SIZE 8            // 4 bytes * 2 (ZOSCII encoded)
#define MODE_SIZE 1                     // 1 byte mode value
#define MODE_ENCODED_SIZE 2             // 1 byte * 2 (ZOSCII encoded)
#define BLOCK_TYPE_SIZE 1               // 1 byte block type value
#define BLOCK_VERSION_SIZE 1            // 1 byte block version value (currently always 1)
#define BLOCK_VERSION_DEFAULT 1         // current block version
#define HASH_TYPE_SIZE 1                // 1 byte hash type value
#define HASH_SIZE 4                     // 4 bytes hash value
#define CRC_PREFIX_ENCODED_SIZE 22      // block_type(1)+block_version(1)+hash_type(1)+hash(4)+prevHash(4) = 11 raw * 2
#define BLOCK_PREFIX_ENCODED_SIZE 24    // mode(1)+block_type(1)+block_version(1)+hash_type(1)+hash(4)+prevHash(4) = 12 raw * 2
#define MODE_NORMAL 0
#define MODE_X1 1                       // prev-block CRC binding, no XOR
#define MODE_X2 2                       // prev-block CRC binding + on-disk XOR

// --- Block types ---
#define BLOCK_TYPE_NORMAL     0         // Standard data block
#define BLOCK_TYPE_CHECKPOINT 1         // Checkpoint marker, optional label payload
#define BLOCK_TYPE_TRUNCATION 2         // Truncation marker, 64KB rolling ROM payload
#define BLOCK_TYPE_FINALISE   3         // Soft finalise, no further blocks allowed
#define BLOCK_TYPE_BRIDGE     4         // Reserved: cross-chain bridge

// --- Hash types (C implementation supports CRC32 only) ---
#define HASH_TYPE_CRC32_FULL  0         // CRC32 over full encoded block (default)
#define HASH_TYPE_CRC32_1KB   1         // CRC32 over first 1KB of encoded block
#define HASH_TYPE_ROLL_FULL   2         // ZRollingHash over full block (C#/future)
#define HASH_TYPE_ROLL_1KB    3         // ZRollingHash over first 1KB (C#/future)

// --- Prefix layout (raw, before ZOSCII encoding) ---
// Byte  0:     mode flag (MODE_NORMAL, MODE_X1, or MODE_X2) -- FIRST, always readable unXOR'd
// Byte  1:     block_type (BLOCK_TYPE_NORMAL etc.)
// Byte  2:     block_version (BLOCK_VERSION_DEFAULT=1; reserved for future format changes)
// Byte  3:     hash_type  (HASH_TYPE_CRC32_FULL etc.; C only writes 0)
// Bytes 4-7:   hash of current encoded block
// Bytes 8-11:  hash of previous block's on-disk data (X1/X2; zero for normal or block 1)
//
// On disk (ZOSCII encoded, 2 bytes per raw byte):
// Bytes  0-1:  mode          (never XOR'd in X2 mode)
// Bytes  2-3:  block_type
// Bytes  4-5:  block_version
// Bytes  6-7:  hash_type
// Bytes  8-15: hash of current block
// Bytes 16-23: hash of previous block

// --- Block Header Structure ---
typedef struct {
    char block_id[GUID_LEN];
    char prev_block_id[GUID_LEN];
    char trunk_id[GUID_LEN];
    uint32_t payload_len;
    uint32_t padded_len;
    uint64_t timestamp;
    uint8_t is_branch;                 // 1 = branch, 0 = trunk
} ZTB_BlockHeader;

// --- Block Info (for scanning/verification) ---
typedef struct {
    int index;
    char block_id[GUID_LEN];
    char prev_block_id[GUID_LEN];
    char trunk_id[GUID_LEN];
    char filename[FILENAME_MAX];
    uint8_t is_branch;
    uint32_t padded_len;
    uint64_t timestamp;
} BlockInfo;

// --- Branch Info (for discovery) ---
typedef struct {
    char branch_id[GUID_LEN];
} BranchInfo;

// --- RNG ---
extern uint32_t g_rng_state;
void init_rng(void);
void init_rng_from_rom(const uint8_t *rom_data, long rom_size);
uint32_t get_random(void);

// --- Utility Functions ---
void generate_guid(char *buffer);
uint32_t calculate_checksum(const uint8_t *data, size_t len);
uint32_t calculate_file_checksum(const char *filename);
uint8_t* load_rom(const char *filename);
uint8_t* load_rom_and_seed_rng(const char *filename);
int compare_blocks(const void *a, const void *b);
int scan_chain_blocks(const char *chain_id, BlockInfo *list, int max_blocks);

// --- ZOSCII Encoding/Decoding ---
uint8_t* zoscii_encode_block(const uint8_t *rom_data, const uint8_t *raw_block,
                             size_t raw_len, size_t *encoded_len);
uint8_t* zoscii_decode_block(const uint8_t *rom_data, const uint8_t *encoded_block,
                             size_t encoded_len, size_t *decoded_len);

// --- Rolling ROM Construction ---
int build_rolling_rom(const char *genesis_rom_file,
                     const BlockInfo *chain_history, int chain_count,
                     const BlockInfo *trunk_history, int trunk_count,
                     int target_index, uint8_t *rolling_rom);

// --- Branch Detection and Discovery ---
int detect_branch_status(const char *genesis_rom_file,
                        const BlockInfo *chain_history, int chain_count,
                        const char *chain_id, char *trunk_id_out);

int discover_branches_from_trunk(const char *genesis_rom_file, const char *trunk_id,
                                 BranchInfo *branches, int max_branches);

// --- X2 Mode: XOR a buffer with a file's content, wrapping if file is shorter ---
// Used to XOR the final on-disk block data (prefix + encoded block) with previous block file.
int xor_buffer_with_file(uint8_t *byBuffer_a, size_t intBufferLen_a,
                         const char *strFilename_a);

#endif // ZTB_COMMON_H