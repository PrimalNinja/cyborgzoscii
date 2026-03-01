// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::fs::File;
use std::io::{Read, Write, BufReader, BufWriter};
use std::process;

const ZOSCII_ROM_LOAD_MAX: usize = 131072;

struct RomData 
{
    ptrROMData: Vec<u8>,
    lngROMSize: usize,
}

fn load_rom(strFilename_a: &str) -> Result<RomData, String> 
{
    let ptrRom: RomData;
    let mut ptrFile: File;
    let mut arrBuf: Vec<u8> = vec![0u8; ZOSCII_ROM_LOAD_MAX];
    let lngRead: usize;
    
    match File::open(strFilename_a) 
    {
        Ok(f) => 
        {
            ptrFile = f;
        }
        Err(e) => 
        {
            return Err(format!("Failed to open ROM file: {}", e));
        }
    }
    
    match ptrFile.read(&mut arrBuf) 
    {
        Ok(n) => 
        {
            lngRead = n;
        }
        Err(e) => 
        {
            return Err(format!("Failed to read ROM file: {}", e));
        }
    }
    
    arrBuf.truncate(lngRead);
    ptrRom = RomData 
    {
        ptrROMData: arrBuf,
        lngROMSize: lngRead,
    };
    
    return Ok(ptrRom);
}

fn decode_file(ptrRom_a: &RomData, strInputFile_a: &str, strOutputFile_a: &str) -> bool 
{
    let mut blnSuccess: bool = false;
    let mut ptrInput: BufReader<File>;
    let mut ptrOutput: BufWriter<File>;
    let lngInputSize: usize;
    let mut arrBuf: [u8; 2] = [0u8; 2];
    
    // Open input file
    match File::open(strInputFile_a) 
    {
        Ok(f) => 
        {
            ptrInput = BufReader::new(f);
        }
        Err(_) => 
        {
            return false;
        }
    }
    
    // Get input file size
    match ptrInput.get_ref().metadata() 
    {
        Ok(meta) => 
        {
            lngInputSize = meta.len() as usize;
        }
        Err(_) => 
        {
            return false;
        }
    }
    
    let lngSlots: usize = lngInputSize / 2;
    
    if lngSlots > 0 
    {
        // Open output file
        match File::create(strOutputFile_a) 
        {
            Ok(f) => 
            {
                ptrOutput = BufWriter::new(f);
                
                // Decode each slot
                for intI in 0..lngSlots 
                {
                    match ptrInput.read_exact(&mut arrBuf) 
                    {
                        Ok(_) => 
                        {
                            let intAddr: usize = u16::from_le_bytes(arrBuf) as usize;
                            if intAddr < ptrRom_a.lngROMSize 
                            {
                                match ptrOutput.write_all(&[ptrRom_a.ptrROMData[intAddr]]) 
                                {
                                    Ok(_) => {}
                                    Err(_) => 
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        Err(_) => 
                        {
                            break;
                        }
                    }
                    
                    if intI == lngSlots - 1 
                    {
                        blnSuccess = true;
                    }
                }
            }
            Err(_) => {}
        }
    }
    
    return blnSuccess;
}

fn main() 
{
    let intResult: i32;
    let ptrRom: RomData;
    let blnDecodeOk: bool;
    
    println!("ZOSCII Decoder");
    println!("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n");

    let strArgs: Vec<String> = env::args().collect();
    
    if strArgs.len() == 4
    {
        match load_rom(&strArgs[1]) 
        {
            Ok(rom) => 
            {
                ptrRom = rom;
                blnDecodeOk = decode_file(&ptrRom, &strArgs[2], &strArgs[3]);
                
                if blnDecodeOk 
                {
                    intResult = 0;
                } 
                else 
                {
                    eprintln!("Decode failed");
                    intResult = 1;
                }
            }
            Err(e) => 
            {
                eprintln!("Failed to load ROM: {}", e);
                intResult = 1;
            }
        }
    } 
    else 
    {
        eprintln!("Usage: {} <romfile> <encoded> <output>", strArgs[0]);
        intResult = 1;
    }
    
    process::exit(intResult);
}