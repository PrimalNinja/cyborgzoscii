<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Server-to-Q Replikate Cron Job (replikate.php)
// 
// Pulls messages from a URL and publishes them to a local Target Queue.
// State is managed using a unique file per target queue, making it concurrency-safe.
// 
// Execution Example:
// http://your.server/replikate.php?url=http://other.server/index.php&sq=sourcequeue&tq=targetqueue
// 
// NOTE: This script requires the PHP cURL extension to be installed and enabled.

// --- Global Configuration ---
define('QUEUE_ROOT', './queues/');
define('STATE_ROOT', './states/');
define('STORE_ROOT', './store/');

define('CLI_ONLY', 'FALSE'); 

DEFINE('LOG_OUTPUT', 'TRUE');	// TRUE or FALSE
DEFINE('FILE_ERRORLOG', './replikate.log'); // or '/var/log/replikate.log'
define('FOLDER_PERMISSIONS', 0755);

define('STATE_FILE_TEMPLATE', './states/replikate_state_%IDENTIFIER%.txt');
define('STATE_KEY_NAME', 'last_processed_id');

// --- Helper Functions ---
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

function initFolders()
{
	// Ensure the root queue directory exists
	if (!is_dir(QUEUE_ROOT)) 
	{
		if (!mkdir(QUEUE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Could not create root queue directory " . QUEUE_ROOT);
			echo("Could not create root queue directory " . QUEUE_ROOT);
			die();
		}
	}

	// Ensure the root state directory exists
	if (!is_dir(STATE_ROOT)) 
	{
		if (!mkdir(STATE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Could not create root state directory " . STATE_ROOT);
			echo("Could not create root state directory " . STATE_ROOT);
			die();
		}
	}

	// Ensure the root store directory exists
	if (!is_dir(STORE_ROOT)) 
	{
		if (!mkdir(STORE_ROOT, FOLDER_PERMISSIONS, true)) 
		{
			logError("Could not create root store directory " . STORE_ROOT);
			echo("Could not create root store directory " . STORE_ROOT);
			die();
		}
	}
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

function fetchNextMessage($strSourceURL_a, $strSourceQueue_a, $strAfterName_a) 
{
    $arrResult = array(null, null);	// name, content

	$strFetchURL = $strSourceURL_a . '?action=fetch&q=' . urlencode($strSourceQueue_a) . '&after=' . urlencode($strAfterName_a);
	//echo("FETCHING: " . $strFetchURL . "\n");

	$objCurl = curl_init($strFetchURL);
	curl_setopt($objCurl, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($objCurl, CURLOPT_HEADER, true);
	curl_setopt($objCurl, CURLOPT_NOBODY, false);
	curl_setopt($objCurl, CURLOPT_TIMEOUT, 10);
	$strResponse = curl_exec($objCurl);

	if (curl_errno($objCurl)) 
	{
		logError("cURL ERROR: Failed to connect or execute request: " . curl_error($objCurl));
	} 
	else 
	{
		$intHeaderSize = curl_getinfo($objCurl, CURLINFO_HEADER_SIZE);
		$strHeaders = substr($strResponse, 0, $intHeaderSize);
		$binContent = substr($strResponse, $intHeaderSize);
		$intHttpStatus = curl_getinfo($objCurl, CURLINFO_HTTP_CODE);

		if ($intHttpStatus !== 200) 
		{
			logError("Received HTTP Status " . $intHttpStatus . ". Server response: " . $binContent);
		} 
		else 
		{
			preg_match('/Content-Disposition: attachment; filename="([^"]+)"/i', $strHeaders, $matches);
			$strName = isset($matches[1]) ? $matches[1] : null;

			if ($strName !== null) 
			{
				$strName = basename($strName); 
			}

			if ($strName && $binContent !== false) 
			{
				$arrResult = array($strName, $binContent);
			} 
			else 
			{
                $arrJSON = json_decode($binContent, true);
                if (json_last_error() === JSON_ERROR_NONE && is_array($arrJSON))
                {
					if (isset($arrJSON['system']) && ($arrJSON['system'] === "ZOSCII MQ"))
					{
						if (isset($arrJSON['error']) && strlen($arrJSON['error']) > 0)
						{
							logError("Source MQ reported JSON error: " . $arrJSON['error'] . " - Message: " . $arrJSON['message']);
						}
					}
					else
					{
						logError("Invalid JSON. Content: " . substr($binContent, 0, 100)); 
					}
                }
                else
                {
                    logError("Invalid response structure (missing name or content, and not valid JSON). Content: " . substr($binContent, 0, 100)); 
                }
			}
		}
	}
	curl_close($objCurl);

    return $arrResult;
}

function saveToQueue($strQueueName_a, $strName_a, $strMessage_a) 
{
    $blnResult = true;

	$strQueuePath = QUEUE_ROOT . $strQueueName_a . '/';
	if (!is_dir($strQueuePath)) 
	{
		if (!mkdir($strQueuePath, FOLDER_PERMISSIONS, true)) 
		{
			$blnResult = false;
			logError("Could not create queue directory: " . $strQueuePath);
		} 
	}
		
	if ($blnResult)
	{
		$strFullPath = $strQueuePath . $strName_a;
		$intBytesWritten = file_put_contents($strFullPath, $strMessage_a);
		if ($intBytesWritten === false) 
		{
			$blnResult = false;
			logError("Failed to write file to queue: " . $strFullPath);
		}
	}

    return $blnResult;
}

function saveToStore($strName_a, $strMessage_a) 
{
    $blnResult = true;
	$strName = $strName_a;
    
    $strTempName = convertNameToBase36($strName);
    
    if ($strTempName === false) 
	{
        logError("Name conversion failed for: " . $strName);
        return false;
    }
    
    $strDir1 = substr($strTempName, 0, 1);
    $strDir2 = substr($strTempName, 1, 1);
    $strDir3 = substr($strTempName, 2, 1);
    $strStorePath = STORE_ROOT . $strDir1 . '/' . $strDir2 . '/' . $strDir3 . '/';

	if (!is_dir($strStorePath)) 
	{
		if (!mkdir($strStorePath, FOLDER_PERMISSIONS, true)) 
		{
			$blnResult = false;
			logError("Could not create nested store directory: " . $strStorePath);
		} 
	}
		
	if ($blnResult)
	{
		$strName = insertSuffixBeforeExtension($strName, "-u");
		$strFullPath = $strStorePath . $strName;
		$intBytesWritten = file_put_contents($strFullPath, $strMessage_a);
		if ($intBytesWritten === false) 
		{
			$blnResult = false;
			logError("Failed to write file to store: " . $strFullPath);
		}
	}

    return $blnResult;
}

function handleReplikate($strSourceURL_a, $strSourceQueue_a, $strTargetQueue_a) 
{
    echo("--- Starting ZOSCII MQ Replikation ---\n");

	$strSafeIdentifier = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strSourceQueue_a);
	$strStateFilePath = str_replace('%IDENTIFIER%', $strSafeIdentifier, STATE_FILE_TEMPLATE);
	$strStateKey = STATE_KEY_NAME;

	$strLastName = '';
	if (file_exists($strStateFilePath)) 
	{
		$strLastName = file_get_contents($strStateFilePath);
	}

	//echo("Processing URL: " . $strSourceURL_a . " -> Target Q: " . $strTargetQueue_a . "\n");
	if (empty($strLastName)) 
	{
		echo("Last Processed Pointer: START\n");
	} 
	else 
	{
		echo("Last Processed Pointer: " . $strLastName . "\n");
	}
	$intTotalReplikated = 0;

	while (true) 
	{
		list($strName, $binContent) = fetchNextMessage($strSourceURL_a, $strSourceQueue_a, $strLastName);

		if ($strName === null) 
		{
			echo("Source URL is caught up or returned no data. Halting this run.");
			break;
		}

		if (strlen($strTargetQueue_a) > 0)
		{
			// save to queue
			$blnSuccess = saveToQueue($strTargetQueue_a, $strName, $binContent);
			if ($blnSuccess) 
			{
				$strLastName = $strName;
				$intTotalReplikated++;
				echo("REPLIKATED: New pointer set to " . $strLastName . "\n");
			} 
			else 
			{
				logError("Failed to replicate message to " . $strTargetQueue_a . ". Halting.");
				echo("Failed to replicate message to " . $strTargetQueue_a . ". Halting.");
				break;
			}
		}
		else
		{
			// save to store
			$blnSuccess = saveToStore($strName, $binContent);
			if ($blnSuccess) 
			{
				$strLastName = $strName;
				$intTotalReplikated++;
			} 
			else 
			{
				logError("Failed to replicate message to store. Halting.");
				echo("Failed to replicate message to store. Halting.");
				break;
			}
		}
	}

	file_put_contents($strStateFilePath, $strLastName);
	echo("--- Replikation finished. Total messages replikated: " . $intTotalReplikated . " ---\n");
}

// entry

if (CLI_ONLY === 'TRUE')
{
	if (php_sapi_name() !== 'cli') 
	{
        logError("This script can only be run from the command line.");
		die("This script can only be run from the command line.");
	}
}

// Check if cURL is installed before attempting to run tests
if (!extension_loaded('curl')) 
{
    echo("\nThe cURL PHP extension is not installed or enabled. Please enable it in your php.ini.\n");
    echo("--- Test Halted ---\n");
    exit(1);
}

initFolders();

// get parameters

$strSourceURL = '';
$strSourceQueue = '';
$strTargetQueue = '';

if (isset($_GET['sq'])) 			{ $strSourceQueue = $_GET['sq']; } 
if (isset($_GET['tq'])) 			{ $strTargetQueue = $_GET['tq']; } 
if (isset($_GET['url'])) 			{ $strSourceURL = $_GET['url']; } 

if (strlen($strSourceQueue) === 0)	{ if (isset($_POST['sq'])) { $strSourceQueue = $_POST['sq']; } }
if (strlen($strTargetQueue) === 0)	{ if (isset($_POST['tq'])) 	{ $strTargetQueue = $_POST['tq']; } }
if (strlen($strSourceURL) === 0)	{ if (isset($_POST['url'])) 	{ $strSourceURL = $_POST['url']; } }

if (!$strSourceURL || !$strSourceQueue) 
{
	logError("Missing required arguments.");
	echo("Missing required arguments.");
	die();
} 

// --- Execution ---
handleReplikate($strSourceURL, $strSourceQueue, $strTargetQueue);
