<?php

// Cyborg ZOSCII MQ v20261002
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (index.php)
// Deletes messages based on the RRRR (Retention Days) value in the name.
//
// Execution: php index.php

define('ALLOW_FETCH', 'TRUE');
define('ALLOW_GET', 'FALSE');
define('ALLOW_IDENTIFY', 'FALSE');
define('ALLOW_PUBLISH', 'TRUE');
define('ALLOW_RETRIEVE', 'FALSE');
define('ALLOW_SCAN', 'FALSE');
define('ALLOW_STORE', 'FALSE');
define('ALLOW_TRANSACTIONS', 'TRUE');	// upload / commit / abort (a commit also needs ALLOW_PUBLISH or ALLOW_STORE)

// A transaction with no upload, commit or abort for this long is deleted, along with
// anything staged in it. Also the age at which a leftover single-upload temp file is removed.
define('TRANSACTION_EXPIRY', 86400);	// in seconds

// Allowed file extensions for fetch operations
$arrAllowedFetchExtensions = array('bin', 'mp3', 'txt', 'jpg', 'jpeg');

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

// Only one publish at a time may pick a name and rename in a queue, so the name
// records the order publishes FINISHED. Without it, two publishes in the same
// second can both take CCCC 0000 (neither has renamed yet when the other looks)
// and their names then sort by GUID, which is not the order they happened.
function lockPublish($strLocksPath_a)
{
	return lockMutex($strLocksPath_a . 'publish.lock');
}

function unlockPublish($strMutexFile_a)
{
	unlockMutex($strMutexFile_a);
}

// Takes the mutex file (created exclusively). A mutex older than LOCK_TIMEFRAME is
// treated as left behind by a request that died and is taken over. A holder that
// keeps it longer (a large commit) must touch() it to keep it fresh.
function lockMutex($strMutexFile_a)
{
	$objMutex = @fopen($strMutexFile_a, 'x');
	$intWaited = 0;

	while ($objMutex === false && $intWaited < (LOCK_TIMEFRAME * 2 * 1000000))
	{
		$intFileMTime = @filemtime($strMutexFile_a);

		if ($intFileMTime !== false && $intFileMTime < time() - LOCK_TIMEFRAME)
		{
			// left behind by a request that died
			@unlink($strMutexFile_a);
		}
		else
		{
			usleep(LOCK_WAIT);
			$intWaited += LOCK_WAIT;
		}

		$objMutex = @fopen($strMutexFile_a, 'x');
	}

	if ($objMutex !== false)
	{
		fclose($objMutex);
	}
	else
	{
		logError("lockMutex: gave up waiting for " . $strMutexFile_a);
	}

	return $strMutexFile_a;
}

function unlockMutex($strMutexFile_a)
{
	@unlink($strMutexFile_a);
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

// -----------------------------------------------------------------------------
// Temp folder and transactions
//
// Single publish / store: the payload is written to QUEUE_ROOT/temp/<guid>.bin,
// checked, then renamed to its final name, so it never appears half written.
//
// Transaction: QUEUE_ROOT/temp/<txguid>/ holds the uploads until a commit moves
// them all to one queue (in upload order) or into the store.
//   NNNNNNNN-<guid>.bin   staged upload, NNNNNNNN = upload order
//   tx.lock               mutex for this transaction
//   next.seq              next upload order number
//   commit.json           destination, written when a commit starts
//   commit.log            "staged|final" per file, written before each move
//   committed.json        final names in upload order, once the commit is done
// -----------------------------------------------------------------------------

function getTempRoot()
{
	return QUEUE_ROOT . TEMP_QUEUE;
}

function isGUID($str_a)
{
	return (preg_match('/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/', $str_a) === 1);
}

// Store folder for a name: the first 3 characters of its base 36 form.
function getStorePath($strName_a)
{
	$strTempName = convertNameToBase36($strName_a);
	$strDir1 = substr($strTempName, 0, 1);
	$strDir2 = substr($strTempName, 1, 1);
	$strDir3 = substr($strTempName, 2, 1);

	return STORE_ROOT . $strDir1 . '/' . $strDir2 . '/' . $strDir3 . '/';
}

// Moves a finished file to its final name without it ever being visible there half
// written. Within one disk both renames are atomic. If the temp folder is on another
// disk (e.g. separate Docker volumes) PHP's rename copies - the copy lands on a hidden
// .part name in the destination folder, then a same-folder rename makes it visible.
function moveIntoPlace($strSource_a, $strDest_a)
{
	$blnResult = false;
	$strPart = dirname($strDest_a) . '/.' . basename($strDest_a) . '.part';

	if (@rename($strSource_a, $strPart))
	{
		if (@rename($strPart, $strDest_a))
		{
			$blnResult = true;
		}
		else
		{
			@rename($strPart, $strSource_a);
		}
	}

	return $blnResult;
}

function deleteFolder($strPath_a)
{
	$arrFiles = @scandir($strPath_a);

	if (is_array($arrFiles))
	{
		foreach ($arrFiles as $strFile)
		{
			if ($strFile !== '.' && $strFile !== '..')
			{
				@unlink($strPath_a . $strFile);
			}
		}
	}

	@rmdir($strPath_a);
}

// Removes transactions with no activity for TRANSACTION_EXPIRY (uploading, moving or
// deleting files in the folder updates its time), and single-upload temp files left
// behind by a request that died.
function cleanUpTransactions($strKeepTx_a)
{
	$intCutoffTime = time() - TRANSACTION_EXPIRY;
	$arrItems = glob(getTempRoot() . '*');

	if (is_array($arrItems))
	{
		foreach ($arrItems as $strItem)
		{
			$intFileMTime = @filemtime($strItem);

			if ($intFileMTime !== false && $intFileMTime < $intCutoffTime && basename($strItem) !== $strKeepTx_a)
			{
				if (is_dir($strItem))
				{
					deleteFolder($strItem . '/');
				}
				else
				{
					@unlink($strItem);
				}
			}
		}
	}
}

function readCommitLog($strTxPath_a)
{
	$arrResult = [];
	$arrLines = @file($strTxPath_a . 'commit.log', FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	if (is_array($arrLines))
	{
		foreach ($arrLines as $strLine)
		{
			$arrParts = explode('|', $strLine);

			if (count($arrParts) === 2)
			{
				$arrResult[$arrParts[0]] = $arrParts[1];
			}
		}
	}

	return $arrResult;
}

function handleUpload($strTx_a, $strNonce_a, $binMessage_a)
{
	if (!isGUID($strTx_a))
	{
		sendJSONResponse("", "Missing or invalid 't' (transaction GUID).", "", []);
	}
	else if (empty($binMessage_a))
	{
		sendJSONResponse("", "Message required.", "", []);
	}
	else
	{
		cleanUpTransactions($strTx_a);

		$strTxPath = getTempRoot() . $strTx_a . '/';

		if (!is_dir($strTxPath))
		{
			if (!@mkdir($strTxPath, FOLDER_PERMISSIONS, true) && !is_dir($strTxPath))
			{
				sendJSONResponse("index.php: Could not create transaction directory: " . $strTxPath, "Could not create transaction.", "", []);
			}
		}

		// the payload is written before the mutex is taken, so a large upload does not
		// hold up other uploads to the same transaction
		$strUploadPath = $strTxPath . getGUID() . '.upload';
		$intBytesWritten = file_put_contents($strUploadPath, $binMessage_a);

		if ($intBytesWritten === false || $intBytesWritten !== strlen($binMessage_a))
		{
			@unlink($strUploadPath);
			sendJSONResponse("index.php: Failed to write upload.", "Failed to create message.", "", []);
		}

		$strMutexFile = lockMutex($strTxPath . 'tx.lock');

		if (file_exists($strTxPath . 'commit.json') || file_exists($strTxPath . 'committed.json'))
		{
			@unlink($strUploadPath);
			unlockMutex($strMutexFile);
			sendJSONResponse("", "Transaction is already committed or being committed.", "", []);
		}

		$intSeq = (int)@file_get_contents($strTxPath . 'next.seq');
		$strStagedName = sprintf('%08d', $intSeq) . '-' . getGUID() . '.bin';

		if (!rename($strUploadPath, $strTxPath . $strStagedName))
		{
			@unlink($strUploadPath);
			unlockMutex($strMutexFile);
			sendJSONResponse("index.php: Failed to stage upload.", "Failed to create message.", "", []);
		}

		file_put_contents($strTxPath . 'next.seq', (string)($intSeq + 1));
		unlockMutex($strMutexFile);

		if (strlen($strNonce_a) > 0)
		{
			$strNonceFile = NONCE_ROOT . $strNonce_a;
			if (!touch($strNonceFile))
			{
				// the upload is staged - only the retry protection is missing
				logError("index.php: Failed to create nonce for upload.");
			}
		}

		sendJSONResponse("", "", "Message uploaded.", $intSeq + 1);
	}
}

// Moves every staged upload to the queue 'q' (in upload order, through the publish
// lock, exactly as if published one after another) or, with no 'q', into the store.
// Retention 'r' and unidentified 'u' apply to every file. A commit that stops part way
// (timeout, error) carries on where it stopped when it is sent again; a commit sent
// after it finished returns the same names again.
function handleCommit($strTx_a, $strQueueName_a, $intRetentionDays_a, $blnUnidentified_a)
{
	global $arrPublishBlocks;

	if (!isGUID($strTx_a))
	{
		sendJSONResponse("", "Missing or invalid 't' (transaction GUID).", "", []);
	}

	$strTxPath = getTempRoot() . $strTx_a . '/';

	if (!is_dir($strTxPath))
	{
		sendJSONResponse("", "Transaction not found.", "", []);
	}

	cleanUpTransactions($strTx_a);
	set_time_limit(0);

	$strMutexFile = lockMutex($strTxPath . 'tx.lock');

	if (file_exists($strTxPath . 'committed.json'))
	{
		$arrNames = json_decode((string)@file_get_contents($strTxPath . 'committed.json'), true);
		unlockMutex($strMutexFile);
		sendJSONResponse("", "", "Transaction already committed.", is_array($arrNames) ? $arrNames : []);
	}

	// The destination is fixed by the first commit; a repeat carries on with it.
	$arrPlan = json_decode((string)@file_get_contents($strTxPath . 'commit.json'), true);

	if (!is_array($arrPlan))
	{
		$arrPlan = ['q' => $strQueueName_a, 'r' => $intRetentionDays_a, 'u' => $blnUnidentified_a];
	}

	$strQueueName = (string)$arrPlan['q'];
	$strRetentionDays = sprintf('%04d', (int)$arrPlan['r']);
	$blnUnidentified = (bool)$arrPlan['u'];
	$blnToQueue = (strlen($strQueueName) > 0);

	if ($blnToQueue && ALLOW_PUBLISH !== 'TRUE')
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Publishing is not allowed.", "", []);
	}

	if ($blnToQueue && in_array($strQueueName, $arrPublishBlocks))
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Invalid action 'commit' for provided queue.", "", []);
	}

	if (!$blnToQueue && ALLOW_STORE !== 'TRUE')
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Storing is not allowed.", "", []);
	}

	$arrStaged = glob($strTxPath . '*.bin');
	if (!is_array($arrStaged))
	{
		$arrStaged = [];
	}
	sort($arrStaged);

	$arrLog = readCommitLog($strTxPath);

	if (count($arrStaged) === 0 && count($arrLog) === 0)
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Transaction is empty.", "", []);
	}

	if (!file_exists($strTxPath . 'commit.json'))
	{
		file_put_contents($strTxPath . 'commit.json', json_encode($arrPlan));
	}

	$strQueuePath = QUEUE_ROOT . $strQueueName . '/';
	$strLockPath = $strQueuePath . LOCK_FOLDER;
	$strPublishMutex = '';
	$strFetchLock = '';
	$intTime = time();
	$intCounter = 0;
	$arrTaken = [];
	$strTakenSecond = '';
	$intLastTouch = time();
	$strError = '';

	if ($blnToQueue)
	{
		if (!is_dir($strLockPath))
		{
			if (!@mkdir($strLockPath, FOLDER_PERMISSIONS, true) && !is_dir($strLockPath))
			{
				unlockMutex($strMutexFile);
				sendJSONResponse("index.php: Could not create queue directory: " . $strLockPath, "Could not create queue.", "", []);
			}
		}

		// fetches wait while this lock exists, so they see the whole commit at once
		$strFetchLock = $strLockPath . getGUID() . ".lock";
		touch($strFetchLock);
		$strPublishMutex = lockPublish($strLockPath);
	}

	foreach ($arrStaged as $strStagedPath)
	{
		if (strlen($strError) === 0)
		{
			$strStaged = basename($strStagedPath);
			$strFinal = '';
			$strFinalPath = '';

			if (isset($arrLog[$strStaged]))
			{
				// named before a commit that stopped part way - keep that name
				$strFinal = $arrLog[$strStaged];
			}
			else if ($blnToQueue)
			{
				// YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin, CCCC counting up in upload order,
				// carrying into the next second past 9999
				$blnFree = false;

				while (!$blnFree)
				{
					$strBaseTime = date('YmdHis', $intTime);

					if ($strTakenSecond !== $strBaseTime)
					{
						$arrTaken = [];
						$arrSecond = glob($strQueuePath . $strBaseTime . '*');

						if (is_array($arrSecond))
						{
							foreach ($arrSecond as $strTakenPath)
							{
								$arrTaken[substr(basename($strTakenPath), 14, 4)] = true;
							}
						}

						$strTakenSecond = $strBaseTime;
					}

					$strCollisionID = sprintf('%04d', $intCounter);

					if (!isset($arrTaken[$strCollisionID]))
					{
						$strFinal = $strBaseTime . $strCollisionID . "-" . $strRetentionDays . "-" . getGUID() . ".bin";
						$arrTaken[$strCollisionID] = true;
						$blnFree = true;
					}
					else
					{
						$intCounter++;

						if ($intCounter > 9999)
						{
							$intTime++;
							$intCounter = 0;
						}
					}
				}
			}
			else
			{
				$strFinal = date('YmdHis') . "0000-" . $strRetentionDays . "-" . getGUID() . ($blnUnidentified ? '-u' : '') . ".bin";
			}

			if (!isset($arrLog[$strStaged]))
			{
				// recorded before the move, so a commit cut off between the two still
				// knows where this file went
				file_put_contents($strTxPath . 'commit.log', $strStaged . '|' . $strFinal . "\n", FILE_APPEND | LOCK_EX);
				$arrLog[$strStaged] = $strFinal;
			}

			if ($blnToQueue)
			{
				$strFinalPath = $strQueuePath . $strFinal;
			}
			else
			{
				$strStorePath = getStorePath($strFinal);

				if (!is_dir($strStorePath))
				{
					@mkdir($strStorePath, FOLDER_PERMISSIONS, true);
				}

				$strFinalPath = $strStorePath . $strFinal;
			}

			if (!moveIntoPlace($strStagedPath, $strFinalPath))
			{
				$strError = "Commit stopped at upload " . ((int)substr($strStaged, 0, 8) + 1) . " - send commit again to carry on.";
			}

			// a large commit keeps its locks fresh so they are not taken over as stale
			if (time() !== $intLastTouch)
			{
				touch($strMutexFile);

				if ($blnToQueue)
				{
					touch($strPublishMutex);
					touch($strFetchLock);
				}

				$intLastTouch = time();
			}
		}
	}

	if ($blnToQueue)
	{
		unlockPublish($strPublishMutex);
		@unlink($strFetchLock);
	}

	if (strlen($strError) > 0)
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("index.php: " . $strError, $strError, "", []);
	}

	// names in upload order, kept so a repeated commit can answer with them again
	ksort($arrLog);
	$arrNames = array_values($arrLog);
	file_put_contents($strTxPath . 'committed.json', json_encode($arrNames));
	@unlink($strTxPath . 'commit.log');
	@unlink($strTxPath . 'commit.json');
	@unlink($strTxPath . 'next.seq');
	unlockMutex($strMutexFile);

	sendJSONResponse("", "", "Transaction committed.", $arrNames);
}

function handleAbort($strTx_a)
{
	if (!isGUID($strTx_a))
	{
		sendJSONResponse("", "Missing or invalid 't' (transaction GUID).", "", []);
	}

	$strTxPath = getTempRoot() . $strTx_a . '/';

	if (!is_dir($strTxPath))
	{
		sendJSONResponse("", "Transaction not found.", "", []);
	}

	$strMutexFile = lockMutex($strTxPath . 'tx.lock');

	if (file_exists($strTxPath . 'committed.json'))
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Transaction already committed.", "", []);
	}

	if (file_exists($strTxPath . 'commit.json'))
	{
		unlockMutex($strMutexFile);
		sendJSONResponse("", "Transaction is part way through a commit - send commit again to finish it.", "", []);
	}

	unlockMutex($strMutexFile);
	deleteFolder($strTxPath);

	sendJSONResponse("", "", "Transaction aborted.", []);
}

function handleFetch($strQueueName_a, $strAfterName_a, $intOffset_a, $intLength_a, $blnReverse_a)
{
    global $arrAllowedFetchExtensions;

    $intLength = $intLength_a;
    $strQueuePath = QUEUE_ROOT . $strQueueName_a . '/';
    $strLockPath = QUEUE_ROOT . $strQueueName_a . '/' . LOCK_FOLDER;

    if (is_dir($strQueuePath))
    {
        // wait for lock to become free
        while (cleanUpLocks($strLockPath) > 0)
        {
            usleep(LOCK_WAIT);
        }

        // Get ALL files (not just .bin)
        $arrAllFiles = glob($strQueuePath . '*');
        if ($arrAllFiles === false || empty($arrAllFiles))
        {
            sendJSONResponse("", "", "Queue is empty.", []);
        }
        else
        {
            // Sort files alphabetically (chronologically by name), reversed when
            // reading backwards
            if ($blnReverse_a)
            {
                rsort($arrAllFiles);
            }
            else
            {
                sort($arrAllFiles);
            }
            $strNextMessagePath = null;

            // reverse: an empty pointer starts at the newest message
            if ($blnReverse_a && $strAfterName_a === '')
            {
                $strAfterName_a = '~';  // sorts above any message name
            }

            foreach ($arrAllFiles as $strFullPath)
            {
                $strName = basename($strFullPath);

                // Skip directories
                if (is_dir($strFullPath))
                {
                    continue;
                }

                // Check if extension is allowed BEFORE comparing pointer
                $strExtension = strtolower(pathinfo($strName, PATHINFO_EXTENSION));
                if (!in_array($strExtension, $arrAllowedFetchExtensions))
                {
                    continue;  // Skip disallowed extensions entirely
                }

                // forward: the first message after the pointer
                // reverse: the last one before it
                $blnFound = ($strName > $strAfterName_a);
                if ($blnReverse_a)
                {
                    $blnFound = ($strName < $strAfterName_a);
                }

                if ($blnFound)
                {
                    $strNextMessagePath = $strFullPath;
                    break;
                }
            }

            if ($strNextMessagePath)
            {
                $strName = basename($strNextMessagePath);

                // Extension already checked above, but double-check
                $strExtension = strtolower(pathinfo($strName, PATHINFO_EXTENSION));
                if (!in_array($strExtension, $arrAllowedFetchExtensions))
                {
                    sendJSONResponse("", "File extension '" . $strExtension . "' not allowed for fetch.", "", []);
                }

                if ($intOffset_a === 0 && $intOffset_a === 0)
                {
                    // Set correct Content-Type based on extension
                    $strContentType = 'application/octet-stream';
                    if ($strExtension === 'mp3')
                    {
                        $strContentType = 'audio/mpeg';
                    }
                    elseif ($strExtension === 'jpg' || $strExtension === 'jpeg')
                    {
                        $strContentType = 'image/jpeg';
                    }
                    elseif ($strExtension === 'txt')
                    {
                        $strContentType = 'text/plain';
                    }

                    header('Content-Type: ' . $strContentType);
                    header('Content-Disposition: attachment; filename="' . $strName . '"');

                    readfile($strNextMessagePath);
                    die();
                }

                $intFileSize = filesize($strNextMessagePath);

                if ($intOffset_a < 0 || $intOffset_a >= $intFileSize)
                {
                    sendJSONResponse("", "Invalid offset.", "", []);
                }

                if ($intLength <= 0 || ($intOffset_a + $intLength) > $intFileSize)
                {
                    $intLength = $intFileSize - $intOffset_a;
                }

                // Set correct Content-Type based on extension
                $strContentType = 'application/octet-stream';
                if ($strExtension === 'mp3')
                {
                    $strContentType = 'audio/mpeg';
                }
                elseif ($strExtension === 'jpg' || $strExtension === 'jpeg')
                {
                    $strContentType = 'image/jpeg';
                }
                elseif ($strExtension === 'txt')
                {
                    $strContentType = 'text/plain';
                }

                header('Content-Type: ' . $strContentType);
                header('Content-Disposition: attachment; filename="' . $strName . '"');
                header('Content-Length: ' . $intLength);
                header('X-ZOSCII-Offset: ' . $intOffset_a);
                header('X-ZOSCII-Total-Length: ' . $intFileSize);

                $objFile = fopen($strNextMessagePath, 'rb');
                if ($objFile === false)
                {
                    sendJSONResponse("", "Failed to open file.", "", []);
                }

                fseek($objFile, $intOffset_a);
                $binChunk = fread($objFile, $intLength);
                fclose($objFile);

                echo($binChunk);
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
		$strMutexFile = '';
		try
		{
			$strName = '';
			$strFullPath = '';
			$strGetGUID = getGUID();

			// the payload is written before the mutex is taken, so a large
			// message does not hold up the other publishers
			$strFullTempPath = QUEUE_ROOT . TEMP_QUEUE . $strGetGUID . ".bin";
			$intBytesWritten = file_put_contents($strFullTempPath, $binMessage_a);

			// a failed or short write never reaches the queue
			if ($intBytesWritten === false || $intBytesWritten !== strlen($binMessage_a))
			{
				@unlink($strFullTempPath);
				@unlink($strLockPath . $strLockFile);
				sendJSONResponse("index.php: Failed to create message 1.", "Failed to create message.", "", []);
			}

			$strMutexFile = lockPublish($strLockPath);

			// Generate Name (YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin)
			// the time is read inside the mutex, so a publisher that waited cannot
			// stamp an earlier second than one that has already renamed
			$strBaseTime = date('YmdHis');

			$intCollisionCounter = 0;
			while (true)
			{
				$strCollisionID = sprintf('%04d', $intCollisionCounter);
				$strName = $strBaseTime . $strCollisionID . "-" . $strRetentionDays . "-" . $strGetGUID . ".bin";
				$strFullPath = $strQueuePath . $strName;

				// Check for any message already using this time and counter, any
				// extension. The GUID is left out on purpose: it is unique every
				// time, so including it always looks free and CCCC stays 0000.
				$arrTaken = glob($strQueuePath . $strBaseTime . $strCollisionID . '-*');

				// If unique, break the loop.
				if (empty($arrTaken) && !file_exists($strFullPath))
				{
					break;
				}

				// If file exists, we had a collision. Try the next sequential number.
				$intCollisionCounter++;

				// Safety break: Prevents an infinite loop.
				if ($intCollisionCounter > 9999)
				{
					unlockPublish($strMutexFile);
					@unlink($strFullTempPath);
					@unlink($strLockPath . $strLockFile);
					sendJSONResponse("index.php: Queue exceeded 9,999 attempted messages in one second.", "Queue overload, try again.", "", []);
				}
			}

			$blnRenamed = rename($strFullTempPath, $strFullPath);

			// the name is taken now, so the next publisher can have the mutex
			unlockPublish($strMutexFile);

			if (!$blnRenamed)
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
			if (strlen($strMutexFile) > 0)
			{
				unlockPublish($strMutexFile);
			}
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

function handleStore($strNonce_a, $intRetentionDays_a, $binMessage_a, $blnUnidentified_a = false)
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

		if ($blnUnidentified_a)
		{
			$strExt = pathinfo($strName, PATHINFO_EXTENSION);
			$strBase = substr($strName, 0, strlen($strName) - strlen($strExt) - 1);
			$strName = $strBase . '-u.' . $strExt;
		}

		$strFullPath = $strStorePath . $strName;

		if (!is_dir($strStorePath))
		{
			if (!mkdir($strStorePath, FOLDER_PERMISSIONS, true))
			{
				sendJSONResponse("index.php: Could not create store directory: " . $strStorePath, "Could not create store.", "", []);
			}
		}

		// written to the temp folder first, then moved into place, so the store never
		// holds a half-written file
		$strFullTempPath = getTempRoot() . $strGetGUID . ".bin";
		$intBytesWritten = file_put_contents($strFullTempPath, $binMessage_a);
		if ($intBytesWritten === false || $intBytesWritten !== strlen($binMessage_a))
		{
			@unlink($strFullTempPath);
			sendJSONResponse("index.php: Failed to create message.", "Failed to create message.", "", []);
		}

		if (!moveIntoPlace($strFullTempPath, $strFullPath))
		{
			@unlink($strFullTempPath);
			sendJSONResponse("index.php: Failed to move message into the store.", "Failed to create message.", "", []);
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
$strLength = '';
$strName = '';
$strNames = '';
$strNonce = '';
$strOffset = '';
$strQueueName = '';
$strRetentionDays = '';
$strTransaction = '';
$strUnidentified = '';

if (ALLOW_GET === 'TRUE')
{
	if (isset($_GET['action'])) 	{ $strAction = $_GET['action']; }
	if (isset($_GET['after'])) 		{ $strAfterName = $_GET['after']; }
	if (isset($_GET['length'])) 	{ $strLength = $_GET['length']; };
	if (isset($_GET['name'])) 		{ $strName = $_GET['name']; }
	if (isset($_GET['names']))		{ $strNames = $_GET['names']; }
	if (isset($_GET['msg'])) 		{ $binMessage = $_GET['msg']; }
	if (isset($_GET['n'])) 			{ $strNonce = $_GET['n']; }
	if (isset($_GET['offset'])) 	{ $strOffset = $_GET['offset']; };
	if (isset($_GET['q'])) 			{ $strQueueName = $_GET['q']; }
	if (isset($_GET['r'])) 			{ $strRetentionDays = $_GET['r']; }
	if (isset($_GET['t'])) 			{ $strTransaction = $_GET['t']; }
	if (isset($_GET['u'])) 			{ $strUnidentified = $_GET['u']; }
}

if (empty($binMessage) && isset($_FILES['msg']) && $_FILES['msg']['error'] === UPLOAD_ERR_OK)
{
    $binMessage = file_get_contents($_FILES['msg']['tmp_name']);
}

if (strlen($strAction) === 0)		{ if (isset($_POST['action'])) 	{ $strAction = $_POST['action']; } }

if (empty($strAction))
{
    echo("Welcome to the ZOSCII MQ.");
    die();
}

if (empty($binMessage))				{ if (isset($_POST['msg'])) 	{ $binMessage = $_POST['msg']; } }
if (strlen($strAfterName) === 0)	{ if (isset($_POST['after'])) 	{ $strAfterName = $_POST['after']; } }
if (strlen($strLength) === 0)		{ if (isset($_POST['length']))  { $strLength = $_POST['length']; } }
if (strlen($strName) === 0)			{ if (isset($_POST['name'])) 	{ $strName = $_POST['name']; } }
if (strlen($strNames) === 0)		{ if (isset($_POST['names'])) 	{ $strNames = $_POST['names']; } }
if (strlen($strNonce) === 0)		{ if (isset($_POST['n'])) 		{ $strNonce = $_POST['n']; } }
if (strlen($strOffset) === 0)		{ if (isset($_POST['offset']))  { $strOffset = $_POST['offset']; } }
if (strlen($strQueueName) === 0)	{ if (isset($_POST['q'])) 		{ $strQueueName = $_POST['q']; } }
if (strlen($strRetentionDays) === 0){ if (isset($_POST['r'])) 		{ $strRetentionDays = $_POST['r']; } }
if (strlen($strTransaction) === 0)	{ if (isset($_POST['t'])) 		{ $strTransaction = $_POST['t']; } }
if (strlen($strUnidentified) === 0)	{ if (isset($_POST['u'])) 		{ $strUnidentified = $_POST['u']; } }

$arrNames = [];
if (strlen($strNames) > 0)
{
	$arrNames = json_decode($strNames, true);
}

// on a fetch, r present (any value, even empty) asks for the message BEFORE the
// pointer instead of after it; on a publish, store or commit, r is the retention days
$blnReverse = isset($_POST['r']);
if (ALLOW_GET === 'TRUE')
{
	if (isset($_GET['r'])) { $blnReverse = true; }
}

$strAfterName = basename($strAfterName);
$strName = basename($strName);
$strNonce = preg_replace('/[^a-zA-Z0-9_-]/', '', $strNonce);
$strQueueName = preg_replace('/[^a-zA-Z0-9_-]/', '', $strQueueName);
$strQueueName = strtolower($strQueueName);
$strTransaction = strtolower(trim($strTransaction));
$intRetentionDays = (int)$strRetentionDays;
$intOffset = (int)$strOffset;
$intLength = (int)$strLength;

if (strlen($strNonce) > 0)
{
	handleNonce($strNonce);
}

// router

switch ($strAction)
{
	case 'fetch':
		if (ALLOW_FETCH === 'TRUE')
		{
			handleFetch($strQueueName, $strAfterName, $intOffset, $intLength, $blnReverse);
		}
		break;

	case 'identify':
		if (ALLOW_IDENTIFY === 'TRUE')
		{
			handleIdentify($arrNames);
		}
		break;

	case 'publish':
		if (ALLOW_PUBLISH === 'TRUE')
		{
			if (in_array($strQueueName, $arrPublishBlocks))
			{
				sendJSONResponse("", "Invalid action '" . $strAction . "' for provided queue.", "", []);
			}
			else
			{
				handlePublish($strQueueName, $strNonce, $intRetentionDays, $binMessage);
			}
		}
		break;

	case 'retrieve':
		if (ALLOW_RETRIEVE === 'TRUE')
		{
			handleRetrieve($strName);
		}
		break;

	case 'scan':
		if (ALLOW_SCAN === 'TRUE')
		{
			handleScan();
		}
		break;

	case 'store':
		if (ALLOW_STORE === 'TRUE')
		{
			$blnUnidentified = ($strUnidentified === '1');
		handleStore($strNonce, $intRetentionDays, $binMessage, $blnUnidentified);
		}
		break;

	case 'upload':
		if (ALLOW_TRANSACTIONS === 'TRUE')
		{
			handleUpload($strTransaction, $strNonce, $binMessage);
		}
		break;

	case 'commit':
		if (ALLOW_TRANSACTIONS === 'TRUE')
		{
			handleCommit($strTransaction, $strQueueName, $intRetentionDays, ($strUnidentified === '1'));
		}
		break;

	case 'abort':
		if (ALLOW_TRANSACTIONS === 'TRUE')
		{
			handleAbort($strTransaction);
		}
		break;

	default:
		sendJSONResponse("", "Unknown action '" . $strAction . "'.", "", []);
}

?>