// ZOSCII Tamperproof Blockchain Utility: ztbaddblock
// Adds a new block to a ZTB chain (trunk or branch) with ZOSCII encoding.
// ENTIRE BLOCK is ZOSCII encoded (header + payload).
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbaddblock <genesis_rom> <chain_id> -t "text string"
// Usage: ztbaddblock <genesis_rom> <chain_id> -f <file_path>

#include "ztbcommon.h"

int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Block Creator\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -t \"text\"\n", argv[0]);
        fprintf(stderr, "Usage: %s <genesis_rom> <chain_id> -f <file>\n", argv[0]);
        return 1;
    }
    
    init_rng();
    
    const char *genesis_rom_file = argv[1];
    const char *chain_id = argv[2];
    const char *flag = argv[3];
    const char *data_source = argv[4];
    
    // --- 1. Load or create payload ---
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
    
    // --- 2. Apply padding ---
    size_t padded_len = (payload_len < MIN_PAYLOAD_SIZE) ? MIN_PAYLOAD_SIZE : payload_len;
    uint8_t *padded_payload = calloc(padded_len, 1);
    memcpy(padded_payload, raw_payload, payload_len);
    free(raw_payload);
    
    if (padded_len > payload_len) {
        printf("Padded to %zu bytes\n", padded_len);
    }
    
    // --- 3. Scan existing blocks ---
    BlockInfo history[MAX_BLOCKS_TO_SCAN];
    int block_count = scan_chain_blocks(chain_id, history, MAX_BLOCKS_TO_SCAN);
    
    int new_index = block_count + 1;
    char prev_block_id[GUID_LEN];
    strcpy(prev_block_id, NULL_GUID);
    
    uint8_t is_branch = 0;
    char trunk_id[GUID_LEN];
    strcpy(trunk_id, NULL_GUID);
    
    if (block_count > 0) {
        // Need to read first block to determine if this chain is a branch
        FILE *f_check = fopen(history[0].filename, "rb");
        if (f_check) {
            size_t first_encoded_size;
            fseek(f_check, 0, SEEK_END);
            first_encoded_size = ftell(f_check);
            fseek(f_check, 0, SEEK_SET);
            
            uint8_t *first_encoded = malloc(first_encoded_size);
            fread(first_encoded, 1, first_encoded_size, f_check);
            fclose(f_check);
            
            // Decode to read header
            uint8_t *genesis_rom = load_rom(genesis_rom_file);
            if (genesis_rom) {
                size_t first_decoded_len;
                uint8_t *first_decoded = zoscii_decode_block(genesis_rom, first_encoded,
                                                             first_encoded_size, &first_decoded_len);
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
            free(first_encoded);
        }
        
        strncpy(prev_block_id, history[block_count - 1].block_id, GUID_LEN - 1);
        printf("Continuing chain from block %d\n", block_count);
    } else {
        printf("Creating first block for chain\n");
    }
    
    // --- 4. Build Rolling ROM ---
    uint8_t *rolling_rom = malloc(ROM_SIZE);
    BlockInfo *trunk_history = NULL;
    int trunk_count = 0;
    
    if (is_branch && strcmp(trunk_id, NULL_GUID) != 0) {
        trunk_history = malloc(MAX_BLOCKS_TO_SCAN * sizeof(BlockInfo));
        trunk_count = scan_chain_blocks(trunk_id, trunk_history, MAX_BLOCKS_TO_SCAN);
    }
    
    if (!build_rolling_rom(genesis_rom_file, history, block_count,
                          trunk_history, trunk_count, new_index, rolling_rom)) {
        fprintf(stderr, "Error: Failed to build rolling ROM\n");
        free(padded_payload);
        free(rolling_rom);
        if (trunk_history) free(trunk_history);
        return 1;
    }
    
    printf("Rolling ROM built from %d previous blocks\n", block_count);
    if (trunk_history) free(trunk_history);
    
    // --- 5. Create Block (Header + Payload) ---
    ZTB_BlockHeader header;
    generate_guid(header.block_id);
    strncpy(header.prev_block_id, prev_block_id, GUID_LEN - 1);
    header.prev_block_id[GUID_LEN - 1] = '\0';
    strncpy(header.trunk_id, trunk_id, GUID_LEN - 1);
    header.trunk_id[GUID_LEN - 1] = '\0';
    header.payload_len = payload_len;
    header.padded_len = padded_len;
    header.checksum = calculate_checksum(padded_payload, padded_len);
    header.timestamp = time(NULL);
    header.is_branch = is_branch;
    
    printf("Block ID: %s\n", header.block_id);
    printf("Checksum: %u (mod 7)\n", header.checksum);
    
    // Create complete raw block: header + payload
    size_t raw_block_len = sizeof(ZTB_BlockHeader) + padded_len;
    uint8_t *raw_block = malloc(raw_block_len);
    memcpy(raw_block, &header, sizeof(ZTB_BlockHeader));
    memcpy(raw_block + sizeof(ZTB_BlockHeader), padded_payload, padded_len);
    
    free(padded_payload);
    
    // --- 6. ZOSCII Encode ENTIRE Block ---
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
    
    // --- 7. Write Encoded Block File ---
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "%s_%04d_%s.ztb", chain_id, new_index, header.block_id);
    
    FILE *f_out = fopen(filename, "wb");
    if (!f_out) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        free(encoded_block);
        return 1;
    }
    
    fwrite(encoded_block, 1, encoded_len, f_out);
    fclose(f_out);
    
    printf("\n✓ Block created: %s\n", filename);
    
    free(encoded_block);
    
    return 0;
}