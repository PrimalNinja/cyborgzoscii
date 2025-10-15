// ZOSCII Tamperproof Blockchain Utility: ztbfetch
// Fetches, decodes, and verifies a block from a trunk or branch.
// Decodes ENTIRE BLOCK and extracts header + payload.
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbfetch <genesis_rom> <chain_id> <block_index>

#include "ztbcommon.h"

int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Block Fetcher\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> <block_index>\n", argv[0]);
        return 1;
    }
    
    const char *genesis_rom_file = argv[1];
    const char *chain_id = argv[2];
    int target_index = atoi(argv[3]);
    
    if (target_index < 1) {
        fprintf(stderr, "Error: Block index must be >= 1\n");
        return 1;
    }
    
    // --- 1. Scan chain blocks ---
    BlockInfo chain_history[MAX_BLOCKS_TO_SCAN];
    int chain_count = scan_chain_blocks(chain_id, chain_history, MAX_BLOCKS_TO_SCAN);
    
    if (chain_count == 0) {
        fprintf(stderr, "Error: Chain '%s' not found\n", chain_id);
        return 1;
    }
    
    // --- 2. Find target block ---
    BlockInfo *target_block = NULL;
    for (int i = 0; i < chain_count; i++) {
        if (chain_history[i].index == target_index) {
            target_block = &chain_history[i];
            break;
        }
    }
    
    if (!target_block) {
        fprintf(stderr, "Error: Block %d not found in chain '%s'\n", target_index, chain_id);
        return 1;
    }
    
    printf("Fetching block %d from chain '%s'\n", target_index, chain_id);
    printf("Filename: %s\n", target_block->filename);
    
    // --- 3. Load encoded block file ---
    FILE *f = fopen(target_block->filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open block file '%s'\n", target_block->filename);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t encoded_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *encoded_block = malloc(encoded_len);
    if (!encoded_block || fread(encoded_block, 1, encoded_len, f) != encoded_len) {
        fprintf(stderr, "Error: Cannot read encoded block\n");
        if (encoded_block) free(encoded_block);
        fclose(f);
        return 1;
    }
    fclose(f);
    
    printf("Encoded block: %zu bytes\n", encoded_len);
    
    // --- 4. Check if this is a branch (decode first block to check) ---
    uint8_t is_branch = 0;
    char trunk_id[GUID_LEN];
    strcpy(trunk_id, NULL_GUID);
    
    if (target_index > 1 || chain_count > 1) {
        // Load first block to check branch status
        FILE *f_first = fopen(chain_history[0].filename, "rb");
        if (f_first) {
            fseek(f_first, 0, SEEK_END);
            size_t first_encoded_len = ftell(f_first);
            fseek(f_first, 0, SEEK_SET);
            
            uint8_t *first_encoded = malloc(first_encoded_len);
            if (first_encoded && fread(first_encoded, 1, first_encoded_len, f_first) == first_encoded_len) {
                uint8_t *genesis_rom = load_rom(genesis_rom_file);
                if (genesis_rom) {
                    size_t first_decoded_len;
                    uint8_t *first_decoded = zoscii_decode_block(genesis_rom, first_encoded,
                                                                 first_encoded_len, &first_decoded_len);
                    if (first_decoded && first_decoded_len >= sizeof(ZTB_BlockHeader)) {
                        ZTB_BlockHeader *first_header = (ZTB_BlockHeader*)first_decoded;
                        is_branch = first_header->is_branch;
                        if (is_branch) {
                            strncpy(trunk_id, first_header->trunk_id, GUID_LEN - 1);
                        }
                        free(first_decoded);
                    }
                    free(genesis_rom);
                }
            }
            if (first_encoded) free(first_encoded);
            fclose(f_first);
        }
    }
    
    // --- 5. Load trunk history if this is a branch ---
    BlockInfo trunk_history[MAX_BLOCKS_TO_SCAN];
    int trunk_count = 0;
    
    if (is_branch && strcmp(trunk_id, NULL_GUID) != 0) {
        printf("Loading trunk history: %s\n", trunk_id);
        trunk_count = scan_chain_blocks(trunk_id, trunk_history, MAX_BLOCKS_TO_SCAN);
        if (trunk_count > 0) {
            printf("Found trunk with %d blocks\n", trunk_count);
        }
    }
    
    // --- 6. Build Rolling ROM ---
    uint8_t *rolling_rom = malloc(ROM_SIZE);
    if (!rolling_rom || !build_rolling_rom(genesis_rom_file,
                                          chain_history, chain_count,
                                          trunk_history, trunk_count,
                                          target_index, rolling_rom)) {
        fprintf(stderr, "Error: Failed to build rolling ROM\n");
        if (rolling_rom) free(rolling_rom);
        free(encoded_block);
        return 1;
    }
    
    printf("Rolling ROM reconstructed\n");
    
    // --- 7. Decode ENTIRE Block ---
    size_t decoded_len;
    uint8_t *decoded_block = zoscii_decode_block(rolling_rom, encoded_block,
                                                 encoded_len, &decoded_len);
    if (!decoded_block) {
        fprintf(stderr, "Error: ZOSCII decoding failed\n");
        free(rolling_rom);
        free(encoded_block);
        return 1;
    }
    
    printf("Decoded block: %zu bytes\n", decoded_len);
    
    free(rolling_rom);
    free(encoded_block);
    
    // --- 8. Extract Header and Payload ---
    if (decoded_len < sizeof(ZTB_BlockHeader)) {
        fprintf(stderr, "Error: Decoded block too small for header\n");
        free(decoded_block);
        return 1;
    }
    
    ZTB_BlockHeader *header = (ZTB_BlockHeader*)decoded_block;
    uint8_t *payload = decoded_block + sizeof(ZTB_BlockHeader);
    size_t payload_size = decoded_len - sizeof(ZTB_BlockHeader);
    
    printf("\n--- Block Header ---\n");
    printf("Block ID:      %s\n", header->block_id);
    printf("Prev Block ID: %s\n", header->prev_block_id);
    printf("Trunk ID:      %s\n", header->trunk_id);
    printf("Is Branch:     %s\n", header->is_branch ? "Yes" : "No");
    printf("Payload Len:   %u bytes\n", header->payload_len);
    printf("Padded Len:    %u bytes\n", header->padded_len);
    printf("Timestamp:     %lu\n", (unsigned long)header->timestamp);
    
    // --- 9. Verify Checksum ---
    uint32_t calculated_checksum = calculate_checksum(payload, header->padded_len);
    
    printf("\n--- Verification ---\n");
    printf("Stored checksum:     %u\n", header->checksum);
    printf("Calculated checksum: %u\n", calculated_checksum);
    
    if (calculated_checksum != header->checksum) {
        fprintf(stderr, "\n!!! INTEGRITY FAILURE !!!\n");
        fprintf(stderr, "Checksum mismatch - data is corrupt or tampered\n");
        free(decoded_block);
        return 1;
    }
    
    printf("✓ Integrity verified\n");
    
    // --- 10. Output Payload ---
    printf("\n--- Decoded Payload (%u bytes) ---\n", header->payload_len);
    fwrite(payload, 1, header->payload_len, stdout);
    printf("\n--- End Payload ---\n");
    
    free(decoded_block);
    
    return 0;
}