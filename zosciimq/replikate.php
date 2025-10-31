<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ External Server-to-Q Replikate Cron Job (replikate.php)
// 
// Pulls messages from an External URL and publishes them to a local Target Queue.
// State is managed using a unique file per target queue, making it concurrency-safe.
// 
// Execution Example:
// http://your.server/replikate.php?url=http://other.server/index.php&sq=sourcequeue&tq=targetqueue
// 
// NOTE: This script requires the PHP cURL extension to be installed and enabled.

// --- Global Configuration ---
define('QUEUE_ROOT', './queues/');

define('CLI_ONLY', 'FALSE'); 

DEFINE('LOG_OUTPUT', 'TRUE');	// TRUE or FALSE
DEFINE('FILE_ERRORLOG', './replikate.log'); // or '/var/log/replikate.log'

define('STATE_FILE_TEMPLATE', './replikate_state_%IDENTIFIER%.txt');
define('STATE_KEY_NAME', 'last_processed_id');

// --- Helper Functions ---
function logDebug($str_a)
{
	if (DEBUG == 'TRUE')
	{
		echo($str_a . "\n");
	}
}

function logError($str_a)
{
	if (LOG_OUTPUT == 'TRUE')
	{
		file_put_contents(FILE_ERRORLOG, date('Y-m-d H:i:s') . " - " . $str_a . "\n", FILE_APPEND);
	}
}

function fetchNextMessageExternal($strSourceURL_a, $strSourceQueue_a, $strAfterFilename_a) 
{
	$strSourceURL = $strSourceURL_a;
	$strSourceQueue = $strSourceQueue_a;
	$strAfterFilename = $strAfterFilename_a;
	
    $arrResult = array(null, null);

	$strFetchURL = $strSourceURL . '?action=fetch&q=' . urlencode($strSourceQueue) . '&after=' . urlencode($strAfterFilename);
	//echo(" FETCHING: " . $strFetchURL . "\n");

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
			logError("SERVER ERROR: Received HTTP Status " . $intHttpStatus . ". Server response: " . $binContent);
		} 
		else 
		{
			preg_match('/Content-Disposition: attachment; filename="([^"]+)"/i', $strHeaders, $matches);
			$strFilename = isset($matches[1]) ? $matches[1] : null;

			if ($strFilename && $binContent !== false) 
			{
				$arrResult = array($strFilename, $binContent);
			} else {
				logError("ERROR: Invalid response structure (missing filename or content).");
			}
		}
	}
	curl_close($objCurl);

    return $arrResult;
}

function saveToQueue($strQueueName_a, $strFilename_a, $strMessage_a) 
{
    $blnResult = true;

	$strQueuePath = QUEUE_ROOT . $strQueueName_a . '/';
	if (!is_dir($strQueuePath)) 
	{
		if (!mkdir($strQueuePath, 0777, true)) 
		{
			$blnResult = false;
			logError("Error: Could not create queue directory: " . $strQueuePath);
			$strResult = "Error: Could not create queue directory: " . $strQueuePath;
		} 
	}
		
	if ($blnResult)
	{
		$strFullPath = $strQueuePath . $strFilename_a;
		$intBytesWritten = file_put_contents($strFullPath, $strMessage_a);
		if ($intBytesWritten === false) 
		{
			$blnResult = false;
		}
	}

    return $blnResult;
}

// --- Main Replikation Logic ---
function handleReplikateExternal($strSourceURL_a, $strSourceQueue_a, $strTargetQueue_a) 
{
	$strSourceURL = $strSourceURL_a;
	$strSourceQueue = $strSourceQueue_a;
	$strTargetQueue = $strTargetQueue_a;
	
    echo("--- Starting ZOSCII MQ External Replikation ---\n");

	$strSafeIdentifier = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strTargetQueue);
	$strStateFilePath = str_replace('%IDENTIFIER%', $strSafeIdentifier, STATE_FILE_TEMPLATE);
	$strStateKey = STATE_KEY_NAME;

	$strLastFilename = '';
	if (file_exists($strStateFilePath)) 
	{
		$strLastFilename = file_get_contents($strStateFilePath);
	}

	//echo("Processing URL: " . $strSourceURL . " -> Target Q: " . $strTargetQueue . "\n");
	if (empty($strLastFilename)) 
	{
		//echo("Last Processed Pointer: START\n");
	} 
	else 
	{
		//echo("Last Processed Pointer: " . $strLastFilename . "\n");
	}
	$intTotalReplikated = 0;

	while (true) 
	{
		list($strFilename, $binContent) = fetchNextMessageExternal($strSourceURL, $strSourceQueue, $strLastFilename);

		if ($strFilename === null) 
		{
			logError("INFO: Source URL is caught up or returned no data. Halting this run.");
			break;
		}

		$blnSuccess = saveToQueue($strTargetQueue, $strFilename, $binContent);
		if ($blnSuccess) 
		{
			$strLastFilename = $strFilename;
			$intTotalReplikated++;
			//echo(" REPLIKATED: New pointer set to " . $strLastFilename . "\n");
		} 
		else 
		{
			logError("FATAL ERROR: Failed to publish message to " . $strTargetQueue . ". Halting.");
			break;
		}
	}

	file_put_contents($strStateFilePath, $strLastFilename);
	echo("--- Replikation finished. Total messages replikated: " . $intTotalReplikated . " ---\n");
	//echo("State saved to: " . $strStateFilePath . "\n");
}

// --- Main Entry

if (CLI_ONLY == 'TRUE')
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
    echo("\nFATAL ERROR: The cURL PHP extension is not installed or enabled. Please enable it in your php.ini.\n");
    echo("--- Test Halted ---\n");
    exit(1);
}

// Ensure the root queue directory exists
if (!is_dir(QUEUE_ROOT)) 
{
    if (!mkdir(QUEUE_ROOT, 0777, true)) 
	{
        logError("Fatal Error: Could not create root queue directory " . QUEUE_ROOT);
        echo("Fatal Error: Could not create root queue directory " . QUEUE_ROOT);
        exit;
    }
}

$strSourceURL = null;
if (isset($_GET['url'])) 
{
	$strSourceURL = $_GET['url'];
} 

$strSourceQueue = null;
if (isset($_GET['sq'])) 
{
	$strSourceQueue = $_GET['sq'];
} 

$strTargetQueue = null;
if (isset($_GET['tq'])) 
{
	$strTargetQueue = $_GET['tq'];
} 

if (!$strSourceURL || !$strSourceQueue || !$strTargetQueue) 
{
	logError("FATAL ERROR: Missing required arguments.");
	//echo("Usage: http://your.server/replikate.php?url=<API_URL>&sq=<S_Q_NAME>&tq=<T_Q_NAME>\n");
	//echo("NOTE: Source URL must point to index.php?action=fetch.\n");
	exit;
} 

// --- Execution ---
handleReplikateExternal($strSourceURL, $strSourceQueue, $strTargetQueue);
