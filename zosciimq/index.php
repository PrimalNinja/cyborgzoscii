<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (index.php)
// Deletes messages based on the RRRR (Retention Days) value in the name.
// 
// Execution: php index.php

define('NONCE_ROOT', './nonce/');
define('QUEUE_ROOT', './queues/');
define('STORE_ROOT', './store/');

define('ALLOW_GET', 'TRUE');

define('LOG_OUTPUT', 'FALSE'); // TRUE or FALSE
define('FILE_ERRORLOG', './zosciimq.log'); // or '/var/log/zosciimq.log'
define('FOLDER_PERMISSIONS', 0755);

// --- Helper Functions ---

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

function initFolders()
{
	// Ensure the root queue directory exists
	if (!is_dir(QUEUE_ROOT)) 
	{
		if (!mkdir(QUEUE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			sendJSONResponse("Fatal Could not create root queue directory: " . QUEUE_ROOT, "System Error.", "", []);
		}
	}

	// Ensure the root store directory exists
	if (!is_dir(STORE_ROOT)) 
	{
		if (!mkdir(STORE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			sendJSONResponse("Fatal Could not create root store directory: " . STORE_ROOT, "System Error.", "", []);
		}
	}

	// Ensure the nonce directory exists
	if (!is_dir(NONCE_ROOT)) 
	{
		if (!mkdir(NONCE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			sendJSONResponse("Fatal Could not create nonce directory: " . NONCE_ROOT, "System Error.", "", []);
		} 
	}
}

function logError($str_a) 
{
    if (LOG_OUTPUT === 'TRUE') 
	{
        file_put_contents(FILE_ERRORLOG, date('Y-m-d H:i:s') . " - " . $str_a . "\n", FILE_APPEND);
    }
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

// containing the '-u' suffix before the extension.
function findUnidentifiedFilesRecursive($strPath_a)
{
    $arrResult = [];
    $arrItems = @scandir($strPath_a); // Use @ to suppress warnings for inaccessible directories

    if (!$arrItems === false) 
	{
		foreach ($arrItems as $strItem) 
		{
			if ($strItem === '.' || $strItem === '..') 
			{
				continue;
			}

			$strFullPath = $strPath_a . $strItem;

			if (is_dir($strFullPath)) 
			{
				// Recursively search subdirectories
				$arrResult = array_merge($arrResult, findUnidentifiedFilesRecursive($strFullPath . '/'));
			} 
			else if (is_file($strFullPath)) 
			{
				// Check specifically for '-u' before the file extension (e.g., blah-u.bin)
				if (preg_match('/-u\.[^.]+$/i', $strItem)) 
				{
					// Return the name relative to the STORE_ROOT path
					$strRelativePath = str_replace(STORE_ROOT, '', $strFullPath);
					$arrResult[] = $strItem;
				}
			}
		}
    }

    return $arrResult;
}

function handleFetch($strQueueName_a, $strAfterName_a)
{
	$strQueuePath = QUEUE_ROOT . $strQueueName_a . '/';

	if (is_dir($strQueuePath)) 
	{
		// Get all message files, sorted chronologically by name
		$arrAllFiles = glob($strQueuePath . '*.bin');
		if ($arrAllFiles === false || empty($arrAllFiles)) 
		{
			sendJSONResponse("", "", "Queue is empty.", []);
		} 
		else 
		{
			// Sort files alphabetically (chronologically by name)
			sort($arrAllFiles);
			$strNextMessagePath = null;

			$blnFoundAfter = false;
			// Check if client is requesting from the start (empty 'after')
			if (empty($strAfterName_a)) 
			{
				$blnFoundAfter = true;
			}

			// Find the next message after the 'after' pointer
			// Loop variable $strFullPath is correctly typed (string full path)
			foreach ($arrAllFiles as $strFullPath) 
			{
				$strName = basename($strFullPath);
				if ($blnFoundAfter) 
				{
					// This is the first file after the 'after' pointer
					$strNextMessagePath = $strFullPath;
					break;
				}
				if ($strName === $strAfterName_a) 
				{
					// Found the pointer; the next iteration's file will be the message to return.
					$blnFoundAfter = true;
				}
			}

			if ($strNextMessagePath) 
			{
				$strName = basename($strNextMessagePath);

				header('Content-Type: application/octet-stream');
				header('Content-Disposition: attachment; filename="' . $strName . '"');

				readfile($strNextMessagePath);
				die();
			} 
			else 
			{
				sendJSONResponse("", "", "No new messages found after '" . $strAfterName_a . "' (or queue is empty).", []);
			}
        }
	}
	else
	{
		sendJSONResponse("", "Queue '" . $strQueueName_a . "' does not exist.", "", []);
    }
}

// Handles the 'identify' action for a batch of files. Locates each file (which must 
// have the -u suffix), removes the -u suffix, and renames the file in place.
// $arrNames_a An array of names with the -u suffix.
// return string JSON response detailing the outcome for each file.
function handleIdentify($arrNames_a)
{
	$arrResult = [];

    foreach ($arrNames_a as $strName_a) 
	{
        $strName_a = basename($strName_a); 
        
        $intExtensionLength = strlen(pathinfo($strName_a, PATHINFO_EXTENSION));
        $intSuffixLength = $intExtensionLength + 3; 

        if (substr($strName_a, -$intSuffixLength, 2) === '-u') 
		{
			$strTempName = convertNameToBase36($strName_a);

			if (!$strTempName === false) 
			{
				$strDir1 = substr($strTempName, 0, 1);
				$strDir2 = substr($strTempName, 1, 1);
				$strDir3 = substr($strTempName, 2, 1);
				$strStorePath = STORE_ROOT . $strDir1 . '/' . $strDir2 . '/' . $strDir3 . '/';
				
				$strFullCurrentPath = $strStorePath . $strName_a;

				if (file_exists($strFullCurrentPath)) 
				{
					// Removes the '-u' suffix
					$strNewName = substr_replace($strName_a, '', -$intSuffixLength, 2);
					$strFullNewPath = $strStorePath . $strNewName;

					if (rename($strFullCurrentPath, $strFullNewPath)) 
					{
						$arrResult[] = $strNewName;
					} 
				}
			}
        }
    }

    sendJSONResponse("", "", "Returned messages identified.", $arrResult);
}

function handleNonce($strNonce_a)
{
	$strNonceFile = NONCE_ROOT . $strNonce_a;
	if (file_exists($strNonceFile)) 
	{
		sendJSONResponse("", "", "Nonce already used.", []);
	}
}

function handlePublish($strQueueName_a, $strNonce_a, $intRetentionDays_a, $binMessage_a)
{
    // Format RRRR, e.g., 3 becomes 0003
    $strRetentionDays = sprintf('%04d', $intRetentionDays_a);

    if (empty($strQueueName_a) || empty($binMessage_a)) 
	{
		sendJSONResponse("", "Missing 'q' (queue name) or 'msg' (message content).", "", []);
	}
	else
	{
        $strQueuePath = QUEUE_ROOT . $strQueueName_a . '/';
		
        if (!is_dir($strQueuePath)) 
		{
            if (!mkdir($strQueuePath, FOLDER_PERMISSIONS, true)) 
			{
				sendJSONResponse("Could not create queue directory: " . $strQueuePath, "Could not create queue.", "", []);
            } 
		}
		
		// Generate Name (YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin)
		$strBaseTime = date('YmdHis');
		$strName = '';
		$strFullPath = '';
		$strGetGUID = getGUID();
		
		$strName = $strBaseTime . "0000-" . $strRetentionDays . "-" . $strGetGUID . ".bin";
		$strFullPath = $strQueuePath . $strName;

		// Write the Message
		$intBytesWritten = file_put_contents($strFullPath, $binMessage_a);
		if ($intBytesWritten === false) 
		{
			sendJSONResponse("Failed to create message.", "Failed to create message.", "", []);
		} 
		
		if (strlen($strNonce_a) > 0)
		{
			$strNonceFile = NONCE_ROOT . $strNonce_a;
			if (!touch($strNonceFile))
			{
				unlink($strFullPath);
				sendJSONResponse("Failed to create nonce.", "Failed to create nonce.", "", []);
			}
		}

		sendJSONResponse("", "", "Message published.", []);
	}
}

function handleRetrieve($strName_a)
{
    if (empty($strName_a))
    {
        sendJSONResponse("", "Missing 'name' argument for retrieve action.", "", []);
    }
    else
    {
        $strTempName = convertNameToBase36($strName_a);
        
        if ($strTempName === false) 
        {
			sendJSONResponse("", "Invalid name '" . $strName_a . "'.", "", []);
        }
        else
        {
            // Extract first 3 chars for nested path
            $strDir1 = substr($strTempName, 0, 1);
            $strDir2 = substr($strTempName, 1, 1);
            $strDir3 = substr($strTempName, 2, 1);
            $strStorePath = STORE_ROOT . $strDir1 . '/' . $strDir2 . '/' . $strDir3 . '/';
            
            $strFullPath = $strStorePath . $strName_a;

            if (file_exists($strFullPath)) 
            {
                header('Content-Type: application/octet-stream');
                header('Content-Disposition: attachment; filename="' . $strName_a . '"');
                
                readfile($strFullPath);
                die();
            } 
            else 
            {
				sendJSONResponse("", "Message not found.", "", []);
            }
        }
    }
}

// Scans the entire store for files marked as unidentified (-u) 
// and returns a JSON list of their relative paths.
function handleScan()
{
	$arrResult = findUnidentifiedFilesRecursive(STORE_ROOT);
	sendJSONResponse("", "", "", $arrResult);
}

function handleStore($strNonce_a, $intRetentionDays_a, $binMessage_a)
{
    // Format RRRR, e.g., 3 becomes 0003
    $strRetentionDays = sprintf('%04d', $intRetentionDays_a);

    if (empty($binMessage_a)) 
	{
		sendJSONResponse("", "Message required.", "", []);
    } 
	else 
	{
		$strBaseTime = date('YmdHis');
		$strName = '';
		$strFullPath = '';
		$strGetGUID = getGUID();
		
		$strName = $strBaseTime . "0000-" . $strRetentionDays . "-" . $strGetGUID . ".bin";
		
		$strTempName = convertNameToBase36($strName);
		
		// Extract first 3 chars for nested path
		$strDir1 = substr($strTempName, 0, 1);
		$strDir2 = substr($strTempName, 1, 1);
		$strDir3 = substr($strTempName, 2, 1);
		$strStorePath = STORE_ROOT . $strDir1 . '/' . $strDir2 . '/' . $strDir3 . '/';
		
		$strFullPath = $strStorePath . $strName;
		
		if (!is_dir($strStorePath)) 
		{
			if (!mkdir($strStorePath, FOLDER_PERMISSIONS, true)) 
			{
				sendJSONResponse("Could not create store directory: " . $strStorePath, "Could not create store.", "", []);
			} 
		}

		$intBytesWritten = file_put_contents($strFullPath, $binMessage_a);
		if ($intBytesWritten === false) 
		{
			sendJSONResponse("Failed to create message.", "Failed to create message.", "", []);
		} 
		
		if (strlen($strNonce_a) > 0)
		{
			$strNonceFile = NONCE_ROOT . $strNonce_a;
			if (!touch($strNonceFile))
			{
				unlink($strFullPath);
				sendJSONResponse("Failed to create nonce.", "Failed to create nonce.", "", []);
			}
		}

		sendJSONResponse("", "", "Message stored.", $strName);
    }
}

// entry

initFolders();

// get parameters

$binMessage = '';
$strAction = '';
$strAfterName = '';
$strName = '';
$strNames = '';
$strNonce = '';
$strQueueName = '';
$strRetentionDays = '';

if (ALLOW_GET === 'TRUE')
{
	if (isset($_GET['action'])) 	{ $strAction = $_GET['action']; }
	if (isset($_GET['after'])) 		{ $strAfterName = $_GET['after']; }
	if (isset($_GET['name'])) 		{ $strName = $_GET['name']; }
	if (isset($_GET['names']))		{ $strNames = $_GET['names']; }
	if (isset($_GET['msg'])) 		{ $binMessage = $_GET['msg']; }
	if (isset($_GET['n'])) 			{ $strNonce = $_GET['n']; }
	if (isset($_GET['q'])) 			{ $strQueueName = $_GET['q']; }
	if (isset($_GET['r'])) 			{ $strRetentionDays = $_GET['r']; }
}

if (strlen($strAction) === 0)		{ if (isset($_POST['action'])) 	{ $strAction = $_POST['action']; } }

if (empty($strAction)) 
{
    echo("Welcome to the ZOSCII MQ.");
    die();
}

if (empty($binMessage))				{ if (isset($_POST['msg'])) 	{ $binMessage = $_POST['msg']; } }
if (strlen($strAfterName) === 0)	{ if (isset($_POST['after'])) 	{ $strAfterName = $_POST['after']; } }
if (strlen($strName) === 0)			{ if (isset($_POST['name'])) 	{ $strName = $_POST['name']; } }
if (strlen($strNames) === 0)		{ if (isset($_POST['names'])) 	{ $strNames = $_POST['names']; } }
if (strlen($strNonce) === 0)		{ if (isset($_POST['n'])) 		{ $strNonce = $_POST['n']; } }
if (strlen($strQueueName) === 0)	{ if (isset($_POST['q'])) 		{ $strQueueName = $_POST['q']; } }
if (strlen($strRetentionDays) === 0){ if (isset($_POST['r'])) 		{ $strRetentionDays = $_POST['r']; } }

$arrNames = [];
if (strlen($strNames) > 0)
{
	$arrNames = json_decode($strNames, true);
}

$strAfterName = basename($strAfterName);
$strName = basename($strName);
$strNonce = preg_replace('/[^a-zA-Z0-9_-]/', '', $strNonce);
$strQueueName = preg_replace('/[^a-zA-Z0-9_-]/', '', $strQueueName);
$intRetentionDays = (int)$strRetentionDays;

if (strlen($strNonce) > 0)
{
	handleNonce($strNonce);
}

// router

switch ($strAction) 
{
	case 'fetch':
		handleFetch($strQueueName, $strAfterName);
		break;
		
	case 'identify':
		handleIdentify($arrNames); 
		break;

	case 'publish':
		handlePublish($strQueueName, $strNonce, $intRetentionDays, $binMessage);
		break;
		
	case 'retrieve':
		handleRetrieve($strName);
		break;
		
	case 'scan':
		handleScan();
		break;
		
	case 'store':
		handleStore($strNonce, $intRetentionDays, $binMessage);
		break;
		
	default:
		sendJSONResponse("", "Unknown action '" . $strAction . "'.", "", []);
}

?>