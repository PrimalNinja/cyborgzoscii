// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::fs::File;
use std::io::{Read, Write, BufWriter};
use std::process;
use rand::Rng;

struct ByteAddresses {
    addresses: Vec<u32>,
    count: u32,
}

fn main() {
    println!("ZOSCII Encoder");
    println!("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n");

    let args: Vec<String> = env::args().collect();
    
    let mut bittage = 16; // default
    let mut offset = 0;

    if args.len() >= 2 && args[1] == "-32" {
        bittage = 32;
        offset = 1;
    } else if args.len() >= 2 && args[1] == "-16" {
        bittage = 16;
        offset = 1;
    }

    if args.len() != 4 + offset {
        eprintln!("Usage: {} [-16|-32] <romfile> <inputdatafile> <encodedoutput>", args[0]);
        process::exit(1);
    }

    // Read ROM file
    let mut rom_data = std::fs::read(&args[1 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening ROM file: {}", err);
            process::exit(1);
        });

    let max_size: usize = if bittage == 16 { 65536 } else { 4294967296 };
    
    if rom_data.len() > max_size {
        rom_data.truncate(max_size);
    }

    let rom_size = rom_data.len();

    // Build address lookup tables
    let mut lookup: [ByteAddresses; 256] = std::array::from_fn(|_| ByteAddresses {
        addresses: Vec::new(),
        count: 0,
    });

    let mut rom_counts = [0u32; 256];

    // Count occurrences
    for &byte in &rom_data {
        rom_counts[byte as usize] += 1;
    }

    // Allocate address arrays
    for i in 0..256 {
        lookup[i].addresses = Vec::with_capacity(rom_counts[i] as usize);
    }

    // Populate address arrays
    for (i, &byte) in rom_data.iter().enumerate() {
        lookup[byte as usize].addresses.push(i as u32);
        lookup[byte as usize].count += 1;
    }

    // Read input file
    let input_data = std::fs::read(&args[2 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening input file: {}", err);
            process::exit(1);
        });

    // Create output file
    let output_file = File::create(&args[3 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening output file: {}", err);
            process::exit(1);
        });

    let mut writer = BufWriter::new(output_file);
    let mut rng = rand::thread_rng();

    // Encode data
    for &byte in &input_data {
        if lookup[byte as usize].count > 0 {
            let random_idx = rng.gen_range(0..lookup[byte as usize].count) as usize;
            let address = lookup[byte as usize].addresses[random_idx];

            if bittage == 16 {
                let address16 = address as u16;
                writer.write_all(&address16.to_le_bytes()).unwrap();
            } else {
                writer.write_all(&address.to_le_bytes()).unwrap();
            }
        }
    }
}