<?php

// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

/**
 * Function to convert string to ZOSCII address sequence
 * @param string $arrBinaryData_a - Binary data containing the ROM/binary data
 * @param string $strInputString_a - Message to convert
 * @param array $arrMemoryBlocks_a - Array of memory blocks with 'start' and 'size' keys
 * @param callable|null $cbConverter_a - Encoding conversion function or null
 * @param int $intUnmappableChar_a - The native character code to be used if it cannot be mapped to ASCII
 * @return array - Array of addresses
 */
function toZOSCII($arrBinaryData_a, $strInputString_a, $arrMemoryBlocks_a, $cbConverter_a, $intUnmappableChar_a)
{
    $arrByteAddresses = [];
    $arrResult = [];
    
    // Initialize Byte Arrays
    for ($intI = 0; $intI < 256; $intI++)
    {
        $arrByteAddresses[$intI] = [];
    }
    
    // Helper function to check if address is in valid memory blocks
    $fnIsValidAddress = function($intAddress) use ($arrMemoryBlocks_a)
    {
        $blnFound = false;
        
        for ($intBlock = 0; $intBlock < count($arrMemoryBlocks_a); $intBlock++)
        {
            $objBlock = $arrMemoryBlocks_a[$intBlock];
            if ($intAddress >= $objBlock['start'] && $intAddress < ($objBlock['start'] + $objBlock['size']))
            {
                $blnFound = true;
                break;
            }
        }
        return $blnFound;
    };
    
    // Parse binary data and populate address arrays
    for ($intAddress = 0; $intAddress < strlen($arrBinaryData_a); $intAddress++)
    {
        // Only process addresses within valid memory blocks
        $blnValidAddress = $fnIsValidAddress($intAddress);
        if ($blnValidAddress)
        {
            $intByte = ord($arrBinaryData_a[$intAddress]);
            $arrByteAddresses[$intByte][] = $intAddress;
        }
    }
    
	// Build result array with random addresses
	for ($intI = 0; $intI < strlen($strInputString_a); $intI++)
	{
		$intIndex = ord($strInputString_a[$intI]);
		if ($cbConverter_a !== null)
		{
			$intIndex = call_user_func($cbConverter_a, $intIndex, $intUnmappableChar_a);
		}

		if ($intIndex >= 0 && count($arrByteAddresses[$intIndex]) > 0)
		{
			// Pick random address from this character's array
			$intRandomPick = mt_rand(0, count($arrByteAddresses[$intIndex]) - 1);
			$intAddress = $arrByteAddresses[$intIndex][$intRandomPick];
			$arrResult[] = $intAddress;
		}
	}
    
    return $arrResult;
}

/**
 * Function to convert PETSCII character codes to ASCII character codes
 * @param int $intPetsciiChar_a - PETSCII character code (0-255)
 * @param int $intUnmappableChar_a - The native character code to be used if it cannot be mapped to ASCII
 * @return int - ASCII character code or intUnmappableChar_a for unmappable characters
 */
function petsciiToAscii($intPetsciiChar_a, $intUnmappableChar_a)
{
    $arrPetsciiToAsciiMap = [
        // 0-31: Control characters
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        // 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

        // 96-255: Everything else
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a
    ];
    
    return $arrPetsciiToAsciiMap[$intPetsciiChar_a];
}

/**
 * Function to convert EBCDIC character codes to ASCII character codes
 * @param int $intEbcdicChar_a - EBCDIC character code (0-255)
 * @param int $intUnmappableChar_a - The native character code to be used if it cannot be mapped to ASCII
 * @return int - ASCII character code or intUnmappableChar_a for unmappable characters
 */
function ebcdicToAscii($intEbcdicChar_a, $intUnmappableChar_a)
{
    $arrEbcdicToAsciiMap = [
        // 0-63: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 64-79: Space and some punctuation
        32, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, 46, 60, 40, 43, 124, // Space .....<(+|
        
        // 80-95: & and punctuation
        38, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, 33, 36, $intUnmappableChar_a, 41, 59, $intUnmappableChar_a, // &.........!$.).;.
        
        // 96-111: - and punctuation
        45, 47, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, 44, 37, 95, 62, 63, // -/........,%_>?
        
        // 112-127: More punctuation
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, 96, 58, 35, 64, 39, 61, 34, // .........`:#@'="
        
        // 128: Control
        $intUnmappableChar_a,
        
        // 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        // 138-144: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 145-153: j-r  
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        // 154-161: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        // 170-192: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        // 202-208: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        // 218-225: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        // 234-239: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a,
        
        // 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        // 250-255: Control/special
        $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a, $intUnmappableChar_a
    ];
    
    return $arrEbcdicToAsciiMap[$intEbcdicChar_a];
}

// Example usage:
$arrMemoryBlocks = [
    ['start' => 0xC000, 'size' => 0x1000],  // ROM at C000-CFFF
    ['start' => 0xE000, 'size' => 0x0800]   // Additional ROM at E000-E7FF
];

// Create 64KB binary data (as string in PHP)
$arrROMData = str_repeat("\x00", 65536);

$arrAddresses = toZOSCII($arrROMData, "Hello, World!", $arrMemoryBlocks, 'ebcdicToAscii', 42);
print_r($arrAddresses);

?>
