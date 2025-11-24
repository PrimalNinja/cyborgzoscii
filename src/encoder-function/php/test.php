<?php

require_once('zoscii-encode');

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
