// Cyborg ZTB Common Definitions v20260418
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

// --- Block Header Structure ---
typedef struct {
    char block_id[GUID_LEN];
    char prev_block_id[GUID_LEN];
    char trunk_id[GUID_LEN];
    uint32_t payload_len;
    uint32_t padded_len;
    uint32_t checksum;                 // CRC32 checksum of padded payload
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
    uint32_t checksum;
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

#endif // ZTB_COMMON_H