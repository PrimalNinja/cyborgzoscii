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

define('CLI_ONLY', 'FALSE'); 

DEFINE('FILE_ERRORLOG', './replikate.log'); // or '/var/log/replikate.log'

define('STATE_FILE_TEMPLATE', './states/replikate_state_%SOURCE%_%TARGET%.txt');
define('STATE_KEY_NAME', 'last_processed_id');

require_once('inc-constants.php');
require_once('inc-utils.php');

function fetchNextMessage($strSourceURL_a, $strSourceQueue_a, $strAfterName_a) 
{
    $arrResult = array(null, null);	// name, content

    $arrPostFields = [
        'action' => 'fetch',
        'q'      => $strSourceQueue_a,
        'after'  => $strAfterName_a
    ];

    $objCurl = curl_init($strSourceURL_a);

    curl_setopt($objCurl, CURLOPT_POST, 1);
    curl_setopt($objCurl, CURLOPT_POSTFIELDS, http_build_query($arrPostFields));
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

function publishToQueue($strTargetQueue_a, $intRetention_a, $binContent_a)
{
	$blnResult = true; 
	
    // Format retention days (r) as a 4-digit string
    $strRetentionDays = sprintf('%04d', $intRetention_a);

    // Prepare POST data
    $arrPostFields = [
        'action' => 'publish',
        'q'      => $strTargetQueue_a,
        'r'      => $strRetentionDays,
        'msg'    => $binContent_a
    ];

    // Initialize cURL
    $objCURL = curl_init();
    
    // Set cURL options for POST
    curl_setopt($objCURL, CURLOPT_URL, LOCAL_URL);
    curl_setopt($objCURL, CURLOPT_POST, 1);
    curl_setopt($objCURL, CURLOPT_POSTFIELDS, http_build_query($arrPostFields));
    curl_setopt($objCURL, CURLOPT_RETURNTRANSFER, true); // Get response as a string
    curl_setopt($objCURL, CURLOPT_TIMEOUT, 30); // Set a timeout

    // Execute and check response
    $strResponse = curl_exec($objCURL);
    $intHttpCode = curl_getinfo($objCURL, CURLINFO_HTTP_CODE);
    curl_close($objCURL);

    if ($intHttpCode !== 200) 
	{
        logError("Local POST to index.php failed with HTTP code: " . $intHttpCode);
        $blnResult = false;
    }
	else
	{
		// Decode and check API response for success
		$arrJSON = json_decode($strResponse, true);
		if ($arrJSON === null) 
		{
			logError("Local POST returned invalid JSON: " . $strResponse);
			$blnResult = false;
		}
		else
		{
			// Success is indicated by an empty error string
			if (strlen($arrJSON['error']) > 0) 
			{
				logError("Local API Error: " . $arrJSON['error'] . " / System Error: " . $arrJSON['system']);
				$blnResult = false;
			}
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

	$strSafeSource = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strSourceQueue_a);
	$strSafeTarget = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strTargetQueue_a);
	if (strlen($strTargetQueue_a) === 0)
	{
		$strSafeTarget = "store";
	}
	$strStateFilePath = STATE_FILE_TEMPLATE;
	$strStateFilePath = str_replace('%SOURCE%', $strSafeSource, $strStateFilePath);
	$strStateFilePath = str_replace('%TARGET%', $strSafeTarget, $strStateFilePath);
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
			//$blnSuccess = saveToQueue($strTargetQueue_a, $strName, $binContent);
			
			$intRetention = getRetentionFromName($strName);
			$blnSuccess = publishToQueue($strTargetQueue_a, $intRetention, $binContent);
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

if (strlen($strSourceQueue) === 0)	{ if (isset($_POST['sq'])) 	{ $strSourceQueue = $_POST['sq']; } }
if (strlen($strTargetQueue) === 0)	{ if (isset($_POST['tq'])) 	{ $strTargetQueue = $_POST['tq']; } }
if (strlen($strSourceURL) === 0)	{ if (isset($_POST['url'])) { $strSourceURL = $_POST['url']; } }

if (!$strSourceURL || !$strSourceQueue) 
{
	logError("Missing required arguments.");
	echo("Missing required arguments.");
	die();
} 

// --- Execution ---
handleReplikate($strSourceURL, $strSourceQueue, $strTargetQueue);
