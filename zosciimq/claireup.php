<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Retention Cron Job (claireup.php)
// Deletes messages based on the RRRR (Retention Days) value in the filename.
//  
// Execution: php claireup.php

// --- Global Configuration ---
define('QUEUE_ROOT', './queues/'); 
define('CLI_ONLY', 'TRUE'); 

define('DEBUG', 'FALSE'); 

DEFINE('FILE_ERRORLOG', './claireup.log'); // or '/var/log/claireup.log'
DEFINE('LOG_OUTPUT', 'FALSE');	// TRUE or FALSE

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
			$arrAllFiles = glob($strQueuePath . '/message-*.bin');
			
			if ($arrAllFiles === false || empty($arrAllFiles)) 
			{
				echo("  INFO: Queue is empty.\n");
				continue;
			}

			// 3. Process each file
			foreach ($arrAllFiles as $strFullPath) 
			{
				$strFilename = basename($strFullPath);
				
				// Expected Filename Format: message-YYYYMMDDHHNNSSCCCC-RRRR.bin
				
				// A. Explicitly pull out the timestamp (14 characters starting after "message-")
				// The timestamp starts at index 8 and is 14 digits long.
				$strMessageTimestamp = substr($strFilename, 8, 14);

				// B. Explicitly pull out the retention days (4 characters before ".bin")
				// The retention starts 7 characters from the end (-7) and is 4 digits long.
				$strRetentionString = substr($strFilename, -7, 4);

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
