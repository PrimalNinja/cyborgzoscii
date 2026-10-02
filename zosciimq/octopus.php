<?php

// Cyborg ZOSCII MQ v20261003
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Octopus Adapter Framework (octopus.php)
//
// Pulls messages from a source queue, passes each through a pluggable adapter,
// and publishes the result to a target queue (or store).
//
// The adapter file must define:
//   function octopusProcess($strName_a, $binContent_a, $arrParams_a)
//   Returns: processed binary content (string), or null/false to skip the message.
//
// Adapters live in: ./adapters/<adaptername>.php
//
// Execution Example (CLI):
//   php octopus.php --url=http://other.server/index.php --sq=sourcequeue --tq=targetqueue --adapter=example
//   php octopus.php --url=http://other.server/index.php --sq=sourcequeue --tq=targetqueue --adapter=example --p1=foo --p2=bar
//
// Execution Example (HTTP):
//   http://your.server/octopus.php?url=http://other.server/index.php&sq=sourcequeue&tq=targetqueue&adapter=example
//
// NOTE: This script requires the PHP cURL extension to be installed and enabled.
// NOTE: If tq is omitted or empty, processed messages are saved to the store instead.

define('CLI_ONLY', 'FALSE');

define('FILE_ERRORLOG', './octopus.log'); // or '/var/log/octopus.log'

define('STATE_FILE_TEMPLATE', './states/octopus_state_%SOURCE%_%TARGET%_%ADAPTER%.txt');
define('STATE_KEY_NAME', 'last_processed_id');

define('ADAPTERS_FOLDER', './adapters/');

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

    $objCURL = curl_init();

    curl_setopt($objCURL, CURLOPT_URL, LOCAL_URL);
    curl_setopt($objCURL, CURLOPT_POST, 1);
    curl_setopt($objCURL, CURLOPT_POSTFIELDS, http_build_query($arrPostFields));
    curl_setopt($objCURL, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($objCURL, CURLOPT_TIMEOUT, 30);

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
		$arrJSON = json_decode($strResponse, true);
		if ($arrJSON === null)
		{
			logError("Local POST returned invalid JSON: " . $strResponse);
			$blnResult = false;
		}
		else
		{
			if (strlen($arrJSON['error']) > 0)
			{
				logError("Local API Error: " . $arrJSON['error'] . " / System Error: " . $arrJSON['system']);
				$blnResult = false;
			}
		}
	}

    return $blnResult;
}

function saveToStore($strName_a, $binContent_a)
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
		$intBytesWritten = file_put_contents($strFullPath, $binContent_a);
		if ($intBytesWritten === false)
		{
			$blnResult = false;
			logError("Failed to write file to store: " . $strFullPath);
		}
	}

    return $blnResult;
}

function handleOctopus($strSourceURL_a, $strSourceQueue_a, $strTargetQueue_a, $strAdapterName_a, $arrParams_a)
{
    echo("--- Starting ZOSCII MQ Octopus ---\n");
	echo("Adapter: " . $strAdapterName_a . "\n");

	// Load the adapter
	$strAdapterFile = ADAPTERS_FOLDER . $strAdapterName_a . '.php';
	if (!file_exists($strAdapterFile))
	{
		logError("Adapter file not found: " . $strAdapterFile);
		echo("Adapter file not found: " . $strAdapterFile . "\n");
		die();
	}
	require_once($strAdapterFile);
	if (!function_exists('octopusProcess'))
	{
		logError("Adapter does not define octopusProcess(): " . $strAdapterFile);
		echo("Adapter does not define octopusProcess(): " . $strAdapterFile . "\n");
		die();
	}

	// Build state file path — unique per source queue + target + adapter
	$strSafeSource = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strSourceQueue_a);
	$strSafeTarget = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strTargetQueue_a);
	if (strlen($strTargetQueue_a) === 0)
	{
		$strSafeTarget = "store";
	}
	$strSafeAdapter = preg_replace('/[^a-zA-Z0-9_-]/', '_', $strAdapterName_a);
	$strStateFilePath = STATE_FILE_TEMPLATE;
	$strStateFilePath = str_replace('%SOURCE%',  $strSafeSource,  $strStateFilePath);
	$strStateFilePath = str_replace('%TARGET%',  $strSafeTarget,  $strStateFilePath);
	$strStateFilePath = str_replace('%ADAPTER%', $strSafeAdapter, $strStateFilePath);

	// Ensure states directory exists
	$strStatesDir = dirname($strStateFilePath);
	if (!is_dir($strStatesDir))
	{
		if (!mkdir($strStatesDir, FOLDER_PERMISSIONS, true))
		{
			logError("Could not create states directory: " . $strStatesDir);
			echo("Could not create states directory: " . $strStatesDir . "\n");
			die();
		}
	}

	$strLastName = '';
	if (file_exists($strStateFilePath))
	{
		$strLastName = file_get_contents($strStateFilePath);
	}

	if (empty($strLastName))
	{
		echo("Last Processed Pointer: START\n");
	}
	else
	{
		echo("Last Processed Pointer: " . $strLastName . "\n");
	}

	$intTotalProcessed = 0;
	$intTotalSkipped = 0;

	while (true)
	{
		list($strName, $binContent) = fetchNextMessage($strSourceURL_a, $strSourceQueue_a, $strLastName);

		if ($strName === null)
		{
			echo("Source queue is caught up or returned no data. Halting this run.\n");
			break;
		}

		// Call the adapter
		$binProcessed = octopusProcess($strName, $binContent, $arrParams_a);

		if ($binProcessed === null || $binProcessed === false)
		{
			// Adapter chose to skip this message — advance the pointer anyway
			$strLastName = $strName;
			$intTotalSkipped++;
			echo("SKIPPED: " . $strName . "\n");
			file_put_contents($strStateFilePath, $strLastName);
			continue;
		}

		// Publish or store the processed result
		if (strlen($strTargetQueue_a) > 0)
		{
			$intRetention = getRetentionFromName($strName);
			$blnSuccess = publishToQueue($strTargetQueue_a, $intRetention, $binProcessed);
			if ($blnSuccess)
			{
				$strLastName = $strName;
				$intTotalProcessed++;
				echo("PROCESSED: New pointer set to " . $strLastName . "\n");
				file_put_contents($strStateFilePath, $strLastName);
			}
			else
			{
				logError("Failed to publish processed message to " . $strTargetQueue_a . ". Halting.");
				echo("Failed to publish processed message to " . $strTargetQueue_a . ". Halting.\n");
				break;
			}
		}
		else
		{
			$blnSuccess = saveToStore($strName, $binProcessed);
			if ($blnSuccess)
			{
				$strLastName = $strName;
				$intTotalProcessed++;
				echo("PROCESSED (store): New pointer set to " . $strLastName . "\n");
				file_put_contents($strStateFilePath, $strLastName);
			}
			else
			{
				logError("Failed to save processed message to store. Halting.");
				echo("Failed to save processed message to store. Halting.\n");
				break;
			}
		}
	}

	echo("--- Octopus finished. Processed: " . $intTotalProcessed . " | Skipped: " . $intTotalSkipped . " ---\n");
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

if (!extension_loaded('curl'))
{
    echo("\nThe cURL PHP extension is not installed or enabled. Please enable it in your php.ini.\n");
    exit(1);
}

initFolders();

// Get parameters — CLI (--key=value) or HTTP (GET/POST)

$strSourceURL   = '';
$strSourceQueue = '';
$strTargetQueue = '';
$strAdapter     = '';
$arrParams      = array();

if (php_sapi_name() === 'cli')
{
	// Parse --key=value arguments
	$arrArgs = array();
	foreach (array_slice($argv, 1) as $strArg)
	{
		if (preg_match('/^--([a-zA-Z0-9_]+)=(.*)$/', $strArg, $arrMatch))
		{
			$arrArgs[$arrMatch[1]] = $arrMatch[2];
		}
	}
	if (isset($arrArgs['url']))     { $strSourceURL   = $arrArgs['url']; }
	if (isset($arrArgs['sq']))      { $strSourceQueue = $arrArgs['sq']; }
	if (isset($arrArgs['tq']))      { $strTargetQueue = $arrArgs['tq']; }
	if (isset($arrArgs['adapter'])) { $strAdapter     = $arrArgs['adapter']; }
	// Collect any extra --p* or adapter-specific params
	foreach ($arrArgs as $strKey => $strVal)
	{
		if (!in_array($strKey, array('url', 'sq', 'tq', 'adapter')))
		{
			$arrParams[$strKey] = $strVal;
		}
	}
}
else
{
	if (isset($_GET['url']))     { $strSourceURL   = $_GET['url']; }
	if (isset($_GET['sq']))      { $strSourceQueue = $_GET['sq']; }
	if (isset($_GET['tq']))      { $strTargetQueue = $_GET['tq']; }
	if (isset($_GET['adapter'])) { $strAdapter     = $_GET['adapter']; }

	if (strlen($strSourceURL)   === 0) { if (isset($_POST['url']))     { $strSourceURL   = $_POST['url']; } }
	if (strlen($strSourceQueue) === 0) { if (isset($_POST['sq']))      { $strSourceQueue = $_POST['sq']; } }
	if (strlen($strTargetQueue) === 0) { if (isset($_POST['tq']))      { $strTargetQueue = $_POST['tq']; } }
	if (strlen($strAdapter)     === 0) { if (isset($_POST['adapter'])) { $strAdapter     = $_POST['adapter']; } }

	// Collect extra GET/POST params (anything not a core key)
	$arrCoreKeys = array('url', 'sq', 'tq', 'adapter', 'action');
	foreach (array_merge($_GET, $_POST) as $strKey => $strVal)
	{
		if (!in_array($strKey, $arrCoreKeys))
		{
			$arrParams[$strKey] = $strVal;
		}
	}
}

// Validate required parameters
if (!$strSourceURL || !$strSourceQueue || !$strAdapter)
{
	$strMsg = "Missing required arguments. Required: url, sq, adapter. Optional: tq (omit to save to store).";
	logError($strMsg);
	echo($strMsg . "\n");
	die();
}

// Sanitise adapter name — only alphanumeric, dash, underscore (no path traversal)
$strAdapter = preg_replace('/[^a-zA-Z0-9_-]/', '', $strAdapter);
if (strlen($strAdapter) === 0)
{
	logError("Invalid adapter name.");
	echo("Invalid adapter name.\n");
	die();
}

handleOctopus($strSourceURL, $strSourceQueue, $strTargetQueue, $strAdapter, $arrParams);