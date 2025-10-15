// ZOSCII Tamperproof Blockchain Common Functions
// Implementation of shared utilities for ZTB tools
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License

#include "ztbcommon.h"
#include <dirent.h>

// --- Global RNG State ---
uint32_t g_rng_state = 0;

// --- Initialize RNG ---
void init_rng(void) {
    uint32_t seed = (uint32_t)time(NULL);
    seed ^= (uint32_t)clock();
    
#ifdef _WIN32
    seed ^= (uint32_t)GetCurrentProcessId();
    seed ^= (uint32_t)GetTickCount();
#else
    seed ^= (uint32_t)getpid();
#endif
    
    g_rng_state = seed;
    
    // Warm up
    for (int i = 0; i < 10; i++) {
        get_random();
    }
}

// --- Simple xorshift32 RNG ---
uint32_t get_random(void) {
    g_rng_state ^= g_rng_state << 13;
    g_rng_state ^= g_rng_state >> 17;
    g_rng_state ^= g_rng_state << 5;
    return g_rng_state;
}

// --- GUID Generation ---
void generate_guid(char *buffer) {
    snprintf(buffer, GUID_LEN,
             "%08X-%04X-%04X-%04X-%04X%08X",
             get_random(),
             get_random() & 0xFFFF,
             (get_random() & 0x0FFF) | 0x4000,
             (get_random() & 0x3FFF) | 0x8000,
             get_random() & 0xFFFF,
             get_random());
}

// --- Checksum (Mod 7) ---
uint32_t calculate_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum % 7;
}

// --- Load ROM File ---
uint8_t* load_rom(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    uint8_t *rom_data = malloc(ROM_SIZE);
    if (!rom_data) {
        fclose(f);
        return NULL;
    }
    
    if (fread(rom_data, 1, ROM_SIZE, f) != ROM_SIZE) {
        free(rom_data);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    return rom_data;
}

// --- Block Comparison for Sorting ---
int compare_blocks(const void *a, const void *b) {
    return ((BlockInfo *)a)->index - ((BlockInfo *)b)->index;
}

// --- Scan Chain Blocks ---
int scan_chain_blocks(const char *chain_id, BlockInfo *list, int max_blocks) {
    DIR *d = opendir(".");
    if (!d) return 0;
    
    struct dirent *dir;
    int count = 0;
    
    while ((dir = readdir(d)) != NULL && count < max_blocks) {
        if (strstr(dir->d_name, chain_id) == dir->d_name && 
            strstr(dir->d_name, ".ztb")) {
            
            char *underscore1 = strchr(dir->d_name, '_');
            if (!underscore1) continue;
            
            int index = atoi(underscore1 + 1);
            
            char *underscore2 = strchr(underscore1 + 1, '_');
            if (!underscore2) continue;
            
            char block_id[GUID_LEN];
            char *dot = strstr(underscore2 + 1, ".ztb");
            if (!dot) continue;
            
            size_t id_len = dot - (underscore2 + 1);
            if (id_len >= GUID_LEN) id_len = GUID_LEN - 1;
            
            strncpy(block_id, underscore2 + 1, id_len);
            block_id[id_len] = '\0';
            
            strncpy(list[count].block_id, block_id, GUID_LEN - 1);
            list[count].index = index;
            strncpy(list[count].filename, dir->d_name, FILENAME_MAX - 1);
            count++;
        }
    }
    
    closedir(d);
    
    if (count > 0) {
        qsort(list, count, sizeof(BlockInfo), compare_blocks);
    }
    
    return count;
}

// --- ZOSCII Encode Entire Block ---
uint8_t* zoscii_encode_block(const uint8_t *rom_data, const uint8_t *raw_block,
                             size_t raw_len, size_t *encoded_len) {
    
    // Build lookup table
    uint32_t counts[256] = {0};
    for (size_t i = 0; i < ROM_SIZE; i++) {
        counts[rom_data[i]]++;
    }
    
    // Allocate address arrays
    uint32_t **lookup = malloc(256 * sizeof(uint32_t*));
    for (int i = 0; i < 256; i++) {
        lookup[i] = malloc(counts[i] * sizeof(uint32_t));
        counts[i] = 0; // Reset for population
    }
    
    // Populate addresses
    for (size_t i = 0; i < ROM_SIZE; i++) {
        uint8_t byte = rom_data[i];
        lookup[byte][counts[byte]++] = i;
    }
    
    // Encode: 1 byte -> 2 bytes (16-bit pointer)
    *encoded_len = raw_len * 2;
    uint8_t *encoded = malloc(*encoded_len);
    if (!encoded) {
        for (int i = 0; i < 256; i++) free(lookup[i]);
        free(lookup);
        return NULL;
    }
    
    for (size_t i = 0; i < raw_len; i++) {
        uint8_t byte = raw_block[i];
        
        if (counts[byte] == 0) {
            fprintf(stderr, "Error: Byte 0x%02X not in ROM\n", byte);
            free(encoded);
            for (int j = 0; j < 256; j++) free(lookup[j]);
            free(lookup);
            return NULL;
        }
        
        // Random selection
        uint32_t idx = get_random() % counts[byte];
        uint32_t addr = lookup[byte][idx];
        
        // Store as little-endian 16-bit pointer
        uint16_t ptr = (uint16_t)addr;
        encoded[i * 2] = ptr & 0xFF;
        encoded[i * 2 + 1] = (ptr >> 8) & 0xFF;
    }
    
    // Cleanup
    for (int i = 0; i < 256; i++) {
        free(lookup[i]);
    }
    free(lookup);
    
    return encoded;
}

// --- ZOSCII Decode Entire Block ---
uint8_t* zoscii_decode_block(const uint8_t *rom_data, const uint8_t *encoded_block,
                             size_t encoded_len, size_t *decoded_len) {
    
    *decoded_len = encoded_len / 2;
    
    uint8_t *decoded = malloc(*decoded_len);
    if (!decoded) return NULL;
    
    for (size_t i = 0; i < *decoded_len; i++) {
        // Read 16-bit little-endian pointer
        uint16_t ptr = encoded_block[i * 2] | (encoded_block[i * 2 + 1] << 8);
        
        if (ptr >= ROM_SIZE) {
            fprintf(stderr, "Error: Pointer %u out of bounds\n", ptr);
            free(decoded);
            return NULL;
        }
        
        decoded[i] = rom_data[ptr];
    }
    
    return decoded;
}

// --- Build Rolling ROM ---
int build_rolling_rom(const char *genesis_rom_file,
					  const BlockInfo *chain_history, int chain_count,
					  const BlockInfo *trunk_history, int trunk_count,
					  int target_index, uint8_t *rolling_rom) {
	
	uint8_t *genesis_rom = load_rom(genesis_rom_file);
	if (!genesis_rom) {
		fprintf(stderr, "Error: Cannot load genesis ROM\n");
		return 0;
	}
	
	size_t bytes_copied = 0;
	int samples_taken = 0;
	int checkpoint_found = 0;
	
	// Step 1: Sample from chain history (blocks BEFORE target_index)
	if (chain_count > 0 && !checkpoint_found) {
		int available = 0;
		for (int i = 0; i < chain_count; i++) {
			if (chain_history[i].index < target_index) available++;
		}
		
		int to_sample = (available < MAX_HISTORY_BLOCKS) ? available : MAX_HISTORY_BLOCKS;
		int start_idx = available - to_sample;
		int current = 0;
		
		for (int i = 0; i < chain_count; i++) {
			if (chain_history[i].index >= target_index) continue;
			if (checkpoint_found) break;
			
			if (current >= start_idx) {
				// Stop sampling if copying the next sample would exceed the ROM size
				if (bytes_copied + ROM_ENTRY_SIZE > ROM_SIZE) break;

				// Check if this block is a checkpoint by decoding its header
				FILE *f = fopen(chain_history[i].filename, "rb");
				if (f) {
					// Read enough data to decode the header
					size_t header_encoded_size = sizeof(ZTB_BlockHeader) * 2;
					uint8_t *header_encoded = malloc(header_encoded_size);
					
					if (header_encoded && fread(header_encoded, 1, header_encoded_size, f) == header_encoded_size) {
						// Decode header using genesis ROM (checkpoints are encoded with genesis only)
						size_t header_decoded_len;
						uint8_t *header_decoded = zoscii_decode_block(genesis_rom, header_encoded, 
						                                               header_encoded_size, &header_decoded_len);
						
						if (header_decoded && header_decoded_len >= sizeof(ZTB_BlockHeader)) {
							ZTB_BlockHeader *header = (ZTB_BlockHeader*)header_decoded;
							
							// Check if this is a checkpoint block
							if (header->is_branch == 2) {
								checkpoint_found = 1;
								printf("Checkpoint detected at block %d - jumping to genesis ROM\n", chain_history[i].index);
								free(header_decoded);
								free(header_encoded);
								fclose(f);
								break;
							}
							free(header_decoded);
						}
					}
					
					// If not a checkpoint, sample the block normally
					if (!checkpoint_found) {
						fseek(f, 0, SEEK_SET);  // Reset to start of file
						uint8_t encoded_sample[ROM_ENTRY_SIZE];
						size_t read_bytes = fread(encoded_sample, 1, ROM_ENTRY_SIZE, f);
						
						if (read_bytes >= ROM_ENTRY_SIZE) {
							// Copy the 1KB ENCODED ADDRESSES directly
							memcpy(rolling_rom + bytes_copied, encoded_sample, ROM_ENTRY_SIZE);
							bytes_copied += ROM_ENTRY_SIZE;
							samples_taken++;
						}
					}
					
					if (header_encoded) free(header_encoded);
					fclose(f);
				}
			}
			current++;
			if (samples_taken >= MAX_HISTORY_BLOCKS) break;
		}
	}
	
	// Step 2: Sample from trunk history (if this is a branch AND no checkpoint found)
	if (samples_taken < MAX_HISTORY_BLOCKS && trunk_count > 0 && !checkpoint_found) {
		int remaining = MAX_HISTORY_BLOCKS - samples_taken;
		int trunk_samples = (trunk_count < remaining) ? trunk_count : remaining;
		int start_idx = trunk_count - trunk_samples;
		
		for (int i = start_idx; i < trunk_count; i++) {
			if (checkpoint_found) break;
			if (bytes_copied + ROM_ENTRY_SIZE > ROM_SIZE) break;

			// Check if this trunk block is a checkpoint
			FILE *f = fopen(trunk_history[i].filename, "rb");
			if (f) {
				// Check for checkpoint
				size_t header_encoded_size = sizeof(ZTB_BlockHeader) * 2;
				uint8_t *header_encoded = malloc(header_encoded_size);
				
				if (header_encoded && fread(header_encoded, 1, header_encoded_size, f) == header_encoded_size) {
					size_t header_decoded_len;
					uint8_t *header_decoded = zoscii_decode_block(genesis_rom, header_encoded,
					                                               header_encoded_size, &header_decoded_len);
					
					if (header_decoded && header_decoded_len >= sizeof(ZTB_BlockHeader)) {
						ZTB_BlockHeader *header = (ZTB_BlockHeader*)header_decoded;
						
						if (header->is_branch == 2) {
							checkpoint_found = 1;
							printf("Checkpoint detected in trunk at block %d - jumping to genesis ROM\n", trunk_history[i].index);
							free(header_decoded);
							free(header_encoded);
							fclose(f);
							break;
						}
						free(header_decoded);
					}
				}
				
				// Sample normally if not checkpoint
				if (!checkpoint_found) {
					fseek(f, 0, SEEK_SET);
					uint8_t encoded_sample[ROM_ENTRY_SIZE];
					size_t read_bytes = fread(encoded_sample, 1, ROM_ENTRY_SIZE, f);
					
					if (read_bytes >= ROM_ENTRY_SIZE) {
						memcpy(rolling_rom + bytes_copied, encoded_sample, ROM_ENTRY_SIZE);
						bytes_copied += ROM_ENTRY_SIZE;
						samples_taken++;
					}
				}
				
				if (header_encoded) free(header_encoded);
				fclose(f);
			}
			if (samples_taken >= MAX_HISTORY_BLOCKS) break;
		}
	}
	
	// Step 3: Fill remainder with genesis ROM
	if (bytes_copied < ROM_SIZE) {
		memcpy(rolling_rom + bytes_copied, genesis_rom, ROM_SIZE - bytes_copied);
	}
	
	free(genesis_rom);
	return 1;
}
