// ZOSCII Tamperproof Blockchain Utility: ztbcheckpoint
// Creates a checkpoint block (Type 2) for archival boundaries.
// Checkpoints are encoded with GENESIS ROM ONLY (not rolling ROM)
// so future blocks can detect them and jump to genesis.
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbcheckpoint <genesis_rom> <chain_id>

#include "ztbcommon.h"

int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Checkpoint Creator\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id>\n", argv[0]);
        fprintf(stderr, "Example: %s genesis.rom MyTrunk\n", argv[0]);
        fprintf(stderr, "\nCreates a checkpoint block marking an archival boundary.\n");
        fprintf(stderr, "Future blocks will jump from this checkpoint to genesis ROM.\n");
        return 1;
    }

    init_rng();

    const char *genesis_rom_file = argv[1];
    const char *chain_id = argv[2];
    
    // --- 1. Scan existing chain ---
    BlockInfo chain_history[MAX_BLOCKS_TO_SCAN];
    int chain_count = scan_chain_blocks(chain_id, chain_history, MAX_BLOCKS_TO_SCAN);
    
    if (chain_count == 0) {
        fprintf(stderr, "Error: Chain '%s' not found or empty.\n", chain_id);
        return 1;
    }
    
    printf("Found chain '%s' with %d blocks\n", chain_id, chain_count);
    
    // --- 2. Determine if this is a branch ---
    uint8_t is_branch = 0;
    char trunk_id[GUID_LEN];
    strcpy(trunk_id, NULL_GUID);
    
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
    
    // --- 3. Create checkpoint payload (just a marker message) ---
    const char *checkpoint_msg = "CHECKPOINT_ARCHIVAL_BOUNDARY";
    size_t payload_len = strlen(checkpoint_msg) + 1;
    
    size_t padded_len = (payload_len < MIN_PAYLOAD_SIZE) ? MIN_PAYLOAD_SIZE : payload_len;
    uint8_t *padded_payload = calloc(padded_len, 1);
    memcpy(padded_payload, checkpoint_msg, payload_len);
    
    if (padded_len > payload_len) {
        printf("Padded checkpoint payload to %zu bytes\n", padded_len);
    }
    
    // --- 4. Create checkpoint header ---
    int new_index = chain_count + 1;
    
    ZTB_BlockHeader header;
    generate_guid(header.block_id);
    strncpy(header.prev_block_id, chain_history[chain_count - 1].block_id, GUID_LEN - 1);
    header.prev_block_id[GUID_LEN - 1] = '\0';
    strncpy(header.trunk_id, trunk_id, GUID_LEN - 1);
    header.trunk_id[GUID_LEN - 1] = '\0';
    header.payload_len = payload_len;
    header.padded_len = padded_len;
    header.checksum = calculate_checksum(padded_payload, padded_len);
    header.timestamp = time(NULL);
    header.is_branch = 2;  // CHECKPOINT TYPE
    
    printf("\nCheckpoint Block:\n");
    printf("  Block ID:  %s\n", header.block_id);
    printf("  Prev ID:   %s\n", header.prev_block_id);
    printf("  Type:      2 (CHECKPOINT)\n");
    printf("  Index:     %d\n", new_index);
    printf("  Checksum:  %u (mod 7)\n", header.checksum);
    
    // --- 5. Create complete raw block ---
    size_t raw_block_len = sizeof(ZTB_BlockHeader) + padded_len;
    uint8_t *raw_block = malloc(raw_block_len);
    memcpy(raw_block, &header, sizeof(ZTB_BlockHeader));
    memcpy(raw_block + sizeof(ZTB_BlockHeader), padded_payload, padded_len);
    
    free(padded_payload);
    
    // --- 6. Load Genesis ROM (CRITICAL: Encode with genesis ONLY) ---
    uint8_t *genesis_rom = load_rom(genesis_rom_file);
    if (!genesis_rom) {
        fprintf(stderr, "Error: Cannot load genesis ROM '%s'\n", genesis_rom_file);
        free(raw_block);
        return 1;
    }
    
    printf("\nEncoding checkpoint with GENESIS ROM ONLY...\n");
    printf("(This allows future blocks to detect the checkpoint)\n");
    
    // --- 7. ZOSCII Encode ENTIRE Block with Genesis ROM ---
    size_t encoded_len;
    uint8_t *encoded_block = zoscii_encode_block(genesis_rom, raw_block,
                                                 raw_block_len, &encoded_len);
    if (!encoded_block) {
        fprintf(stderr, "Error: ZOSCII encoding failed\n");
        free(raw_block);
        free(genesis_rom);
        return 1;
    }
    
    printf("ZOSCII encoded: %zu bytes -> %zu bytes\n", raw_block_len, encoded_len);
    
    free(raw_block);
    free(genesis_rom);
    
    // --- 8. Write Encoded Block File ---
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "%s_%04d_%s.ztb", chain_id, new_index, header.block_id);
    
    FILE *f_out = fopen(filename, "wb");
    if (!f_out) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        free(encoded_block);
        return 1;
    }
    
    if (fwrite(encoded_block, 1, encoded_len, f_out) != encoded_len) {
        fprintf(stderr, "Error: Failed to write block file\n");
        fclose(f_out);
        free(encoded_block);
        return 1;
    }
    
    fclose(f_out);
    free(encoded_block);
    
    printf("\n✓ Checkpoint created: %s\n", filename);
    printf("\n=== CHECKPOINT INSTRUCTIONS ===\n");
    printf("1. This checkpoint marks block %d as an archival boundary.\n", new_index);
    printf("2. You can now archive blocks 1-%d to cold storage.\n", new_index - 1);
    printf("3. Copy genesis ROM to archive for independent verification.\n");
    printf("4. Keep checkpoint block + genesis ROM on active storage.\n");
    printf("5. Future blocks will automatically jump from checkpoint to genesis.\n");
    printf("================================\n");
    
    return 0;
}