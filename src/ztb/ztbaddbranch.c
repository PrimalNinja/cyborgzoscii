// ZOSCII Tamperproof Blockchain Utility: ztbaddbranch
// Creates a new branch from a trunk block with ZOSCII encoding.
// ENTIRE BLOCK is ZOSCII encoded (header + payload).
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -t "text"
// Usage: ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -f <file>

#include "ztbcommon.h"

int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Branch Creator\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id> <new_branch_id> -t \"text\"\n", argv[0]);
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id> <new_branch_id> -f <file>\n", argv[0]);
        return 1;
    }
    
    init_rng();
    
    const char *genesis_rom_file = argv[1];
    const char *trunk_id = argv[2];
    const char *new_branch_id = argv[3];
    const char *flag = argv[4];
    const char *data_source = argv[5];
    
    // --- 1. Scan trunk blocks ---
    BlockInfo trunk_history[MAX_BLOCKS_TO_SCAN];
    int trunk_count = scan_chain_blocks(trunk_id, trunk_history, MAX_BLOCKS_TO_SCAN);
    
    if (trunk_count == 0) {
        fprintf(stderr, "Error: Trunk '%s' not found.\n", trunk_id);
        return 1;
    }
    
    printf("Found trunk '%s' with %d blocks\n", trunk_id, trunk_count);
    
    // --- 2. Check if branch already exists ---
    BlockInfo branch_history[MAX_BLOCKS_TO_SCAN];
    int branch_count = scan_chain_blocks(new_branch_id, branch_history, MAX_BLOCKS_TO_SCAN);
    
    if (branch_count > 0) {
        fprintf(stderr, "Error: Branch '%s' already exists with %d blocks.\n", new_branch_id, branch_count);
        fprintf(stderr, "Use ztbaddblock to add to existing branch.\n");
        return 1;
    }
    
    // --- 3. Load payload ---
    uint8_t *raw_payload = NULL;
    size_t payload_len = 0;
    
    if (strcmp(flag, "-t") == 0) {
        payload_len = strlen(data_source);
        raw_payload = malloc(payload_len);
        memcpy(raw_payload, data_source, payload_len);
    } else if (strcmp(flag, "-f") == 0) {
        FILE *f = fopen(data_source, "rb");
        if (!f) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", data_source);
            return 1;
        }
        
        fseek(f, 0, SEEK_END);
        payload_len = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        raw_payload = malloc(payload_len);
        fread(raw_payload, 1, payload_len, f);
        fclose(f);
    } else {
        fprintf(stderr, "Error: Invalid flag '%s'\n", flag);
        return 1;
    }
    
    printf("Payload loaded: %zu bytes\n", payload_len);
    
    // --- 4. Apply padding ---
    size_t padded_len = (payload_len < MIN_PAYLOAD_SIZE) ? MIN_PAYLOAD_SIZE : payload_len;
    uint8_t *padded_payload = calloc(padded_len, 1);
    memcpy(padded_payload, raw_payload, payload_len);
    free(raw_payload);
    
    if (padded_len > payload_len) {
        printf("Padded to %zu bytes\n", padded_len);
    }
    
    // --- 5. Build Rolling ROM (trunk -> genesis) ---
    uint8_t *rolling_rom = malloc(ROM_SIZE);
    if (!build_rolling_rom(genesis_rom_file, NULL, 0,
                          trunk_history, trunk_count, 1, rolling_rom)) {
        fprintf(stderr, "Error: Failed to build rolling ROM\n");
        free(padded_payload);
        free(rolling_rom);
        return 1;
    }
    printf("Rolling ROM built from trunk history\n");
    
    // --- 6. Create Branch Block Header ---
    ZTB_BlockHeader header;
    generate_guid(header.block_id);
    strncpy(header.prev_block_id, trunk_history[trunk_count - 1].block_id, GUID_LEN - 1);
    header.prev_block_id[GUID_LEN - 1] = '\0';
    strncpy(header.trunk_id, trunk_id, GUID_LEN - 1);
    header.trunk_id[GUID_LEN - 1] = '\0';
    header.payload_len = payload_len;
    header.padded_len = padded_len;
    header.checksum = calculate_checksum(padded_payload, padded_len);
    header.timestamp = time(NULL);
    header.is_branch = 1;
    
    printf("Branch Block ID: %s\n", header.block_id);
    printf("Anchored to trunk block: %s\n", header.prev_block_id);
    printf("Checksum: %u (mod 7)\n", header.checksum);
    
    // --- 7. Create complete raw block ---
    size_t raw_block_len = sizeof(ZTB_BlockHeader) + padded_len;
    uint8_t *raw_block = malloc(raw_block_len);
    memcpy(raw_block, &header, sizeof(ZTB_BlockHeader));
    memcpy(raw_block + sizeof(ZTB_BlockHeader), padded_payload, padded_len);
    
    free(padded_payload);
    
    // --- 8. ZOSCII Encode ENTIRE Block ---
    size_t encoded_len;
    uint8_t *encoded_block = zoscii_encode_block(rolling_rom, raw_block,
                                                 raw_block_len, &encoded_len);
    if (!encoded_block) {
        fprintf(stderr, "Error: ZOSCII encoding failed\n");
        free(raw_block);
        free(rolling_rom);
        return 1;
    }
    
    printf("ZOSCII encoded: %zu bytes -> %zu bytes\n", raw_block_len, encoded_len);
    
    free(raw_block);
    free(rolling_rom);
    
    // --- 9. Write Encoded Block File ---
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "%s_%04d_%s.ztb", new_branch_id, 1, header.block_id);
    
    FILE *f_out = fopen(filename, "wb");
    if (!f_out) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        free(encoded_block);
        return 1;
    }
    
    fwrite(encoded_block, 1, encoded_len, f_out);
    fclose(f_out);
    
    printf("\n✓ Branch block created: %s\n", filename);
    printf("✓ New branch '%s' started from trunk '%s'\n", new_branch_id, trunk_id);
    
    free(encoded_block);
    
    return 0;
}