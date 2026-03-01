<?php
// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

class RomData 
{
    public $ptrROMData = '';
    public $lngROMSize = 0;
}

define('ZOSCII_ROM_LOAD_MAX', 131072);

function loadRom($strFilename_a) 
{
    $ptrRom = null;
    
    if (file_exists($strFilename_a)) 
    {
        $ptrRom = new RomData();
        $ptrRom->ptrROMData = file_get_contents($strFilename_a);
        if ($ptrRom->ptrROMData !== false) 
        {
            $ptrRom->lngROMSize = strlen($ptrRom->ptrROMData);
            if ($ptrRom->lngROMSize > ZOSCII_ROM_LOAD_MAX) 
            {
                $ptrRom->ptrROMData = substr($ptrRom->ptrROMData, 0, ZOSCII_ROM_LOAD_MAX);
                $ptrRom->lngROMSize = ZOSCII_ROM_LOAD_MAX;
            }
        } 
        else 
        {
            $ptrRom = null;
        }
    }
    
    return $ptrRom;
}

function unloadRom($ptrRom_a) 
{
    // In PHP, garbage collector handles this, but method kept for symmetry
    $ptrRom_a->ptrROMData = '';
    $ptrRom_a->lngROMSize = 0;
}

function decodeFile($ptrRom_a, $strInputFile_a, $strOutputFile_a) 
{
    $blnSuccess = false;
    $ptrInput = null;
    $ptrOutput = null;
    $arrBuf = '';
    $lngInputSize = 0;
    $lngSlots = 0;
    $intI = 0;
    
    if (file_exists($strInputFile_a)) 
    {
        $ptrInput = fopen($strInputFile_a, 'rb');
        if ($ptrInput) 
        {
            // Get input file size
            fseek($ptrInput, 0, SEEK_END);
            $lngInputSize = ftell($ptrInput);
            fseek($ptrInput, 0, SEEK_SET);
            
            $lngSlots = $lngInputSize / 2;
            
            if ($lngSlots >= 0) 
            {
                $ptrOutput = fopen($strOutputFile_a, 'wb');
                if ($ptrOutput) 
                {
                    // Decode each slot
                    for ($intI = 0; $intI < $lngSlots; $intI++) 
                    {
                        $arrBuf = fread($ptrInput, 2);
                        if (strlen($arrBuf) != 2) 
                        {
                            break;
                        }
                        $arrData = unpack('v', $arrBuf);
                        $intAddr = $arrData[1];
                        if ($intAddr < $ptrRom_a->lngROMSize) 
                        {
                            $byte = $ptrRom_a->ptrROMData[$intAddr];
                            if (fwrite($ptrOutput, $byte) === false) 
                            {
                                break;
                            }
                        }
                    }
                    
                    if ($intI == $lngSlots) 
                    {
                        $blnSuccess = true;
                    }
                    
                    fclose($ptrOutput);
                }
            }
            fclose($ptrInput);
        }
    }
    
    return $blnSuccess;
}

// Test harness
function main() 
{
    $intResult = 1;
    $ptrRom = null;
    $blnDecodeOk = false;
    
    echo "ZOSCII Decoder\n";
    echo "(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n\n";
    
    // Test harness - hardcoded filenames for testing
    $strRomFile = 'rom.bin';
    $strInputFile = 'encoded.enc';
    $strOutputFile = 'decoded.dat';
    
    echo "Test Harness - Using:\n";
    echo "  ROM file: {$strRomFile}\n";
    echo "  Input file: {$strInputFile}\n";
    echo "  Output file: {$strOutputFile}\n\n";
    
    if (file_exists($strRomFile)) 
    {
        $ptrRom = loadRom($strRomFile);
        if ($ptrRom) 
        {
            if (file_exists($strInputFile)) 
            {
                $blnDecodeOk = decodeFile($ptrRom, $strInputFile, $strOutputFile);
                
                if ($blnDecodeOk) 
                {
                    $intResult = 0;
                    echo "Decode successful!\n";
                } 
                else 
                {
                    echo "Decode failed\n";
                }
            } 
            else 
            {
                echo "Input file not found: {$strInputFile}\n";
            }
            
            unloadRom($ptrRom);
        } 
        else 
        {
            echo "Failed to load ROM: {$strRomFile}\n";
        }
    } 
    else 
    {
        echo "ROM file not found: {$strRomFile}\n";
    }
    
    exit($intResult);
}

// Run the test harness
main();
?>