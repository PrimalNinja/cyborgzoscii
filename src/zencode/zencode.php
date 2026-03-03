<?php
// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

class ByteAddresses 
{
    public $ptrAddresses = array();
    public $intCount = 0;
}

class RomData 
{
    public $ptrROMData = '';
    public $lngROMSize = 0;
    public $arrLookup = array();
}

define('ZOSCII_ROM_LOAD_MAX', 131072);

function buildLookupTable($ptrRom_a) 
{
    $arrCounts = array_fill(0, 256, 0);
    $lngROMSize = 0;
    
    // Initialize lookup array
    $ptrRom_a->arrLookup = array();
    for ($intI = 0; $intI < 256; $intI++) 
    {
        $ptrRom_a->arrLookup[$intI] = new ByteAddresses();
        $ptrRom_a->arrLookup[$intI]->ptrAddresses = array();
        $ptrRom_a->arrLookup[$intI]->intCount = 0;
    }
    
    // ROM addresses are 16-bit, so only use first 64KB
    $lngROMSize = $ptrRom_a->lngROMSize;
    if ($lngROMSize > 65536) 
    {
        $lngROMSize = 65536;
    }
    
    // Count occurrences
    for ($lngI = 0; $lngI < $lngROMSize; $lngI++) 
    {
        $byte = ord($ptrRom_a->ptrROMData[$lngI]);
        $arrCounts[$byte]++;
    }
    
    // Allocate memory for each byte value
    for ($intI = 0; $intI < 256; $intI++) 
    {
        if ($arrCounts[$intI] > 0) 
        {
            $ptrRom_a->arrLookup[$intI]->ptrAddresses = array();
            $ptrRom_a->arrLookup[$intI]->intCount = 0;
        }
    }
    
    // Fill addresses
    for ($lngI = 0; $lngI < $lngROMSize; $lngI++) 
    {
        $by = ord($ptrRom_a->ptrROMData[$lngI]);
        $ptrRom_a->arrLookup[$by]->ptrAddresses[] = $lngI;
        $ptrRom_a->arrLookup[$by]->intCount++;
    }

	// Seed rand based on ROM content
	$intRomHash = 0;
	for ($lngI = 0; $lngI < $ptrRom_a->lngROMSize; $lngI++) 
	{
		$intRomHash = ($intRomHash * 33) + ord($ptrRom_a->ptrROMData[$lngI]);
	}

	$intRomHash ^= (int)(microtime(true) * 1000000);

	srand($intRomHash);
}

function loadRom($strFilename_a) 
{
    $ptrRom = null;
    
    if (file_exists($strFilename_a)) 
    {
        $ptrRom = new RomData();
        
        $ptrFile = fopen($strFilename_a, 'rb');
        if ($ptrFile) 
        {
            $data = fread($ptrFile, ZOSCII_ROM_LOAD_MAX);
            fclose($ptrFile);
            
            if ($data !== false) 
            {
                $ptrRom->ptrROMData = $data;
                $ptrRom->lngROMSize = strlen($data);
                buildLookupTable($ptrRom);
            } 
            else 
            {
                $ptrRom = null;
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
    $ptrRom_a->arrLookup = array();
}

function encodeFile($ptrRom_a, $strInputFile_a, $strOutputFile_a) 
{
    $blnSuccess = false;
    $ptrInput = null;
    $ptrOutput = null;
    
    if (file_exists($strInputFile_a)) 
    {
        $ptrInput = fopen($strInputFile_a, 'rb');
        if ($ptrInput) 
        {
            $ptrOutput = fopen($strOutputFile_a, 'wb');
            if ($ptrOutput) 
            {
                // Stream-encode input
                while (!feof($ptrInput)) 
                {
                    $intCh = fgetc($ptrInput);
                    if ($intCh === false) 
                    {
                        break;
                    }
                    $by = ord($intCh);
                    if ($ptrRom_a->arrLookup[$by]->intCount > 0) 
                    {
                        $intRandomIdx = rand(0, $ptrRom_a->arrLookup[$by]->intCount - 1);
                        $intAddress = $ptrRom_a->arrLookup[$by]->ptrAddresses[$intRandomIdx];
                        fwrite($ptrOutput, pack('v', $intAddress));
                    }
                }
                
                $blnSuccess = true;
                
                fclose($ptrOutput);
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
    $blnEncodeOk = false;
    
    echo "ZOSCII Encoder v20260303\n";
    echo "(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n\n";
    
    // Test harness - hardcoded filenames for testing
    $strRomFile = 'rom.bin';
    $strInputFile = 'input.dat';
    $strOutputFile = 'output.enc';
    
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
                $blnEncodeOk = encodeFile($ptrRom, $strInputFile, $strOutputFile);
                
                if ($blnEncodeOk) 
                {
                    $intResult = 0;
                    echo "Encode successful!\n";
                } 
                else 
                {
                    echo "Encode failed\n";
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