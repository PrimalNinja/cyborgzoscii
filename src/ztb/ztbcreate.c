// ZOSCII Tamperproof Blockchain Utility: ztbcreate
// Creates a high-entropy 64KB Genesis ROM file for use as the blockchain anchor.
// This ROM serves as the immutable root of trust for the entire ZTB system.
// (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License
// Usage: ztbcreate <output_genesis_rom_file>

#include "ztbcommon.h"

// --- Generate random bytes ---
void generate_random_bytes(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (i % 1024 == 0) {
            g_rng_state ^= (uint32_t)clock();
        }
        buffer[i] = (uint8_t)(get_random() & 0xFF);
    }
}

// --- Analyze ROM entropy ---
void analyze_rom_entropy(const uint8_t *rom_data) {
    uint32_t byte_counts[256] = {0};
    
    for (size_t i = 0; i < ROM_SIZE; i++) {
        byte_counts[rom_data[i]]++;
    }
    
    uint32_t min_count = ROM_SIZE;
    uint32_t max_count = 0;
    uint32_t missing_bytes = 0;
    
    for (int i = 0; i < 256; i++) {
        if (byte_counts[i] == 0) {
            missing_bytes++;
        } else {
            if (byte_counts[i] < min_count) min_count = byte_counts[i];
            if (byte_counts[i] > max_count) max_count = byte_counts[i];
        }
    }
    
    double expected = ROM_SIZE / 256.0;
    
    printf("\n--- ROM Entropy Analysis ---\n");
    printf("Total bytes:           %d\n", ROM_SIZE);
    printf("Expected per value:    %.1f\n", expected);
    printf("Minimum occurrences:   %u\n", min_count);
    printf("Maximum occurrences:   %u\n", max_count);
    printf("Missing byte values:   %u\n", missing_bytes);
    
    if (missing_bytes > 0) {
        printf("\n⚠️  WARNING: %u byte values missing!\n", missing_bytes);
        printf("   This will cause encoding failures.\n");
        printf("   Run ztbcreate again to regenerate.\n");
    } else {
        printf("\n✓ All 256 byte values present.\n");
    }
    
    double deviation = ((double)max_count - min_count) / expected * 100.0;
    printf("Distribution variance: %.1f%%\n", deviation);
    
    if (deviation < 50.0) {
        printf("✓ Excellent entropy.\n");
    } else if (deviation < 100.0) {
        printf("✓ Good entropy.\n");
    } else {
        printf("⚠️  Fair entropy (consider regenerating).\n");
    }
    printf("----------------------------\n");
}

int main(int argc, char *argv[]) {
    printf("ZOSCII Tamperproof Blockchain - Genesis ROM Creator\n");
    printf("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_genesis_rom_file>\n", argv[0]);
        fprintf(stderr, "Example: %s genesis.rom\n", argv[0]);
        fprintf(stderr, "\nCreates a 64KB high-entropy file as the immutable\n");
        fprintf(stderr, "root of trust for your ZTB blockchain.\n");
        return 1;
    }
    
    const char *output_filename = argv[1];
    
    init_rng();
    
    uint8_t *rom_data = malloc(ROM_SIZE);
    if (!rom_data) {
        fprintf(stderr, "Fatal: Cannot allocate %d bytes\n", ROM_SIZE);
        return 1;
    }
    
    printf("Generating %d bytes of random data...\n", ROM_SIZE);
    generate_random_bytes(rom_data, ROM_SIZE);
    printf("✓ Random data generated\n");
    
    analyze_rom_entropy(rom_data);
    
    printf("\nWriting Genesis ROM to: %s\n", output_filename);
    
    FILE *f = fopen(output_filename, "wb");
    if (!f) {
        perror("Fatal: Cannot create output file");
        free(rom_data);
        return 1;
    }
    
    if (fwrite(rom_data, 1, ROM_SIZE, f) != ROM_SIZE) {
        fprintf(stderr, "Fatal: Write failed\n");
        fclose(f);
        free(rom_data);
        return 1;
    }
    
    fclose(f);
    free(rom_data);
    
    printf("✓ Genesis ROM created successfully\n");
    printf("\n--- IMPORTANT SECURITY NOTES ---\n");
    printf("1. Keep this Genesis ROM file secure and backed up.\n");
    printf("2. All blocks depend on this file.\n");
    printf("3. Loss or corruption breaks verification.\n");
    printf("4. Use with ztbaddblock, ztbverify, and ztbfetch.\n");
    printf("5. If any byte values missing, regenerate ROM.\n");
    printf("--------------------------------\n");
    
    return 0;
}