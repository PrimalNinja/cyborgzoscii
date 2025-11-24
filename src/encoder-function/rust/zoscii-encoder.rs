// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

//! ZOSCII encoding library for Rust

use std::time::Instant;
use rand::Rng;

/// Represents a block of memory in the ROM
#[derive(Debug, Clone, Copy)]
pub struct MemoryBlock {
    pub start: usize,
    pub size: usize,
}

/// Contains the encoded addresses and statistics
#[derive(Debug)]
pub struct ZosciiResult {
    pub addresses: Vec<usize>,
    pub input_counts: [usize; 256],
    pub rom_counts: [usize; 256],
}

/// Function type for character conversion
pub type ConverterFunc = fn(char_code: usize, unmappable_char: usize) -> usize;

/// Converts a string to ZOSCII address sequence
pub fn to_zoscii(
    binary_data: &[u8],
    input_string: &str,
    memory_blocks: &[MemoryBlock],
    converter: Option<ConverterFunc>,
    unmappable_char: usize,
) -> ZosciiResult {
    let start_time = Instant::now();

    let mut byte_counts = [0usize; 256];
    let mut byte_addresses: Vec<Vec<usize>> = (0..256).map(|_| Vec::new()).collect();
    let mut input_counts = [0usize; 256];
    let mut result_count = 0;
    let mut debug_missing = 0;

    // Pass 1: Count occurrences by iterating through blocks
    for block in memory_blocks {
        let end = (block.start + block.size).min(binary_data.len());
        for address in block.start..end {
            byte_counts[binary_data[address] as usize] += 1;
        }
    }

    // Pass 2: Pre-allocate exact-sized vectors
    for i in 0..256 {
        if byte_counts[i] > 0 {
            byte_addresses[i] = Vec::with_capacity(byte_counts[i]);
        }
    }

    // Pass 3: Populate arrays by iterating through blocks
    for block in memory_blocks {
        let end = (block.start + block.size).min(binary_data.len());
        for address in block.start..end {
            let byte_val = binary_data[address] as usize;
            byte_addresses[byte_val].push(address);
        }
    }

    // Count valid characters for result array size
    for ch in input_string.chars() {
        let mut index = ch as usize;
        if let Some(conv) = converter {
            index = conv(index, unmappable_char);
        }
        if index < 256 && !byte_addresses[index].is_empty() {
            result_count += 1;
        } else {
            debug_missing += 1;
            if debug_missing <= 10 {
                println!("Missing character: '{}' (code {} -> {})", ch, ch as usize, index);
            }
        }
    }

    println!("Characters found in ROM: {}", result_count);
    println!("Characters missing from ROM: {}", debug_missing);

    // Build result array with random addresses
    let mut addresses = Vec::with_capacity(result_count);
    let mut rng = rand::thread_rng();

    for ch in input_string.chars() {
        let mut index = ch as usize;
        if let Some(conv) = converter {
            index = conv(index, unmappable_char);
        }

        if index < 256 && !byte_addresses[index].is_empty() {
            input_counts[index] += 1;
            let random_pick = rng.gen_range(0..byte_addresses[index].len());
            addresses.push(byte_addresses[index][random_pick]);
        }
    }

    let elapsed = start_time.elapsed();

    println!("ZOSCII Performance:");
    println!("- Input length: {} chars", input_string.len());
    println!("- Memory blocks: {}", memory_blocks.len());
    println!("- Execution time: {:.2}ms", elapsed.as_secs_f64() * 1000.0);
    println!("- Output addresses: {}", addresses.len());

    ZosciiResult {
        addresses,
        input_counts,
        rom_counts: byte_counts,
    }
}

/// Converts PETSCII character codes to ASCII
pub fn petscii_to_ascii(petscii_char: usize, unmappable_char: usize) -> usize {
    const PETSCII_MAP: [i32; 256] = [
        // 0-31: Control characters
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        // 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        
        // 96-255: Everything else mapped to unmappable
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    ];

    if petscii_char > 255 {
        return unmappable_char;
    }

    let result = PETSCII_MAP[petscii_char];
    if result == -1 {
        unmappable_char
    } else {
        result as usize
    }
}

/// Converts EBCDIC character codes to ASCII
pub fn ebcdic_to_ascii(ebcdic_char: usize, unmappable_char: usize) -> usize {
    const EBCDIC_MAP: [i32; 256] = [
        // 0-63: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 64-79: Space and some punctuation
        32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 46, 60, 40, 43, 124,
        
        // 80-95: & and punctuation
        38, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, 36, -1, 41, 59, -1,
        
        // 96-111: - and punctuation
        45, 47, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 37, 95, 62, 63,
        
        // 112-127: More punctuation
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 96, 58, 35, 64, 39, 61, 34,
        
        // 128: Control
        -1,
        
        // 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        // 138-144: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        // 145-153: j-r
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        // 154-161: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        // 170-192: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1,
        
        // 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        // 202-208: Control/special
        -1, -1, -1, -1, -1, -1, -1,
        
        // 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        // 218-225: Control/special
        -1, -1, -1, -1, -1, -1, -1, -1,
        
        // 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        // 234-239: Control/special
        -1, -1, -1, -1, -1, -1,
        
        // 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        // 250-255: Control/special
        -1, -1, -1, -1, -1, -1,
    ];

    if ebcdic_char > 255 {
        return unmappable_char;
    }

    let result = EBCDIC_MAP[ebcdic_char];
    if result == -1 {
        unmappable_char
    } else {
        result as usize
    }
}