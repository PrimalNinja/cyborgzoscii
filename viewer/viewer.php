<?php

$strFilename = "";

if (isset($_GET['file']))
{
	$strFilename = $_GET['file'];
}

// Read binary file and convert to JSON array
if (file_exists($strFilename)) 
{
    $strBinaryData = file_get_contents($strFilename);
    $arrAddresses = array();
    
    // Convert 4-byte chunks to integers (little-endian)
    for ($intI = 0; $intI < strlen($strBinaryData); $intI += 4) 
    {
        if ($intI + 3 < strlen($strBinaryData)) 
        {
            $intByte1 = ord($strBinaryData[$intI]);
            $intByte2 = ord($strBinaryData[$intI + 1]);
            $intByte3 = ord($strBinaryData[$intI + 2]);
            $intByte4 = ord($strBinaryData[$intI + 3]);
            
            $intAddress = $intByte1 | ($intByte2 << 8) | ($intByte3 << 16) | ($intByte4 << 24);
            $arrAddresses[] = $intAddress;
        }
    }
    
    header('Content-Type: application/json');
    echo json_encode($arrAddresses);
} 
else 
{
    header('Content-Type: application/json');
    echo json_encode(array('error' => 'File not found'));
}
?>