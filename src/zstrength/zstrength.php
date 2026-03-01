<?php
// Cyborg ZOSCII Strength Analyzer v20260301
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
    public $arrROMCounts = array();
    
    public function __construct() 
    {
        $this->arrROMCounts = array_fill(0, 256, 0);
    }
}

define('ZOSCII_ROM_LOAD_MAX', 131072);

function printLargeNumber($dblExponent_a) 
{
    if ($dblExponent_a < 3) 
    {
        printf("~%.0f permutations", pow(10, $dblExponent_a));
    } 
    else if ($dblExponent_a < 6) 
    {
        printf("~%.1f thousand permutations", pow(10, $dblExponent_a) / 1000.0);
    } 
    else if ($dblExponent_a < 9) 
    {
        printf("~%.1f million permutations", pow(10, $dblExponent_a) / 1000000.0);
    } 
    else if ($dblExponent_a < 12) 
    {
        printf("~%.1f billion permutations", pow(10, $dblExponent_a) / 1000000000.0);
    } 
    else if ($dblExponent_a < 15) 
    {
        printf("~%.1f trillion permutations", pow(10, $dblExponent_a) / 1000000000000.0);
    } 
    else if ($dblExponent_a < 82) 
    {
        printf("More than all atoms in the observable universe (10^%.0f permutations)", $dblExponent_a);
    } 
    else if ($dblExponent_a < 1000) 
    {
        printf("Incomprehensibly massive (10^%.0f permutations)", $dblExponent_a);
    } 
    else 
    {
        printf("Astronomically secure (10^%.1fM permutations)", $dblExponent_a / 1000000.0);
    }
}

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
            
            // Count ROM byte occurrences
            for ($lngI = 0; $lngI < $ptrRom->lngROMSize; $lngI++) 
            {
                $byte = ord($ptrRom->ptrROMData[$lngI]);
                $ptrRom->arrROMCounts[$byte]++;
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
    $ptrRom_a->arrROMCounts = array_fill(0, 256, 0);
}

function analyzeFile($ptrRom_a, $strInputFile_a) 
{
    $blnSuccess = false;
    $arrInputCounts = array_fill(0, 256, 0);
    $intInputLength = 0;
    $intCharsUsed = 0;
    $dblGeneralStrength = 0.0;
    $dblFileStrength = 0.0;
    $dblUtilisation = 0.0;
    
    if (file_exists($strInputFile_a)) 
    {
        $inputData = file_get_contents($strInputFile_a);
        if ($inputData !== false) 
        {
            $intInputLength = strlen($inputData);
            
            // Count input character occurrences
            for ($lngI = 0; $lngI < $intInputLength; $lngI++) 
            {
                $byte = ord($inputData[$lngI]);
                $arrInputCounts[$byte]++;
            }
            
            // Count characters utilized
            for ($intI = 0; $intI < 256; $intI++) 
            {
                if ($arrInputCounts[$intI] > 0) 
                {
                    $intCharsUsed++;
                }
            }
            
            // Calculate ROM strength metrics
            for ($intI = 0; $intI < 256; $intI++) 
            {
                if ($ptrRom_a->arrROMCounts[$intI] > 0) 
                {
                    $dblGeneralStrength += log10($ptrRom_a->arrROMCounts[$intI]);
                }
                if ($arrInputCounts[$intI] > 0 && $ptrRom_a->arrROMCounts[$intI] > 0) 
                {
                    $dblFileStrength += $arrInputCounts[$intI] * log10($ptrRom_a->arrROMCounts[$intI]);
                }
            }
            
            $dblUtilisation = ($intCharsUsed / 256.0) * 100.0;
            
            echo "ROM Strength Analysis\n";
            echo "=====================\n\n";
            
            echo "Input Information:\n";
            echo "- Text Length: $intInputLength characters\n";
            echo "- Characters Utilized: $intCharsUsed of 256 (" . number_format($dblUtilisation, 1) . "%)\n";
            echo "\n";
            
            echo "General ROM Capacity: ~10^" . number_format($dblGeneralStrength, 0) . " (";
            printLargeNumber($dblGeneralStrength);
            echo ")\n";
            
            echo "This File Security: ~10^" . number_format($dblFileStrength, 0) . " (";
            printLargeNumber($dblFileStrength);
            echo ")\n\n";
            
            echo "Byte Analysis:\n";
            echo "Byte  Dec  ROM Count  Input Count  Char\n";
            echo "----  ---  ---------  -----------  ----\n";
            
            for ($intI = 0; $intI < 256; $intI++) 
            {
                if ($ptrRom_a->arrROMCounts[$intI] > 0 || $arrInputCounts[$intI] > 0) 
                {
                    $chDisplay = ($intI >= 32 && $intI <= 126) ? chr($intI) : ' ';
                    printf("0x%02X  %3d  %9u  %11u    %c\n", 
                           $intI, $intI, $ptrRom_a->arrROMCounts[$intI], $arrInputCounts[$intI], $chDisplay);
                }
            }
            
            $blnSuccess = true;
        }
    }
    
    return $blnSuccess;
}

// Test harness
function main() 
{
    $intResult = 1;
    $ptrRom = null;
    $blnAnalyzeOk = false;
    
    echo "ZOSCII ROM Strength Analyzer\n";
    echo "(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n\n";
    
    // Test harness - hardcoded filenames for testing
    $strRomFile = 'rom.bin';
    $strInputFile = 'input.dat';
    
    echo "Test Harness - Using:\n";
    echo "  ROM file: {$strRomFile}\n";
    echo "  Input file: {$strInputFile}\n\n";
    
    if (file_exists($strRomFile)) 
    {
        $ptrRom = loadRom($strRomFile);
        if ($ptrRom) 
        {
            if (file_exists($strInputFile)) 
            {
                $blnAnalyzeOk = analyzeFile($ptrRom, $strInputFile);
                
                if ($blnAnalyzeOk) 
                {
                    $intResult = 0;
                    echo "\nAnalysis complete!\n";
                } 
                else 
                {
                    echo "Analysis failed\n";
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