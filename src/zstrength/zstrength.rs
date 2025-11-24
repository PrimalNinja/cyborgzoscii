// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::process;

fn print_large_number(exponent: f64) {
    if exponent < 3.0 {
        print!("~{:.0} permutations", 10_f64.powf(exponent));
    } else if exponent < 6.0 {
        print!("~{:.1} thousand permutations", 10_f64.powf(exponent) / 1000.0);
    } else if exponent < 9.0 {
        print!("~{:.1} million permutations", 10_f64.powf(exponent) / 1000000.0);
    } else if exponent < 12.0 {
        print!("~{:.1} billion permutations", 10_f64.powf(exponent) / 1000000000.0);
    } else if exponent < 15.0 {
        print!("~{:.1} trillion permutations", 10_f64.powf(exponent) / 1000000000000.0);
    } else if exponent < 82.0 {
        print!("More than all atoms in the observable universe (10^{:.0} permutations)", exponent);
    } else if exponent < 1000.0 {
        print!("Incomprehensibly massive (10^{:.0} permutations)", exponent);
    } else {
        print!("Astronomically secure (10^{:.1}M permutations)", exponent / 1000000.0);
    }
}

fn main() {
    println!("ZOSCII ROM Strength Analyzer");
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

    if args.len() != 3 + offset {
        eprintln!("Usage: {} [-16|-32] <romfile> <inputdatafile>", args[0]);
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

    // Count ROM byte occurrences
    let mut rom_counts = [0u32; 256];
    let mut input_counts = [0u32; 256];

    for &byte in &rom_data {
        rom_counts[byte as usize] += 1;
    }

    // Read input file
    let input_data = std::fs::read(&args[2 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening input file: {}", err);
            process::exit(1);
        });

    // Count input character occurrences
    let input_length = input_data.len();
    let mut chars_used = 0;

    for &byte in &input_data {
        input_counts[byte as usize] += 1;
    }

    // Count characters utilized
    for i in 0..256 {
        if input_counts[i] > 0 {
            chars_used += 1;
        }
    }

    // Calculate ROM strength metrics
    let mut general_strength = 0.0;
    let mut file_strength = 0.0;

    for i in 0..256 {
        if rom_counts[i] > 0 {
            general_strength += (rom_counts[i] as f64).log10();
        }
        if input_counts[i] > 0 && rom_counts[i] > 0 {
            file_strength += (input_counts[i] as f64) * (rom_counts[i] as f64).log10();
        }
    }

    let utilisation = (chars_used as f64 / 256.0) * 100.0;

    println!("ROM Strength Analysis ({}-bit)", bittage);
    println!("===============================\n");

    println!("Input Information:");
    println!("- Text Length: {} characters", input_length);
    println!("- Characters Utilized: {} of 256 ({:.1}%)", chars_used, utilisation);
    println!();

    print!("General ROM Capacity: ~10^{:.0} (", general_strength);
    print_large_number(general_strength);
    println!(")");

    print!("This File Security: ~10^{:.0} (", file_strength);
    print_large_number(file_strength);
    println!(")\n");

    println!("Byte Analysis:");
    println!("Byte  Dec  ROM Count  Input Count  Char");
    println!("----  ---  ---------  -----------  ----");

    for i in 0..256 {
        if rom_counts[i] > 0 || input_counts[i] > 0 {
            let display = if i >= 32 && i <= 126 {
                i as u8 as char
            } else {
                ' '
            };
            println!("0x{:02X}  {:3}  {:9}  {:11}    {}",
                i, i, rom_counts[i], input_counts[i], display);
        }
    }
}