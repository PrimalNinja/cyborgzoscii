<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Statistics Assessor (statissa.php)
// 
// Scans all queues in QUEUE_ROOT to calculate storage used, file counts,
// and identifies the oldest/newest message in each queue.
// 
// Outputs a single HTML report with basic visualization.
// 
// Execution: Open in a web browser (e.g., http://your.host/statissa.php)

define('PAGE_TITLE', 'ZOSCII MQ Storage Statistics (Statissa)');

define('QUEUE_ROOT', './queues/');
define('STORE_ROOT', './store/');

define('TEMP_QUEUE', 'temp/');

define('LOG_OUTPUT', 'TRUE');	// TRUE or FALSE
define('FILE_ERRORLOG', './statissa.log'); // or '/var/log/statissa.log'

function logError($str_a)
{
	if (LOG_OUTPUT === 'TRUE')
	{
		file_put_contents(FILE_ERRORLOG, date('Y-m-d H:i:s') . " - " . $str_a . "\n", FILE_APPEND);
	}
}

function checkFolders()
{
	// Ensure the root queue directory exists
	if (!is_dir(QUEUE_ROOT)) 
	{
		logError("Fatal Error: Could not find root queue directory " . QUEUE_ROOT);
		echo("Fatal Error: Could not find root queue directory " . QUEUE_ROOT);
		die();
	}

	if (!is_dir(QUEUE_ROOT . TEMP_QUEUE)) 
	{
		logError("Fatal Error: Could not find temp queue directory " . QUEUE_ROOT . TEMP_QUEUE);
		echo("Fatal Error: Could not find temp queue directory " . QUEUE_ROOT . TEMP_QUEUE);
		die();
	}

	// Ensure the root store directory exists
	if (!is_dir(STORE_ROOT)) 
	{
		logError("Fatal Error: Could not find root store directory " . STORE_ROOT);
		echo("Fatal Error: Could not find root store directory " . STORE_ROOT);
		die();
	}
}

function getQueueStats() 
{
    $arrQueueStats = [];
    $intTotalStorage = 0;
    if (!is_dir(QUEUE_ROOT)) 
	{
        return [[], 0];
    }
    
	// Find all queue directories
    $arrQueuePaths = glob(QUEUE_ROOT . '*', GLOB_ONLYDIR);
    if ($arrQueuePaths === false || empty($arrQueuePaths)) 
	{
        return [[], 0];
    }
    
	foreach ($arrQueuePaths as $strQueuePath) 
	{
        $strQueueName = basename($strQueuePath);
        $intQueueSize = 0;
        $intFileCount = 0;
        $strOldestFile = 'N/A';
        $strNewestFile = 'N/A';
        
		// Get all message files and sort them (alphabetical = chronological)
        $arrAllFiles = glob($strQueuePath . '/*.bin');
        if ($arrAllFiles === false) 
		{
            // Handle error during glob
            continue;
        }
        
		// Sort files to easily find oldest (first) and newest (last)
        sort($arrAllFiles);
        $intFileCount = count($arrAllFiles);
        if ($intFileCount > 0) 
		{
            $strOldestFile = basename($arrAllFiles[0]);
            $strNewestFile = basename($arrAllFiles[$intFileCount - 1]);
            foreach ($arrAllFiles as $strFullPath) 
			{
                // Safely add file size to queue total
                $intQueueSize += filesize($strFullPath);
            }
        }
		
        $strOldestDisplay = 'N/A';
        $strNewestDisplay = 'N/A';
        if ($intFileCount > 0) 
		{
            $strOldestDisplay = substr($strOldestFile, 8, 8) . '...';
            $strNewestDisplay = substr($strNewestFile, 8, 8) . '...';
        }
		
        $arrQueueStats[$strQueueName] = [
            'count' => $intFileCount,
            'size_bytes' => $intQueueSize,
            'oldest' => $strOldestFile,
            'oldest_display' => $strOldestDisplay,
            'newest' => $strNewestFile,
            'newest_display' => $strNewestDisplay
        ];
		
        $intTotalStorage += $intQueueSize;
    }
	
    return [$arrQueueStats, $intTotalStorage];
}

function getDirectorySizeRecursive($strPath_a) 
{
    $intTotalSize = 0;
    $intFileCount = 0;
    // We use actual file modification time (mtime) to find oldest/newest recursively
    $arrOldestFile = null; // [timestamp, filename]
    $arrNewestFile = null; // [timestamp, filename]

    // Use RecursiveDirectoryIterator for deep traversal (PHP 5.0+)
    try 
	{
        $objIterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($strPath_a, RecursiveDirectoryIterator::SKIP_DOTS), RecursiveIteratorIterator::SELF_FIRST);

        foreach ($objIterator as $objItemInfo) 
		{
            // Only process files for size and time
            if ($objItemInfo->isFile()) 
			{
                $intTotalSize += $objItemInfo->getSize();
                $intFileCount++;
                
                $intMTime = $objItemInfo->getMTime();
                $strFileName = $objItemInfo->getFilename();

                // Track oldest file (smallest modification time)
                if ($arrOldestFile === null || $intMTime < $arrOldestFile[0]) 
				{
                    $arrOldestFile = [$intMTime, $strFileName]; 
                }

                // Track newest file (largest modification time)
                if ($arrNewestFile === null || $intMTime > $arrNewestFile[0]) 
				{
                    $arrNewestFile = [$intMTime, $strFileName];
                }
            }
        }
    } 
	catch (Exception $objError) 
	{
        // Log the error if traversal fails
		logError("Error traversing directory " . $strPath_a . ": " . $objError->getMessage());
        return [0, 0, 'N/A', 'N/A'];
    }

    $strOldestFileName = $arrOldestFile ? $arrOldestFile[1] : 'N/A';
    $strNewestFileName = $arrNewestFile ? $arrNewestFile[1] : 'N/A';
    
    return [$intTotalSize, $intFileCount, $strOldestFileName, $strNewestFileName];
}

function getStoreStats() 
{
    $arrStoreStats = [];
    $intTotalStorage = 0;
    if (!is_dir(STORE_ROOT)) 
	{
        return [[], 0];
    }
    
	// Find all top-level store directories
    $arrStorePaths = glob(STORE_ROOT . '*', GLOB_ONLYDIR);
    if ($arrStorePaths === false || empty($arrStorePaths)) 
	{
        return [[], 0];
    }
    
	foreach ($arrStorePaths as $strStorePath) 
	{
        $strStoreName = basename($strStorePath);
        
        // Use the recursive helper function
        list($intStoreSize, $intFileCount, $strOldestFile, $strNewestFile) = getDirectorySizeRecursive($strStorePath);
        
        $strOldestDisplay = 'N/A';
        $strNewestDisplay = 'N/A';
        if ($intFileCount > 0) 
		{
            // Apply original filename display logic (assuming file names are long enough)
            $strOldestDisplay = (strlen($strOldestFile) > 16) ? substr($strOldestFile, 8, 8) . '...' : $strOldestFile;
            $strNewestDisplay = (strlen($strNewestFile) > 16) ? substr($strNewestFile, 8, 8) . '...' : $strNewestFile;
        }
		
        $arrStoreStats[$strStoreName] = [
            'count' => $intFileCount,
            'size_bytes' => $intStoreSize,
            'oldest' => $strOldestFile,
            'oldest_display' => $strOldestDisplay,
            'newest' => $strNewestFile,
            'newest_display' => $strNewestDisplay
        ];
		
        $intTotalStorage += $intStoreSize;
    }
	
    return [$arrStoreStats, $intTotalStorage];
}

function outputHtmlReport($arrQueueStats_a, $intQueueTotalStorage_a, $arrStoreStats_a, $intStoreTotalStorage_a) 
{
    $strReportTime = date('Y-m-d H:i:s T');

    echo("<!DOCTYPE html>\n");
    echo("<html lang=\"en\">\n");
    echo("<head>\n");
    echo(" <meta charset=\"UTF-8\">\n");
    echo(" <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    echo(" <title>" . PAGE_TITLE . "</title>\n");
    echo(" <style>\n");
    echo(" /* Base styles */\n");
    echo(" body { font-family: Arial, sans-serif; background-color: #f8fafc; margin: 0 5% 5% 5%; }\n");
    echo(" table { border-collapse: collapse; width: 100%; }\n");
    echo(" th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n");
    echo(" th { background-color: #f0f0f0; }\n");
    echo(" </style>\n");
    echo("</head>\n");
    echo("<body>\n\n");
    echo("<h1>" . PAGE_TITLE . "</h1>\n");
    echo("<p>Generated at: " . $strReportTime . "</p>\n");

	// queues
    $strFormattedQueueTotalStorage = formatBytes($intQueueTotalStorage_a);
    echo("<h2>Queue Overall Summary</h2>\n");
    echo("<p>Total Overall Storage Used: " . $intQueueTotalStorage_a . " Bytes (" . $strFormattedQueueTotalStorage . ")</p>\n");
    echo("<h2>Queue Breakdown</h2>\n");

    if (empty($arrQueueStats_a)) 
	{
        echo("<p>No Queues Found</p>\n");
        echo("<p>The queue root directory (QUEUE_ROOT) is empty or does not contain any subdirectories.</p>\n");
    } 
	else 
	{
        echo("<table>\n");
        echo(" <tr>\n");
        echo(" <th>Queue Name</th>\n");
        echo(" <th>Messages</th>\n");
        echo(" <th>Size</th>\n");
        echo(" <th>Oldest Message (File)</th>\n");
        echo(" <th>Newest Message (File)</th>\n");
		echo(" <th>Storage % Share</th>\n");
        echo(" </tr>\n");

		foreach ($arrQueueStats_a as $strQueueName => $arrData) 
		{
			// Safe division check for storage share
			$fltShare = 0;
			if ($intQueueTotalStorage_a > 0) 
			{
				$fltShare = ($arrData['size_bytes'] / $intQueueTotalStorage_a) * 100;
			}
			
			echo(" <tr>\n");
			echo(" <td>" . $strQueueName . "</td>\n");
			echo(" <td>" . $arrData['count'] . "</td>\n");
			echo(" <td>" . formatBytes($arrData['size_bytes']) . "</td>\n");
			echo(" <td>" . $arrData['oldest_display'] . "</td>\n");
			echo(" <td>" . $arrData['newest_display'] . "</td>\n");
			echo(" <td>\n");
			echo(" <table style='width: 100px; border: none;'><tr>\n");
			if ($fltShare > 0)
			{
				echo(" <td style='width: " . $fltShare . "%; background-color: #3b82f6;'></td>\n");
			}
			if ($fltShare < 100)
			{
				echo(" <td style='width: " . (100 - $fltShare) . "%;'></td>\n");
			}
			echo(" </tr></table>\n");
			echo(" " . number_format($fltShare, 2) . "%\n");
			echo(" </td>\n");
			echo(" </tr>\n");
		}
        echo("</table>\n");
    }
	
	// store
    $strFormattedStoreTotalStorage = formatBytes($intStoreTotalStorage_a);
    echo("<h2>Store Overall Summary</h2>\n");
    echo("<p>Total Overall Storage Used: " . $intStoreTotalStorage_a . " Bytes (" . $strFormattedStoreTotalStorage . ")</p>\n");
    echo("<h2>Store Breakdown</h2>\n");

    if (empty($arrStoreStats_a)) 
	{
        echo("<p>No Stores Found</p>\n");
        echo("<p>The store root directory (STORE_ROOT) is empty or does not contain any subdirectories.</p>\n");
    } 
	else 
	{
        echo("<table>\n");
        echo(" <tr>\n");
        echo(" <th>Store Name</th>\n");
        echo(" <th>Messages</th>\n");
        echo(" <th>Size</th>\n");
        echo(" <th>Oldest Message (File)</th>\n");
        echo(" <th>Newest Message (File)</th>\n");
		echo(" <th>Storage % Share</th>\n");
        echo(" </tr>\n");

		foreach ($arrStoreStats_a as $strStoreName => $arrData) 
		{
			// Safe division check for storage share
			$fltShare = 0;
			if ($intStoreTotalStorage_a > 0) 
			{
				$fltShare = ($arrData['size_bytes'] / $intStoreTotalStorage_a) * 100;
			}
			
			echo(" <tr>\n");
			echo(" <td>" . $strStoreName . "</td>\n");
			echo(" <td>" . $arrData['count'] . "</td>\n");
			echo(" <td>" . formatBytes($arrData['size_bytes']) . "</td>\n");
			echo(" <td>" . $arrData['oldest_display'] . "</td>\n");
			echo(" <td>" . $arrData['newest_display'] . "</td>\n");
			echo(" <td>\n");
			echo(" <table style='width: 100px; border: none;'><tr>\n");
			if ($fltShare > 0)
			{
				echo(" <td style='width: " . $fltShare . "%; background-color: #3b82f6;'></td>\n");
			}
			if ($fltShare < 100)
			{
				echo(" <td style='width: " . (100 - $fltShare) . "%;'></td>\n");
			}
			echo(" </tr></table>\n");
			echo(" " . number_format($fltShare, 2) . "%\n");
			echo(" </td>\n");
			echo(" </tr>\n");
		}
        echo("</table>\n");
    }

    echo("</body>\n");
    echo("</html>\n");
}

function formatBytes($intBytes_a, $intPrecision_a = 2) 
{
    $strResult = '0 Bytes';

    if ($intBytes_a > 0) 
	{
        $arrUnits = array('Bytes', 'KB', 'MB', 'GB', 'TB');
        $intBytes = max($intBytes_a, 0);
        $logValue = log($intBytes);
        
		$intPow = 0;
        if ($logValue > 0) 
		{
            $intPow = floor($logValue / log(1024));
        }
        $intPow = min($intPow, count($arrUnits) - 1);
		
        $intBytes /= pow(1024, $intPow);
        $formattedBytes = round($intBytes, $intPrecision_a);
        $strResult = $formattedBytes . ' ' . $arrUnits[$intPow];
    }

    return $strResult;
}

// entry

checkFolders();

list($arrQueueStats, $intQueueTotalStorage) = getQueueStats();
list($arrStoreStats, $intStoreTotalStorage) = getStoreStats();
outputHtmlReport($arrQueueStats, $intQueueTotalStorage, $arrStoreStats, $intStoreTotalStorage);

?>