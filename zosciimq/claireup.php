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

define('CLI_ONLY', 'TRUE'); 
define('DEBUG', 'FALSE'); 

DEFINE('LOG_OUTPUT', 'FALSE');	// TRUE or FALSE
DEFINE('FILE_ERRORLOG', './claireup.log'); // or '/var/log/claireup.log'

DEFINE('NONCE_TIMEFRAME', 5);	// minimum minutes to keep NONCE

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

// --- CLEANUP LOGIC ---
// Renamed the main cleanup function
function handleClaireup() 
{
    $intCurrentTimestamp = time();
    $intDeletedCount = 0;
    
    // Updated the execution message
    echo("--- Starting ZOSCII MQ Claireup at " . date('Y-m-d H:i:s', $intCurrentTimestamp) . " ---\n");

    // 1. Find all queue directories
    // We use GLOB_ONLYDIR to only get folders, not files in the root
    $arrQueuePaths = glob(QUEUE_ROOT . '*', GLOB_ONLYDIR);
    
    if ($arrQueuePaths === false || empty($arrQueuePaths)) 
	{
        echo("INFO: No queues found in " . QUEUE_ROOT . ".\n");
        // Updated the execution message
        echo("--- Claireup complete. Total deleted: " . $intDeletedCount . " ---\n");
    }
	else
	{
		foreach ($arrQueuePaths as $strQueuePath) 
		{
			$strQueueName = basename($strQueuePath);
			echo("Processing queue: " . $strQueueName . "\n");

			// 2. Find all message files in the queue
			$arrAllFiles = glob($strQueuePath . '/*.bin');
			
			if ($arrAllFiles === false || empty($arrAllFiles)) 
			{
				echo("  INFO: Queue is empty.\n");
				continue;
			}

			// 3. Process each file
			foreach ($arrAllFiles as $strFullPath) 
			{
				$strFilename = basename($strFullPath);
				
				// Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
				
				// Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin

				$strFilename = basename($strFullPath); // Always use basename for safety

				// 1. Split the filename into parts based on the hyphen delimiter
				// Result: [ "YYYYMMDDHHNNSSCCCC", "RRRR", "GUID.bin" ]
				$arrParts = explode('-', $strFilename);

				// 2. Assign the parts based on their index
				$strMessageTimestamp = $arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
				$strRetentionString  = $arrParts[1]; // RRRR (4 characters)

				// C. Convert to integers
				$intRetentionDays = (int)$strRetentionString;
				
				// Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
				// PHP's strtotime is forgiving, but we use the fixed YYYYMMDDHHNNSS part.
				$intMessageTime = strtotime($strMessageTimestamp);
				
				// Safety check for invalid timestamp (e.g., corrupted filename)
				if ($intMessageTime === false) 
				{
					 // WARNINGs are left outside the DEBUG check as they indicate an operational problem.
					 echo("  WARNING: Skipping file with invalid timestamp/format: " . $strFilename . " (Retention: " . $strRetentionString . ", Timestamp: " . $strMessageTimestamp . ")\n");
					 continue;
				}

				// Calculate the expiration time (Message Time + Retention Days in seconds)
				// 86400 seconds = 1 day
				$intExpirationTime = $intMessageTime + ($intRetentionDays * 86400);

					// The main file processing information is now conditional
				logDebug("DEBUG: " . $strFilename . " | Retention: " . $intRetentionDays . " days | Expires: " . date('Y-m-d H:i:s', $intExpirationTime));
				// -------------------------------


				// 5. Check for expiration and delete
				// If the message is older than the calculated expiration time, delete it.
				// RetentionDays=0 now means "expire immediately if timestamp is in the past."
				if ($intCurrentTimestamp >= $intExpirationTime) 
				{
					// Time to delete!
					if (unlink($strFullPath)) 
					{
						$intDeletedCount++;
						echo("  DELETED: " . $strFilename . " (Expired: " . date('Y-m-d H:i:s', $intExpirationTime) . ")\n");
					} 
					else 
					{
						// ERRORs are also left outside the DEBUG check as they indicate an operational problem.
						logError("ERROR: Could not delete file: " . $strFilename . " (Permission denied?)");
					}
				}
			}
		}

		if (NONCE_TIMEFRAME > 0)
		{
			// --- NONCE Cleanup Logic ---
			echo("Processing NONCE marker files...\n");

			// Calculate the cutoff time for deletion: Current Time - (NONCE_TIMEFRAME minutes * 60 seconds)
			// NONCE_TIMEFRAME is defined as 5 minutes in claireup.php
			$intCutoffTime = $intCurrentTimestamp - (NONCE_TIMEFRAME * 60);

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
							echo("  DELETED NONCE: " . basename($strFullPath) . " (Aged out: " . date('Y-m-d H:i:s', $intFileModTime) . ")\n");
						}
						else
						{
							// Log permission errors if unlink fails
							logError("ERROR: Could not delete NONCE file: " . basename($strFullPath) . " (Permission denied?)");
						}
					}
				}
			}
			// End NONCE Cleanup
		}
		
		// Updated the execution message
		echo("--- Claireup complete. Total deleted: " . $intDeletedCount . " ---\n");
	}
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

// --- Main Execution Block ---

// Renamed the main function call
handleClaireup();

?>
