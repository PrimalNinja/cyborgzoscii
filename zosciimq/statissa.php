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

// --- Global Configuration ---
// Ensure this path matches the root directory used by index.php and claireup.php
define('QUEUE_ROOT', './queues/');

define('PAGE_TITLE', 'ZOSCII MQ Storage Statistics (Statissa)');

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

function outputHtmlReport($arrQueueStats, $intTotalStorage) 
{
    $strReportTime = date('Y-m-d H:i:s T');
    $strFormattedTotalStorage = formatBytes($intTotalStorage);

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
    echo("<h2>Overall Summary</h2>\n");
    echo("<p>Total Overall Storage Used: " . $intTotalStorage . " Bytes (" . $strFormattedTotalStorage . ")</p>\n");
    echo("<h2>Queue Breakdown</h2>\n");

    if (empty($arrQueueStats)) 
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

		foreach ($arrQueueStats as $strQueueName => $arrData) 
		{
			// Safe division check for storage share
			$fltShare = 0;
			if ($intTotalStorage > 0) 
			{
				$fltShare = ($arrData['size_bytes'] / $intTotalStorage) * 100;
			}
			
			echo(" <tr>\n");
			echo(" <td>" . $strQueueName . "</td>\n");
			echo(" <td>" . $arrData['count'] . "</td>\n");
			echo(" <td>" . formatBytes($arrData['size_bytes']) . "</td>\n");
			echo(" <td>" . $arrData['oldest_display'] . "</td>\n");
			echo(" <td>" . $arrData['newest_display'] . "</td>\n");
			echo(" <td>\n");
			echo(" <table style='width: 100px; border: none;'><tr>\n");
			echo(" <td style='width: " . $fltShare . "%; background-color: #3b82f6;'></td>\n");
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
list($arrQueueStats, $intTotalStorage) = getQueueStats();
outputHtmlReport($arrQueueStats, $intTotalStorage);

?>