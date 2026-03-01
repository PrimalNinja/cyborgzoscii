// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::process;
use std::fs;

struct ByteAddresses {
    ptrAddresses: Vec<u16>,
    intCount: u32,
}

struct RomData {
    ptrROMData: Vec<u8>,
    lngROMSize: usize,
    arrROMCounts: [u32; 256],
}

const ZOSCII_ROM_LOAD_MAX: usize = 131072;

fn print_large_number(dblExponent_a: f64) {
    if dblExponent_a < 3.0 {
        print!("~{:.0} permutations", 10_f64.powf(dblExponent_a));
    } else if dblExponent_a < 6.0 {
        print!("~{:.1} thousand permutations", 10_f64.powf(dblExponent_a) / 1000.0);
    } else if dblExponent_a < 9.0 {
        print!("~{:.1} million permutations", 10_f64.powf(dblExponent_a) / 1000000.0);
    } else if dblExponent_a < 12.0 {
        print!("~{:.1} billion permutations", 10_f64.powf(dblExponent_a) / 1000000000.0);
    } else if dblExponent_a < 15.0 {
        print!("~{:.1} trillion permutations", 10_f64.powf(dblExponent_a) / 1000000000000.0);
    } else if dblExponent_a < 82.0 {
        print!("More than all atoms in the observable universe (10^{:.0} permutations)", dblExponent_a);
    } else if dblExponent_a < 1000.0 {
        print!("Incomprehensibly massive (10^{:.0} permutations)", dblExponent_a);
    } else {
        print!("Astronomically secure (10^{:.1}M permutations)", dblExponent_a / 1000000.0);
    }
}

fn load_rom(strFilename_a: &str) -> Result<RomData, String> {
    let mut ptrRom: RomData;
    
    match fs::read(strFilename_a) {
        Ok(arrBuf) => {
            let lngSize = arrBuf.len();
            let lngLoad = if lngSize > ZOSCII_ROM_LOAD_MAX {
                ZOSCII_ROM_LOAD_MAX
            } else {
                lngSize
            };
            
            let mut arrCounts = [0u32; 256];
            
            // Count ROM byte occurrences
            for i in 0..lngLoad {
                arrCounts[arrBuf[i] as usize] += 1;
            }
            
            ptrRom = RomData {
                ptrROMData: arrBuf[..lngLoad].to_vec(),
                lngROMSize: lngLoad,
                arrROMCounts: arrCounts,
            };
            
            Ok(ptrRom)
        }
        Err(e) => {
            Err(format!("Failed to load ROM file: {}", e))
        }
    }
}

fn unload_rom(ptrRom_a: &mut RomData) {
    // In Rust, memory is freed when variables go out of scope,
    // but method kept for symmetry
    ptrRom_a.ptrROMData.clear();
    ptrRom_a.lngROMSize = 0;
    for i in 0..256 {
        ptrRom_a.arrROMCounts[i] = 0;
    }
}

fn analyze_file(ptrRom_a: &RomData, strInputFile_a: &str) -> bool {
    let mut blnSuccess = false;
    let mut arrInputCounts = [0u32; 256];
    let mut intInputLength = 0;
    let mut intCharsUsed = 0;
    let mut dblGeneralStrength = 0.0;
    let mut dblFileStrength = 0.0;
    let dblUtilisation: f64;
    
    match fs::read(strInputFile_a) {
        Ok(inputData) => {
            intInputLength = inputData.len();
            
            // Count input character occurrences
            for &by in &inputData {
                arrInputCounts[by as usize] += 1;
            }
            
            // Count characters utilized
            for i in 0..256 {
                if arrInputCounts[i] > 0 {
                    intCharsUsed += 1;
                }
            }
            
            // Calculate ROM strength metrics
            for i in 0..256 {
                if ptrRom_a.arrROMCounts[i] > 0 {
                    dblGeneralStrength += (ptrRom_a.arrROMCounts[i] as f64).log10();
                }
                if arrInputCounts[i] > 0 && ptrRom_a.arrROMCounts[i] > 0 {
                    dblFileStrength += (arrInputCounts[i] as f64) * (ptrRom_a.arrROMCounts[i] as f64).log10();
                }
            }
            
            dblUtilisation = (intCharsUsed as f64 / 256.0) * 100.0;
            
            println!("ROM Strength Analysis");
            println!("=====================");
            println!();
            
            println!("Input Information:");
            println!("- Text Length: {} characters", intInputLength);
            println!("- Characters Utilized: {} of 256 ({:.1}%)", intCharsUsed, dblUtilisation);
            println!();
            
            print!("General ROM Capacity: ~10^{:.0} (", dblGeneralStrength);
            print_large_number(dblGeneralStrength);
            println!(")");
            
            print!("This File Security: ~10^{:.0} (", dblFileStrength);
            print_large_number(dblFileStrength);
            println!(")");
            println!();
            
            println!("Byte Analysis:");
            println!("Byte  Dec  ROM Count  Input Count  Char");
            println!("----  ---  ---------  -----------  ----");
            
            for i in 0..256 {
                if ptrRom_a.arrROMCounts[i] > 0 || arrInputCounts[i] > 0 {
                    let display = if i >= 32 && i <= 126 {
                        i as u8 as char
                    } else {
                        ' '
                    };
                    println!("0x{:02X}  {:3}  {:9}  {:11}    {}",
                        i, i, ptrRom_a.arrROMCounts[i], arrInputCounts[i], display);
                }
            }
            
            blnSuccess = true;
        }
        Err(_) => {
            blnSuccess = false;
        }
    }
    
    blnSuccess
}

fn main() {
    let mut intResult: i32 = 1;
    let mut ptrRom: RomData;
    let blnAnalyzeOk: bool;
    
    println!("ZOSCII ROM Strength Analyzer");
    println!("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License");
    println!();

    let strArgs: Vec<String> = env::args().collect();
    
    if strArgs.len() == 3 {
        match load_rom(&strArgs[1]) {
            Ok(rom) => {
                ptrRom = rom;
                blnAnalyzeOk = analyze_file(&ptrRom, &strArgs[2]);
                
                if blnAnalyzeOk {
                    intResult = 0;
                } else {
                    eprintln!("Analysis failed");
                }
                
                unload_rom(&mut ptrRom);
            }
            Err(e) => {
                eprintln!("Failed to load ROM: {}", e);
            }
        }
    } else {
        eprintln!("Usage: {} <romfile> <inputdatafile>", strArgs[0]);
    }
    
    process::exit(intResult);
}