<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (index.php)
// Deletes messages based on the RRRR (Retention Days) value in the name.
// 
// Execution: php index.php

define('ALLOW_GET', 'TRUE');

define('FILE_ERRORLOG', './zosciimq.log'); // or '/var/log/zosciimq.log'

require_once('inc-constants.php');
require_once('inc-utils.php');

function cleanUpLocks($strLocksPath_a)
{
	$intResult = 0;
	
    $intCutoffTime = time() - LOCK_TIMEFRAME;
    
    //logError("cleanUpLocks: Checking path: " . $strLocksPath_a . "*.lock");
    
    // Find all .lock files
    $arrLockFiles = glob($strLocksPath_a . '*.lock');
    
    //logError("cleanUpLocks: glob returned " . var_export($arrLockFiles, true));
    
    if (is_array($arrLockFiles)) 
	{
		logError("cleanUpLocks: Found " . count($arrLockFiles) . " lock files");
		
		foreach ($arrLockFiles as $strLockFile)
		{
			$intFileMTime = @filemtime($strLockFile);
			
			//logError("cleanUpLocks: Checking lock file: " . $strLockFile . " (mtime: " . $intFileMTime . ", cutoff: " . $intCutoffTime . ")");
			
			if ($intFileMTime !== false && $intFileMTime < $intCutoffTime)
			{
				if (@unlink($strLockFile))
				{
					//logError("cleanUpLocks: DELETED stale lock: " . $strLockFile);
				}
				else
				{
					logError("Failed to delete stale lock file: " . $strLockFile);
				}
			}
			else
			{
				//logError("cleanUpLocks: Lock file is fresh, keeping: " . $strLockFile);
			}
		}
    }
	else
	{
		logError("cleanUpLocks: glob did NOT return an array!");
	}
	
	// Re-scan
	$arrLockFiles = glob($strLocksPath_a . '*.lock');
    if (is_array($arrLockFiles)) 
	{
		$intResult = count($arrLockFiles);
	}
	
	//logError("cleanUpLocks: Returning lock count: " . $intResult);
	
    return $intResult;
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
	$strLockPath = QUEUE_ROOT . $strQueueName_a . '/' . LOCK_FOLDER;

	if (is_dir($strQueuePath)) 
	{
		// wait for lock to become free
		while (cleanUpLocks($strLockPath) > 0) 
		{
			usleep(LOCK_WAIT);
		}
		
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
		$strLockPath = QUEUE_ROOT . $strQueueName_a . '/' . LOCK_FOLDER;
		
		// creates the lock folder and the queue folder at the same time
		if (!is_dir($strLockPath)) 
		{
			if (!mkdir($strLockPath, FOLDER_PERMISSIONS, true)) 
			{
				sendJSONResponse("index.php: Could not create queue directory: " . $strLockPath, "Could not create queue.", "", []);
			} 
		}
		
		$strLockFile = getGUID() . ".lock";
		touch($strLockPath . $strLockFile);
		try
		{
			// Generate Name (YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin)
			$strBaseTime = date('YmdHis');
			$strName = '';
			$strFullPath = '';
			$strGetGUID = getGUID();
			
			$strFullTempPath = QUEUE_ROOT . TEMP_QUEUE . $strGetGUID . ".bin";
			$intBytesWritten = file_put_contents($strFullTempPath, $binMessage_a);

			$intCollisionCounter = 0;
			while (true) 
			{
				$strCollisionID = sprintf('%04d', $intCollisionCounter);
				$strName = $strBaseTime . $strCollisionID . "-" . $strRetentionDays . "-" . $strGetGUID . ".bin";
				$strFullPath = $strQueuePath . $strName;

				// Check for existence. If unique, break the loop.
				if (!file_exists($strFullPath)) 
				{
					break; 
				}

				// If file exists, we had a collision. Try the next sequential number.
				$intCollisionCounter++;
				
				// Safety break: Prevents an infinite loop.
				if ($intCollisionCounter > 9999) 
				{
					@unlink($strLockPath . $strLockFile);
					sendJSONResponse("index.php: Queue exceeded 9,999 attempted messages in one second.", "Queue overload, try again.", "", []);
				}
			}

			if (rename($strFullTempPath, $strFullPath))
			{
				if ($intBytesWritten === false) 
				{
					@unlink($strFullTempPath);
					@unlink($strLockPath . $strLockFile);
					sendJSONResponse("index.php: Failed to create message 1.", "Failed to create message.", "", []);
				} 
			}
			else
			{
				@unlink($strFullTempPath);
				@unlink($strLockPath . $strLockFile);
				sendJSONResponse("index.php: Failed to create message 2.", "Failed to create message.", "", []);
			}
			
			if (strlen($strNonce_a) > 0)
			{
				$strNonceFile = NONCE_ROOT . $strNonce_a;
				if (!touch($strNonceFile))
				{
					@unlink($strFullPath);
					@unlink($strLockPath . $strLockFile);
					sendJSONResponse("index.php: Failed to create nonce.", "Failed to create nonce.", "", []);
				}
			}

			@unlink($strLockPath . $strLockFile);
			sendJSONResponse("", "", "Message published.", []);
		}
		finally
		{
			@unlink($strLockPath . $strLockFile);
		}
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
				sendJSONResponse("index.php: Could not create store directory: " . $strStorePath, "Could not create store.", "", []);
			} 
		}

		$intBytesWritten = file_put_contents($strFullPath, $binMessage_a);
		if ($intBytesWritten === false) 
		{
			sendJSONResponse("index.php: Failed to create message.", "Failed to create message.", "", []);
		} 
		
		if (strlen($strNonce_a) > 0)
		{
			$strNonceFile = NONCE_ROOT . $strNonce_a;
			if (!touch($strNonceFile))
			{
				@unlink($strFullPath);
				sendJSONResponse("index.php: Failed to create nonce.", "Failed to create nonce.", "", []);
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