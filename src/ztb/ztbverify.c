// ZOSCII Tamperproof Blockchain Utility: ztbverify
// Verifies the integrity of a ZTB chain (trunk or branch).
// Verifies BACKWARDS from latest block to genesis for each chain.
// Discovers and verifies all branches.
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbverify <genesis_rom> <trunk_id>           - Verify trunk and all branches
// Usage: ztbverify <genesis_rom> <trunk_id> -t        - Verify trunk only
// Usage: ztbverify <genesis_rom> <trunk_id> -b <branch_id> - Verify specific branch only
// Usage: ztbverify <genesis_rom> <trunk_id> -bb       - Verify all branches only (not trunk)

#include "ztbcommon.h"
#include <dirent.h>

// --- Branch Discovery ---
typedef struct {
    char branch_id[GUID_LEN];
} BranchInfo;

// --- Verification Statistics ---
typedef struct {
    int total_blocks;
    int verified_blocks;
    int failed_blocks;
    int branches_found;
} VerifyStats;

// --- Verify Single Block ---
int verify_single_block(const char *genesis_rom_file,
                       const BlockInfo *chain_history, int chain_count,
                       const BlockInfo *trunk_history, int trunk_count,
                       int target_index) {
    
    BlockInfo *target = NULL;
    for (int i = 0; i < chain_count; i++) {
        if (chain_history[i].index == target_index) {
            target = (BlockInfo *)&chain_history[i];
            break;
        }
    }
    
    if (!target) return 0;
    
    // Build rolling ROM
    uint8_t *rolling_rom = malloc(ROM_SIZE);
    if (!rolling_rom || !build_rolling_rom(genesis_rom_file,
                                          chain_history, chain_count,
                                          trunk_history, trunk_count,
                                          target_index, rolling_rom)) {
        if (rolling_rom) free(rolling_rom);
        return 0;
    }
    
    // Load encoded block
    FILE *f = fopen(target->filename, "rb");
    if (!f) {
        free(rolling_rom);
        return 0;
    }
    
    fseek(f, 0, SEEK_END);
    size_t encoded_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *encoded = malloc(encoded_len);
    if (!encoded || fread(encoded, 1, encoded_len, f) != encoded_len) {
        if (encoded) free(encoded);
        fclose(f);
        free(rolling_rom);
        return 0;
    }
    fclose(f);
    
    // Decode ENTIRE block
    size_t decoded_len;
    uint8_t *decoded = zoscii_decode_block(rolling_rom, encoded, encoded_len, &decoded_len);
    
    free(rolling_rom);
    free(encoded);
    
    if (!decoded || decoded_len < sizeof(ZTB_BlockHeader)) {
        if (decoded) free(decoded);
        return 0;
    }
    
    // Extract header and payload
    ZTB_BlockHeader *header = (ZTB_BlockHeader*)decoded;
    uint8_t *payload = decoded + sizeof(ZTB_BlockHeader);
    
    // Verify checksum
    uint32_t calc_checksum = calculate_checksum(payload, header->padded_len);
    int valid = (calc_checksum == header->checksum);
    
    free(decoded);
    return valid;
}

// --- Verify Chain BACKWARDS ---
int verify_chain_backwards(const char *genesis_rom_file, const char *chain_id,
                          const char *trunk_id, VerifyStats *stats) {
    
    BlockInfo history[MAX_BLOCKS_TO_SCAN];
    int block_count = scan_chain_blocks(chain_id, history, MAX_BLOCKS_TO_SCAN);
    
    if (block_count == 0) return 0;
    
    // Determine if branch and load trunk history
    BlockInfo trunk_history[MAX_BLOCKS_TO_SCAN];
    int trunk_count = 0;
    uint8_t is_branch = 0;
    char actual_trunk_id[GUID_LEN];
    strcpy(actual_trunk_id, NULL_GUID);
    
    // Decode first block to check branch status
    FILE *f_first = fopen(history[0].filename, "rb");
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
                        strncpy(actual_trunk_id, first_header->trunk_id, GUID_LEN - 1);
                    }
                    free(first_decoded);
                }
                free(genesis_rom);
            }
        }
        if (first_encoded) free(first_encoded);
        fclose(f_first);
    }
    
    if (is_branch && strcmp(actual_trunk_id, NULL_GUID) != 0) {
        trunk_count = scan_chain_blocks(actual_trunk_id, trunk_history, MAX_BLOCKS_TO_SCAN);
    }
    
    // Verify BACKWARDS from latest to first
    for (int i = block_count - 1; i >= 0; i--) {
        printf("  Block %d (%s)... ", history[i].index, history[i].block_id);
        
        if (verify_single_block(genesis_rom_file, history, block_count,
                               trunk_history, trunk_count, history[i].index)) {
            printf("[PASS]\n");
            stats->verified_blocks++;
        } else {
            printf("[FAIL]\n");
            stats->failed_blocks++;
            return 0;
        }
        
        stats->total_blocks++;
    }
    
    return 1;
}

// --- Discover Branches by Scanning ---
int discover_branches_from_trunk(const char *genesis_rom_file, const char *trunk_id,
                                 BranchInfo *branches, int max_branches) {
    DIR *d = opendir(".");
    if (!d) return 0;
    
    struct dirent *dir;
    int branch_count = 0;
    
    while ((dir = readdir(d)) != NULL) {
        if (!strstr(dir->d_name, ".ztb")) continue;
        
        // Skip if it's a trunk block
        if (strstr(dir->d_name, trunk_id) == dir->d_name) continue;
        
        FILE *f = fopen(dir->d_name, "rb");
        if (!f) continue;
        
        fseek(f, 0, SEEK_END);
        size_t encoded_len = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        uint8_t *encoded = malloc(encoded_len);
        if (encoded && fread(encoded, 1, encoded_len, f) == encoded_len) {
            uint8_t *genesis_rom = load_rom(genesis_rom_file);
            if (genesis_rom) {
                size_t decoded_len;
                uint8_t *decoded = zoscii_decode_block(genesis_rom, encoded,
                                                       encoded_len, &decoded_len);
                if (decoded && decoded_len >= sizeof(ZTB_BlockHeader)) {
                    ZTB_BlockHeader *header = (ZTB_BlockHeader*)decoded;
                    if (header->is_branch && strcmp(header->trunk_id, trunk_id) == 0) {
                        // Extract chain_id from filename
                        char *underscore = strchr(dir->d_name, '_');
                        if (underscore) {
                            size_t id_len = underscore - dir->d_name;
                            if (id_len >= GUID_LEN) id_len = GUID_LEN - 1;
                            
                            // Check if already found
                            int found = 0;
                            for (int i = 0; i < branch_count; i++) {
                                if (strncmp(branches[i].branch_id, dir->d_name, id_len) == 0) {
                                    found = 1;
                                    break;
                                }
                            }
                            
                            if (!found && branch_count < max_branches) {
                                strncpy(branches[branch_count].branch_id, dir->d_name, id_len);
                                branches[branch_count].branch_id[id_len] = '\0';
                                branch_count++;
                            }
                        }
                    }
                    free(decoded);
                }
                free(genesis_rom);
            }
        }
        if (encoded) free(encoded);
        fclose(f);
    }
    
    closedir(d);
    return branch_count;
}

// --- Main ---
int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Chain Verifier\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "Usage: %s <genesis_rom> <trunk_id>                - Verify trunk and all branches\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -t             - Verify trunk only\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -b <branch_id> - Verify specific branch only\n", argv[0]);
        fprintf(stderr, "       %s <genesis_rom> <trunk_id> -bb            - Verify all branches only (not trunk)\n", argv[0]);
        return 1;
    }
    
    const char *genesis_rom_file = argv[1];
    const char *trunk_id = argv[2];
    
    // Parse mode
    int verify_trunk = 1;
    int verify_branches = 1;
    char specific_branch[GUID_LEN] = "";
    
    if (argc >= 4) {
        if (strcmp(argv[3], "-t") == 0) {
            // Trunk only
            verify_trunk = 1;
            verify_branches = 0;
        } else if (strcmp(argv[3], "-bb") == 0) {
            // All branches only
            verify_trunk = 0;
            verify_branches = 1;
        } else if (strcmp(argv[3], "-b") == 0) {
            // Specific branch
            if (argc != 5) {
                fprintf(stderr, "Error: -b requires a branch_id argument\n");
                return 1;
            }
            verify_trunk = 0;
            verify_branches = 0;
            strncpy(specific_branch, argv[4], GUID_LEN - 1);
            specific_branch[GUID_LEN - 1] = '\0';
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[3]);
            return 1;
        }
    }
    
    VerifyStats stats = {0, 0, 0, 0};
    
    // Verify trunk if requested
    if (verify_trunk) {
        printf("=== Verifying Trunk: %s ===\n", trunk_id);
        if (!verify_chain_backwards(genesis_rom_file, trunk_id, NULL, &stats)) {
            printf("\n✗ TRUNK VERIFICATION FAILED\n");
            return 1;
        }
        printf("✓ Trunk verified\n\n");
    }
    
    // Discover branches if needed
    BranchInfo branches[100];
    int branch_count = 0;
    
    if (verify_branches || specific_branch[0] != '\0') {
        branch_count = discover_branches_from_trunk(genesis_rom_file, trunk_id, 
                                                     branches, 100);
        stats.branches_found = branch_count;
    }
    
    // Verify specific branch
    if (specific_branch[0] != '\0') {
        // Check if branch exists
        int found = 0;
        for (int i = 0; i < branch_count; i++) {
            if (strcmp(branches[i].branch_id, specific_branch) == 0) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            fprintf(stderr, "Error: Branch '%s' not found or not linked to trunk '%s'\n", 
                    specific_branch, trunk_id);
            return 1;
        }
        
        printf("=== Verifying Branch: %s ===\n", specific_branch);
        if (!verify_chain_backwards(genesis_rom_file, specific_branch, trunk_id, &stats)) {
            printf("\n✗ BRANCH VERIFICATION FAILED\n");
            return 1;
        }
        printf("✓ Branch verified\n\n");
    }
    // Verify all branches
    else if (verify_branches && branch_count > 0) {
        printf("=== Found %d branch(es) ===\n\n", branch_count);
        
        for (int i = 0; i < branch_count; i++) {
            printf("--- Verifying Branch: %s ---\n", branches[i].branch_id);
            
            if (!verify_chain_backwards(genesis_rom_file, branches[i].branch_id,
                                       trunk_id, &stats)) {
                printf("\n✗ BRANCH VERIFICATION FAILED\n");
                return 1;
            }
            printf("✓ Branch verified\n\n");
        }
    } else if (verify_branches && branch_count == 0) {
        printf("=== No branches found ===\n\n");
    }
    
    // Summary
    printf("=== Verification Summary ===\n");
    printf("Total blocks verified: %d\n", stats.verified_blocks);
    printf("Failed verifications:  %d\n", stats.failed_blocks);
    if (verify_branches || specific_branch[0] != '\0') {
        printf("Branches found:        %d\n", stats.branches_found);
    }
    
    if (stats.failed_blocks == 0) {
        printf("\n✓✓✓ ALL VERIFICATIONS PASSED ✓✓✓\n");
        return 0;
    } else {
        printf("\n✗ VERIFICATION FAILED\n");
        return 1;
    }
}