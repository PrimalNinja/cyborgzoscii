// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::fs::File;
use std::io::{Read, Write, BufReader, BufWriter};
use std::process;

fn main() {
    println!("ZOSCII Decoder");
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
        eprintln!("Usage: {} [-16|-32] <romfile> <encodedinput> <outputdatafile>", args[0]);
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

    // Open encoded input file
    let input_file = File::open(&args[2 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening encoded input file: {}", err);
            process::exit(1);
        });
    
    let mut reader = BufReader::new(input_file);

    // Create output file
    let output_file = File::create(&args[3 + offset])
        .unwrap_or_else(|err| {
            eprintln!("Error opening output file: {}", err);
            process::exit(1);
        });

    let mut writer = BufWriter::new(output_file);

    // Decode data
    if bittage == 16 {
        let mut buffer = [0u8; 2];
        loop {
            match reader.read_exact(&mut buffer) {
                Ok(_) => {
                    let address = u16::from_le_bytes(buffer) as usize;
                    if address < rom_size {
                        writer.write_all(&[rom_data[address]]).unwrap();
                    }
                }
                Err(ref e) if e.kind() == std::io::ErrorKind::UnexpectedEof => break,
                Err(e) => {
                    eprintln!("Error reading input: {}", e);
                    process::exit(1);
                }
            }
        }
    } else {
        let mut buffer = [0u8; 4];
        loop {
            match reader.read_exact(&mut buffer) {
                Ok(_) => {
                    let address = u32::from_le_bytes(buffer) as usize;
                    if address < rom_size {
                        writer.write_all(&[rom_data[address]]).unwrap();
                    }
                }
                Err(ref e) if e.kind() == std::io::ErrorKind::UnexpectedEof => break,
                Err(e) => {
                    eprintln!("Error reading input: {}", e);
                    process::exit(1);
                }
            }
        }
    }
}