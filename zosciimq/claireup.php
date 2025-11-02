<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Retention Cron Job (claireup.php)
// Deletes messages based on the RRRR (Retention Days) value in the filename.
//  
// Execution: php claireup.php

// --- Global Configuration ---
define('NONCE_ROOT', './nonce/');
define('QUEUE_ROOT', './queues/'); 
define('STORE_ROOT', './store/'); 

define('CLI_ONLY', 'FALSE'); 
define('DEBUG', 'FALSE'); 

define('LOG_OUTPUT', 'FALSE');	// TRUE or FALSE
define('FILE_ERRORLOG', './claireup.log'); // or '/var/log/claireup.log'
define('FOLDER_PERMISSIONS', 0755);

define('NONCE_TIMEFRAME', 5);	// minimum minutes to keep NONCE

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
			logError("Fatal Could not create root queue directory: " . QUEUE_ROOT);
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

function cleanUpNonces($intCurrentTimestamp_a)
{
    $intDeletedCount = 0;

	if (NONCE_TIMEFRAME > 0)
	{
		// --- NONCE Cleanup Logic ---
		echo("Processing NONCE marker files...\n");

		// Calculate the cutoff time for deletion: Current Time - (NONCE_TIMEFRAME minutes * 60 seconds)
		// NONCE_TIMEFRAME is defined as 5 minutes in claireup.php
		$intCutoffTime = $intCurrentTimestamp_a - (NONCE_TIMEFRAME * 60);

		// Find all files in the nonce directory
		$arrNonceFiles = glob(NONCE_ROOT . '*');

		if ($arrNonceFiles === false || empty($arrNonceFiles))
		{
			echo("INFO: No NONCE files found.\n");
		}
		else
		{
			foreach ($arrNonceFiles as $strFullPath)
			{
				// Get the last modification time of the file (when it was created by index.php)
				$intFileModTime = filemtime($strFullPath);
				
				// If the modification time is older than the calculated cutoff time, delete it.
				if ($intFileModTime !== false && $intFileModTime < $intCutoffTime)
				{
					if (unlink($strFullPath))
					{
						$intDeletedCount++;
						echo("DELETED NONCE: " . basename($strFullPath) . " (Aged out: " . date('Y-m-d H:i:s', $intFileModTime) . ")\n");
					}
					else
					{
						// Log permission errors if unlink fails
						logError("Could not delete NONCE file: " . basename($strFullPath) . " (Permission denied?)");
					}
				}
			}
		}
		// End NONCE Cleanup
	}
	
	return $intDeletedCount;
}

function cleanUpQueues($intCurrentTimestamp_a)
{
    $intDeletedCount = 0;

    // Find all queue directories
    // We use GLOB_ONLYDIR to only get folders, not files in the root
    $arrQueuePaths = glob(QUEUE_ROOT . '*', GLOB_ONLYDIR);
    
	foreach ($arrQueuePaths as $strQueuePath) 
	{
		$strQueueName = basename($strQueuePath);
		echo("Processing queue: " . $strQueueName . "\n");

		// Find all message files in the queue
		$arrAllFiles = glob($strQueuePath . '/*.bin');
		
		if ($arrAllFiles === false || empty($arrAllFiles)) 
		{
			echo("INFO: Queue is empty.\n");
			continue;
		}

		// Process each file
		foreach ($arrAllFiles as $strFullPath) 
		{
			$strFilename = basename($strFullPath);
			
			// Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin

			$strFilename = basename($strFullPath); // Always use basename for safety

			// Split the filename into parts based on the hyphen delimiter
			// Result: [ "YYYYMMDDHHNNSSCCCC", "RRRR", "GUID.bin" ]
			$arrParts = explode('-', $strFilename);
            
            // --- PARTS CHECK ---
            // Ensure we have at least 3 parts for a valid filename structure (as requested)
            if (count($arrParts) < 3) 
			{
                 echo("WARNING: Skipping non-standard file: " . $strFilename . ".\n");
                 continue;
            }

			// Assign the parts based on their index
			$strMessageTimestamp = $arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
			$strRetentionString  = $arrParts[1]; // RRRR (4 characters)

			// Convert to integers
			$intRetentionDays = (int)$strRetentionString;
			
			// Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
			// *** FIX: Explicitly use the first 14 characters to guarantee a clean time for strtotime() ***
			$intMessageTime = strtotime(substr($strMessageTimestamp, 0, 14));
			
			// Safety check for invalid timestamp (e.g., corrupted filename)
			if ($intMessageTime === false) 
			{
				 // WARNINGs are left outside the DEBUG check as they indicate an operational problem.
				 echo("WARNING: Skipping file with invalid timestamp/format: " . $strFilename . " (Retention: " . $strRetentionString . ")\n");
				 continue;
			}

			// Calculate the expiration time (Message Time + Retention Days in seconds)
			// 86400 seconds = 1 day
			$intExpirationTime = $intMessageTime + ($intRetentionDays * 86400);

				// The main file processing information is now conditional
			logDebug("DEBUG: " . $strFilename . " | Retention: " . $intRetentionDays . " days | Expires: " . date('Y-m-d H:i:s', $intExpirationTime));
			// -------------------------------


			// Check for expiration and delete
			// If the message is older than the calculated expiration time, delete it.
			// RetentionDays=0 now means "expire immediately if timestamp is in the past."
			if ($intCurrentTimestamp_a >= $intExpirationTime) 
			{
				// Time to delete!
				if (unlink($strFullPath)) 
				{
					$intDeletedCount++;
					echo("DELETED: " . $strFilename . " (Expired: " . date('Y-m-d H:i:s', $intExpirationTime) . ")\n");
				} 
				else 
				{
					// ERRORs are also left outside the DEBUG check as they indicate an operational problem.
					logError("Could not delete file: " . $strFilename . " (Permission denied?)");
				}
			}
		}
	}
	
	return $intDeletedCount;
}

// --- New Helper Function for Recursive Cleanup ---
function cleanUpRecursive($strPath_a, $intCurrentTimestamp_a)
{
    $intDeletedCount = 0;

    // Use RecursiveDirectoryIterator for deep traversal (PHP 5.0+)
    try {
        $objIterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($strPath_a, RecursiveDirectoryIterator::SKIP_DOTS), RecursiveIteratorIterator::SELF_FIRST);

        foreach ($objIterator as $strFullPath => $objItemInfo) 
		{
            // Only process files for deletion
            if ($objItemInfo->isFile()) 
			{
                $strFilename = $objItemInfo->getFilename();
                
                // 1. Split the filename into parts based on the hyphen delimiter
                // Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
                $arrParts = explode('-', $strFilename);
                
                // Ensure we have at least 3 parts for a valid filename structure
                if (count($arrParts) < 3) 
				{
                     echo("WARNING: Skipping non-standard file: " . $strFilename . "\n");
                     continue;
                }

                $strMessageTimestamp = $arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
                $strRetentionString  = $arrParts[1]; // RRRR (4 characters)

                // C. Convert to integers
                $intRetentionDays = (int)$strRetentionString;
                
                // Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
                $intMessageTime = strtotime(substr($strMessageTimestamp, 0, 14)); // Only use YYYYMMDDHHNNSS
                
                // Safety check for invalid timestamp (e.g., corrupted filename)
                if ($intMessageTime === false) 
                {
                     echo("WARNING: Skipping file with invalid timestamp/format: " . $strFilename . " (Retention: " . $strRetentionString . ", Timestamp: " . $strMessageTimestamp . ")\n");
                     continue;
                }

                // Calculate the expiration time (Message Time + Retention Days in seconds)
                // 86400 seconds = 1 day
                $intExpirationTime = $intMessageTime + ($intRetentionDays * 86400);

                logDebug("DEBUG: " . $strFilename . " | Retention: " . $intRetentionDays . " days | Expires: " . date('Y-m-d H:i:s', $intExpirationTime));
                
                // 5. Check for expiration and delete
                if ($intCurrentTimestamp_a >= $intExpirationTime) 
                {
                    // Time to delete!
                    if (unlink($strFullPath)) 
                    {
                        $intDeletedCount++;
                        // Use relative path for cleaner output in recursive mode
                        $strRelativePath = str_replace(STORE_ROOT, '', $strFullPath);
                        echo("DELETED: " . $strRelativePath . " (Expired: " . date('Y-m-d H:i:s', $intExpirationTime) . ")\n");
                    } 
                    else 
                    {
                        // Log permission errors if unlink fails, using the script's logError function
                        logError("Could not delete file: " . $strFullPath . " (Permission denied?)");
                    }
                }
            }
        }
    } catch (Exception $objError) 
	{
        // Non-fatal error during traversal (e.g., permissions issue on a subdirectory).
        // Skip this path and continue processing other stores.
        logError("Failed to traverse directory " . $strPath_a . " recursively: " . $objError->getMessage());
        return 0;
    }
    
    return $intDeletedCount;
}

// --- Updated cleanUpStore function ---
function cleanUpStore($intCurrentTimestamp_a)
{
    $intDeletedCount = 0;

    // Find all top-level store directories
    $arrStorePaths = glob(STORE_ROOT . '*', GLOB_ONLYDIR);
    
	foreach ($arrStorePaths as $strStorePath) 
	{
		$strStoreName = basename($strStorePath);
		echo("Processing store (recursively): " . $strStoreName . "\n");

		// Use the recursive helper function to clean up the store and its subdirectories
        $intDeletedCount += cleanUpRecursive($strStorePath, $intCurrentTimestamp_a);
	}
	
	return $intDeletedCount;
}

function handleClaireup() 
{
    $intCurrentTimestamp = time();
    $intDeletedCount = 0;
    
    echo("--- Starting ZOSCII MQ Claireup at " . date('Y-m-d H:i:s', $intCurrentTimestamp) . " ---\n");

	$intDeletedCount += cleanUpNonces($intCurrentTimestamp);
	$intDeletedCount += cleanUpQueues($intCurrentTimestamp);
	$intDeletedCount += cleanUpStore($intCurrentTimestamp);
		
	echo("--- Claireup complete. Total deleted: " . $intDeletedCount . " ---\n");
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

initFolders();

// Renamed the main function call
handleClaireup();

?>
