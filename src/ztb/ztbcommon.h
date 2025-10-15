// ZOSCII Tamperproof Blockchain Common Definitions
// Defines shared constants, structures, and utility functions for ZTB tools.
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License

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
#define ROM_SIZE 65536                  // 64KB
#define GUID_LEN 37                     // 36 chars + null terminator
#define MAX_HISTORY_BLOCKS 64           // Max blocks sampled to create rolling ROM
#define BYTES_PER_SAMPLE 1024           // 1KB sample per block
#define MAX_BLOCKS_TO_SCAN 2048
#define MIN_PAYLOAD_SIZE 512            // Minimum payload size (padded)
#define NULL_GUID "00000000-0000-0000-0000-000000000000"
#define ROM_ENTRY_SIZE 1024

// --- Block Header Structure (NOT ZOSCII encoded - always plaintext) ---
// This structure appears at the START of the unencoded block data
typedef struct {
    char block_id[GUID_LEN];           // This block's GUID
    char prev_block_id[GUID_LEN];      // Previous block's GUID
    char trunk_id[GUID_LEN];           // Parent trunk ID (for branches, else NULL_GUID)
    uint32_t payload_len;              // Original payload length
    uint32_t padded_len;               // Padded payload length
    uint32_t checksum;                 // Mod 7 checksum
    uint64_t timestamp;                // Unix timestamp
    uint8_t is_branch;                 // 2 = checkpoint, 1 = branch, 0 = trunk
} ZTB_BlockHeader;

// --- Block Info (for scanning/verification) ---
typedef struct {
    int index;
    char block_id[GUID_LEN];
    char prev_block_id[GUID_LEN];
    char trunk_id[GUID_LEN];
    char filename[FILENAME_MAX];
    uint8_t is_branch;
    uint32_t checksum;
    uint32_t padded_len;
    uint64_t timestamp;
} BlockInfo;

// --- RNG State (global) ---
extern uint32_t g_rng_state;

// --- RNG Functions ---
void init_rng(void);
uint32_t get_random(void);

// --- Utility Functions ---
void generate_guid(char *buffer);
uint32_t calculate_checksum(const uint8_t *data, size_t len);
uint8_t* load_rom(const char *filename);
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

#endif // ZTB_COMMON_H