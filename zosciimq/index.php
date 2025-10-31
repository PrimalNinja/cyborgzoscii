<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (index.php)
// Deletes messages based on the RRRR (Retention Days) value in the filename.
// 
// Execution: php index.php

// --- Global Configuration ---
// 'g_' prefix for global constant (used as a global configuration value)
define('NONCE_ROOT', './nonce/');
define('QUEUE_ROOT', './queues/');

define('ALLOW_GET', 'TRUE');

DEFINE('LOG_OUTPUT', 'FALSE'); // TRUE or FALSE
DEFINE('FILE_ERRORLOG', './zosciimq.log'); // or '/var/log/zosciimq.log'

// --- Helper Functions ---
function logError($str_a) 
{
    if (LOG_OUTPUT == 'TRUE') 
	{
        file_put_contents(FILE_ERRORLOG, date('Y-m-d H:i:s') . " - " . $str_a . "\n", FILE_APPEND);
    }
}

function handlePublish($strQueueName_a, $strNonce_a, $intRetentionDays_a, $binMessage_a)
{
	$blnResult = true;
	
	$strQueueName = $strQueueName_a;
	$strNonce = $strNonce_a;
	$intRetentionDays = $intRetentionDays_a;
	$binMessage = $binMessage_a;
	
    // Format RRRR, e.g., 3 becomes 0003
    $strRetentionDays = sprintf('%04d', $intRetentionDays);

    if (empty($strQueueName) || empty($binMessage)) 
	{
		logError("Error: Missing 'q' (queue name) or 'msg' (message content).");
        $strResult = "Error: Missing 'q' (queue name) or 'msg' (message content).";
    } 
	else 
	{
        $strQueuePath = QUEUE_ROOT . $strQueueName . '/';
		
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
			// Generate Filename (message-YYYYMMDDHHNNSSCCCC-RRRR.bin)
			$strBaseTime = date('YmdHis');
			$intCollisionCounter = 0;
			$strFilename = '';
			$strFullPath = '';
			
			while ($intCollisionCounter < 9999) 
			{
				$intCollisionCounter++;

				// Collision Counter (CCCC format)
				$strCollisionCounter = sprintf('%04d', $intCollisionCounter);
				$strFilename = "message-" . $strBaseTime . $strCollisionCounter . "-" . $strRetentionDays . ".bin";
				$strFullPath = $strQueuePath . $strFilename;
				if (!file_exists($strFullPath)) 
				{
					break;
				}
			}
			
			if ($intCollisionCounter > 9999) 
			{
				logError("Error: Exceeded collision attempts. Try again in the next second.");
				$strResult = "Error: Exceeded collision attempts. Try again in the next second.";
			} 
			else 
			{
				// Write the Message
				$intBytesWritten = file_put_contents($strFullPath, $binMessage);
				if ($intBytesWritten !== false) 
				{
					$strResult = "Success: Message published to '" . $strQueueName . "'\n";
					$strResult .= "Filename: " . $strFilename . "\n";
					$strResult .= "Bytes Written: " . $intBytesWritten;
				} 
				else 
				{
					logError("Error: Failed to write message file.");
					$strResult = "Error: Failed to write message file.";
					$blnResult = false;
				}
				
				if ($blnResult && strlen($strNonce) > 0)
				{
					$strNonceFile = NONCE_ROOT . $strNonce;
					if (touch($strNonceFile))
					{
						// nonce file is created
					} 
					else 
					{
						logError("Failed to touch file: " . $strNonceFile);
						$blnResult = false;
					}
				}
			}
		}
    }

    return $strResult;
}

function handleFetch($strQueueName_a, $strAfterFilename_a)
{
	$strQueueName = $strQueueName_a;
	$strAfterFilename = $strAfterFilename_a;
	
	$strQueuePath = QUEUE_ROOT . $strQueueName . '/';
	if (!is_dir($strQueuePath)) 
	{
		logError("Error: Queue '" . $strQueueName . "' does not exist.");
		$strResult = "Error: Queue '" . $strQueueName . "' does not exist.";
	} 
	else 
	{
		// Get all message files, sorted chronologically by name
		$arrAllFiles = glob($strQueuePath . 'message-*.bin');
		if ($arrAllFiles === false || empty($arrAllFiles)) 
		{
			$strResult = "Status: Queue is empty.";
		} 
		else 
		{
			// Sort files alphabetically (chronologically by filename)
			sort($arrAllFiles);
			$strNextMessagePath = null;

			$blnFoundAfter = false;
			// Check if client is requesting from the start (empty 'after')
			if (empty($strAfterFilename)) 
			{
				$blnFoundAfter = true;
			}

			// Find the next message after the 'after' pointer
			// Loop variable $strFullPath is correctly typed (string full path)
			foreach ($arrAllFiles as $strFullPath) 
			{
				$strFilename = basename($strFullPath);
				if ($blnFoundAfter) 
				{
					// This is the first file after the 'after' pointer
					$strNextMessagePath = $strFullPath;
					break;
				}
				if ($strFilename === $strAfterFilename) 
				{
					// Found the pointer; the next iteration's file will be the message to return.
					$blnFoundAfter = true;
				}
			}

			if ($strNextMessagePath) 
			{
				$strFilename = basename($strNextMessagePath);
				// Output headers to treat the response as a raw file download
				header('Content-Type: application/octet-stream');
				header('Content-Disposition: attachment; filename="' . $strFilename . '"');
				// Output the raw binary content
				readfile($strNextMessagePath);
				exit;
			} 
			else 
			{
				$strResult = "Status: No new messages found after '" . $strAfterFilename . "' (or queue is empty).";
			}
        }
    }
	
    logError($strResult);
    
	return $strResult;
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

$strAction = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['action'])) 
	{
		$strAction = $_GET['action'];
	}
}

if (strlen($strAction) == 0)
{
	if (isset($_POST['action'])) 
	{
		$strAction = $_POST['action'];
	}
}

if (empty($strAction)) 
{
    echo("Welcome to the ZOSCII MQ.");
    exit;
}

$strQueueName = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['q'])) 
	{
		// Strip non-alphanumeric characters for safety
		$strQueueName = $_GET['q'];
		$strQueueName = preg_replace('/[^a-zA-Z0-9_-]/', '', $strQueueName);
	}
}

if (strlen($strQueueName) == 0)
{
	if (isset($_POST['q'])) 
	{
		$strQueueName = $_POST['q'];
		$strQueueName = preg_replace('/[^a-zA-Z0-9_-]/', '', $strQueueName);
	}
}

$strNonce = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['n'])) 
	{
		// Strip non-alphanumeric characters for safety
		$strNonce = $_GET['n'];
		$strNonce = preg_replace('/[^a-zA-Z0-9_-]/', '', $strNonce);
	}
}

if (strlen($strNonce) == 0)
{
	if (isset($_POST['n'])) 
	{
		$strNonce = $_POST['n'];
		$strNonce = preg_replace('/[^a-zA-Z0-9_-]/', '', $strNonce);
	}
}

$strRetentionDays = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['r'])) 
	{
		$strRetentionDays = $_GET['r'];
	}
}
	
if (strlen($strRetentionDays) == 0)
{
	if (isset($_POST['r'])) 
	{
		$strRetentionDays = $_POST['r'];
	}
}
$intRetentionDays = (int)$strRetentionDays;

$binMessage = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['msg'])) 
	{
		$binMessage = $_GET['msg'];
	}
}

if (empty($binMessage))
{
	if (isset($_POST['msg'])) 
	{
		$binMessage = $_POST['msg'];
	}
}

$strAfterFilename = '';
if (ALLOW_GET == 'TRUE')
{
	if (isset($_GET['after'])) 
	{
		// Use basename for safety, filtering out directory paths
		$strAfterFilename = basename($_GET['after']);
	}
}

if (strlen($strAfterFilename) == 0)
{
	if (isset($_POST['after'])) 
	{
		$strAfterFilename = $_POST['after'];
	}
}

if (empty($strQueueName)) 
{
	logError("Error: Missing 'q' (queue name).");
	exit;
} 

if (!is_dir(NONCE_ROOT)) 
{
	if (!mkdir(NONCE_ROOT, 0777, true)) 
	{
		$blnResult = false;
		logError("Error: Could not create nonce directory: " . NONCE_ROOT);
		echo("Error: Could not create nonce directory: " . NONCE_ROOT);
		exit;
	} 
}

if (strlen($strNonce) > 0)
{
	$strNonceFile = NONCE_ROOT . $strNonce;
	if (file_exists($strNonceFile)) 
	{
		echo("Success: Message already published.");
		exit;
	}
}
		
// --- Action Router ---
switch ($strAction) 
{
    case 'publish':
        $strResult = handlePublish($strQueueName, $strNonce, $intRetentionDays, $binMessage);
        echo($strResult);
        break;
		
    case 'fetch':
        $strResult = handleFetch($strQueueName, $strAfterFilename);
        if ($strResult === false) 
		{
            // handleFetch($strQueueName, $strAfterFilename) has already output the file, so do not output anything here
        } 
		else 
		{
            echo($strResult);
        }
        break;
		
    default:
        logError("Error: Unknown action '" . $strAction . "'");
}

?>