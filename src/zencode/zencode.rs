// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

use std::env;
use std::fs::File;
use std::io::{Read, Write, BufReader, BufWriter};
use std::process;
use rand::{Rng, SeedableRng};
use rand_chacha::ChaCha8Rng;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};
use lazy_static::lazy_static;

struct ByteAddresses 
{
    ptrAddresses: Vec<u16>,
    intCount: u32,
}

const ZOSCII_ROM_LOAD_MAX: usize = 131072;

struct RomData 
{
    ptrROMData: Vec<u8>,
    lngROMSize: usize,
    arrLookup: [ByteAddresses; 256],
}

lazy_static! 
{
    static ref GLOBAL_RNG: Mutex<ChaCha8Rng> = Mutex::new(ChaCha8Rng::seed_from_u64(0));
}

fn build_lookup_table(ptrRom_a: &mut RomData) 
{
    let mut arrCounts: [u32; 256] = [0; 256];
    let mut lngROMSize: usize;
    let mut lngI: usize = 0;
    let mut intI: usize = 0;
    
    // Initialize lookup array
    for intI in 0..256 
    {
        ptrRom_a.arrLookup[intI].ptrAddresses = Vec::new();
        ptrRom_a.arrLookup[intI].intCount = 0;
    }
    
    // ROM addresses are 16-bit, so only use first 64KB
    lngROMSize = ptrRom_a.lngROMSize;
    if lngROMSize > 65536 
    {
        lngROMSize = 65536;
    }
    
    // Count occurrences
    for lngI in 0..lngROMSize 
    {
        arrCounts[ptrRom_a.ptrROMData[lngI] as usize] += 1;
    }
    
    // Allocate memory for each byte value
    for intI in 0..256 
    {
        if arrCounts[intI] > 0 
        {
            ptrRom_a.arrLookup[intI].ptrAddresses = Vec::with_capacity(arrCounts[intI] as usize);
            ptrRom_a.arrLookup[intI].intCount = 0;
        }
    }
    
    // Fill addresses
    for lngI in 0..lngROMSize 
    {
        let by: u8 = ptrRom_a.ptrROMData[lngI];
        ptrRom_a.arrLookup[by as usize].ptrAddresses.push(lngI as u16);
        ptrRom_a.arrLookup[by as usize].intCount += 1;
    }
	
	// Seed rand based on ROM content
	let mut intRomHash: u64 = 0;
	for lngI in 0..ptrRom_a.lngROMSize 
	{
		intRomHash = intRomHash.wrapping_mul(33).wrapping_add(ptrRom_a.ptrROMData[lngI] as u64);
	}

	let time = SystemTime::now()
		.duration_since(UNIX_EPOCH)
		.unwrap()
		.as_micros() as u64;
	intRomHash ^= time;

	*GLOBAL_RNG.lock().unwrap() = ChaCha8Rng::seed_from_u64(intRomHash);
}

fn load_rom(strFilename_a: &str) -> Result<RomData, String> 
{
    let mut ptrRom: RomData;
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
        arrLookup: std::array::from_fn(|_| ByteAddresses 
        { 
            ptrAddresses: Vec::new(), 
            intCount: 0 
        }),
    };
    
    // Pre-build lookup table for reuse across multiple encodes
    build_lookup_table(&mut ptrRom);
    
    return Ok(ptrRom);
}

fn unload_rom(ptrRom_a: &mut RomData) 
{
    // In Rust, memory is freed when variables go out of scope,
    // but method kept for symmetry
    ptrRom_a.ptrROMData.clear();
    ptrRom_a.lngROMSize = 0;
    for intI in 0..256 
    {
        ptrRom_a.arrLookup[intI].ptrAddresses.clear();
        ptrRom_a.arrLookup[intI].intCount = 0;
    }
}

fn encode_file(ptrRom_a: &RomData, strInputFile_a: &str, strOutputFile_a: &str) -> bool 
{
    let mut blnSuccess: bool = false;
    let mut ptrInput: BufReader<File>;
    let mut ptrOutput: BufWriter<File>;
    let mut arrBuf: [u8; 1] = [0u8; 1];
    
    match File::open(strInputFile_a) 
    {
        Ok(f) => 
        {
            ptrInput = BufReader::new(f);
            
            match File::create(strOutputFile_a) 
            {
                Ok(f) => 
                {
                    ptrOutput = BufWriter::new(f);
                    
                    loop 
                    {
                        match ptrInput.read_exact(&mut arrBuf) 
                        {
                            Ok(_) => 
                            {
                                let by: u8 = arrBuf[0];
                                if ptrRom_a.arrLookup[by as usize].intCount > 0 
                                {
                                    let intIdx: usize = GLOBAL_RNG.lock().unwrap().gen_range(
                                        0..ptrRom_a.arrLookup[by as usize].intCount
                                    ) as usize;
                                    let intAddress: u16 = ptrRom_a.arrLookup[by as usize].ptrAddresses[intIdx];
                                    
                                    if ptrOutput.write_all(&intAddress.to_le_bytes()).is_err() 
                                    {
                                        break;
                                    }
                                }
                            }
                            Err(ref e) if e.kind() == std::io::ErrorKind::UnexpectedEof => 
                            {
                                blnSuccess = true;
                                break;
                            }
                            Err(_) => 
                            {
                                break;
                            }
                        }
                    }
                }
                Err(_) => {}
            }
        }
        Err(_) => {}
    }
    
    return blnSuccess;
}

fn main() 
{
    let mut intResult: i32 = 1;
    let mut ptrRom: RomData;
    let strArgs: Vec<String> = env::args().collect();
    let blnEncodeOk: bool;
    
    println!("ZOSCII Encoder v20260303");
    println!("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n");

    if strArgs.len() == 4
    {
        match load_rom(&strArgs[1]) 
        {
            Ok(rom) => 
            {
                ptrRom = rom;
                blnEncodeOk = encode_file(&ptrRom, &strArgs[2], &strArgs[3]);
                
                if blnEncodeOk 
                {
                    intResult = 0;
                } 
                else 
                {
                    eprintln!("Encode failed");
                }
                
                unload_rom(&mut ptrRom);
            }
            Err(e) => 
            {
                eprintln!("Failed to load ROM: {}", e);
            }
        }
    } 
    else 
    {
        eprintln!("Usage: {} <romfile> <inputdatafile> <encodedoutput>", strArgs[0]);
    }
    
    process::exit(intResult);
}