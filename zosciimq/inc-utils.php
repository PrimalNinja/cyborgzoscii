<?php 

// Converts the name timestamp part (YYYYMMDDHHNNSSCCCC) to a reordered 
// decimal number (SSNNHHDDMMYYYYCCCC) and encodes it in BASE36.
// This conversion is used to create a non-chronological, distributed name 
// for store directory hashing.
// Returns: Base36 string (0-9 and a-z) or false on failure.
function convertNameToBase36($strName_a)
{
	$varResult = false;
	
    // Extract the timestamp part from YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
    $arrParts = explode('-', $strName_a);
    
    // We only care about the first part (timestamp)
    $strTimestamp = $arrParts[0]; 
    
    if (strlen($strTimestamp) === 18) 
	{
		// Reorder the parts: SS NN HH DD MM YYYY CCCC
		$SS = substr($strTimestamp, 12, 2);
		$NN = substr($strTimestamp, 10, 2);
		$HH = substr($strTimestamp, 8, 2);
		$DD = substr($strTimestamp, 6, 2);
		$MM = substr($strTimestamp, 4, 2);
		$YYYY = substr($strTimestamp, 0, 4);
		$CCCC = substr($strTimestamp, 14, 4); 

		// Concatenate to form the large decimal number
		$strReorderedDecimal = $SS . $NN . $HH . $DD . $MM . $YYYY . $CCCC;

		// Convert to BASE36 (0-9 and a-z)
		$varResult = base_convert($strReorderedDecimal, 10, 36);
	}
    
    return $varResult;
}

function getGUID()
{
    // Generate 16 random bytes
    $binData = random_bytes(16);
    
    // Set version (4) - bits 12-15 of time_hi_and_version field
    $binData[6] = chr(ord($binData[6]) & 0x0f | 0x40);
    
    // Set variant (RFC 4122) - bits 6-7 of clock_seq_hi_and_reserved
    $binData[8] = chr(ord($binData[8]) & 0x3f | 0x80);
    
    // Format as standard GUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    $strGUID = vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($binData), 4));
    
    return $strGUID;
}

function initFolders()
{
	// Ensure the root queue directory exists
	if (!is_dir(QUEUE_ROOT)) 
	{
		if (!mkdir(QUEUE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Fatal Could not create root queue directory: " . QUEUE_ROOT);
			die();
		}
	}

	if (!is_dir(QUEUE_ROOT . TEMP_QUEUE)) 
	{
		if (!mkdir(QUEUE_ROOT . TEMP_QUEUE, FOLDER_PERMISSIONS, true)) 
		{
			logError("Fatal Could not create temp queue directory: " . QUEUE_ROOT . TEMP_QUEUE);
			die();
		}
	}

	// Ensure the root store directory exists
	if (!is_dir(STORE_ROOT)) 
	{
		if (!mkdir(STORE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Fatal Could not create root store directory: " . STORE_ROOT);
			die();
		}
	}

	// Ensure the nonce directory exists
	if (!is_dir(NONCE_ROOT)) 
	{
		if (!mkdir(NONCE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Fatal Could not create nonce directory: " . NONCE_ROOT);
			die();
		} 
	}
}

function getRetentionFromName($strName_a)
{
    // Name format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
    // The retention value (RRRR) is the second segment.
    $arrParts = explode('-', $strName_a);

    // Check if the parts exist (we need at least two segments for retention)
    if (count($arrParts) >= 2) {
        // Cast to integer to get the numeric value from the RRRR segment (index 1)
        return (int)$arrParts[1];
    }
    
    // Default to 0 days retention if the format is invalid
    return 0; 
}

function insertSuffixBeforeExtension($strName_a, $strSuffix_a)
{
	$strResult = "";
	
    // Split the name into components (dirname, basename, extension, name)
    $arrPathParts = pathinfo($strName_a);

    // Ensure we have a name and an extension to work with
    $strBaseName = $arrPathParts['name'] ?? $strName_a;
    $strExtension = $arrPathParts['extension'] ?? null;

    // Recombine the new base name and the extension
    if ($strExtension !== null) 
	{
        $strResult = $strBaseName . $strSuffix_a . '.' . $strExtension;
    }
	else
	{
		// Insert the suffix into the base name
		$strResult = $strBaseName . $strSuffix_a;
	}

    // If there was no extension (e.g., just "blah"), return the base name with the suffix
    return $strResult;
}

function logDebug($str_a)
{
	if (DEBUG === 'TRUE')
	{
		echo($str_a . "\n");
	}
}

function logError($str_a)
{
	if (LOG_OUTPUT === 'TRUE')
	{
		file_put_contents(FILE_ERRORLOG, date('Y-m-d H:i:s') . " - " . $str_a . "\n", FILE_APPEND);
	}
}

function sendJSONResponse($strSystemError_a, $strError_a, $strMessage_a, $arrResult_a)
{
    $arrJSON = ["system" => "ZOSCII MQ", "version" => "1.0", "error" => $strError_a, "message" => $strMessage_a, "result" => $arrResult_a];

	if (strlen($strSystemError_a) > 0)
	{
		logError($strSystemError_a);
	}

    header('Content-Type: application/json');
	$strJSON = json_encode($arrJSON);
	echo($strJSON);
	die();
}

