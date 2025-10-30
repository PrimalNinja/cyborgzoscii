<?php

// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Test Harness (test-get.php)
// 
// This script is used to populate your ZOSCII MQ system (via index.php)
// with sample data for testing the statissa.php report generator.
// 
// NOTE: This version sends ALL data (q, r, msg) via the URL query string ($_GET), 
// matching the protocol required by the latest index.php.
//  
// Execution:
// 1. Ensure your web server (Apache/Nginx/etc.) is running and serving the directory.
// 2. Run from the command line: php test-get.php
//  
// NOTE: This version requires the PHP cURL extension to be installed and enabled.

// --- Configuration ---
// IMPORTANT: Adjust this URL to point to your index.php endpoint.
// The base URL should NOT include any query parameters (?action=publish), as they are added by the script.
define('PUBLISH_URL', 'http://localhost/zosciimq/index.php');

function runTestBatch($strQueueName, $intRetentionDays, $intCount, $strBasePayload)
{
    // Changed interpolation to concatenation here:
    echo("\nPublishing " . $intCount . " messages to queue '" . $strQueueName . "' (Retention: " . $intRetentionDays . " days):\n");

    // Renamed counter $i to $intI
    for ($intI = 1; $intI <= $intCount; $intI++) 
	{
        // Set debug flag only for the first message of each batch
        $blnDebug = ($intI === 1);

        $strPayload = $strBasePayload . $intI . " - Timestamp: " . time();
        
        // Pass the debug flag to the request function
        $strResult = sendPublishRequest($strQueueName, $intRetentionDays, $strPayload, $blnDebug);

        // Check for success string from index.php (it starts with 'Success:')
        if (strpos($strResult, 'Success:') !== false) 
		{
            // Changed interpolation to concatenation here:
            echo("  [SUCCESS] Message #" . $intI . " published.\n");
        } 
		else 
		{
            // Changed interpolation to concatenation here:
            echo("  [FAILURE] Message #" . $intI . " failed. Response: " . $strResult . "\n");
            // Halt on the first failure to prevent queue flooding on error
            return; 
        }
        // Small delay to ensure unique file timestamps if running quickly
        usleep(50000); 
    }
}

function sendPublishRequest($strQueueName, $intRetentionDays, $strPayload, $blnDebug = false)
{
    // Build the query string containing ALL parameters (action, q, r, msg)
    $arrQueryParams = [
        'action' => 'publish',
        'q' => $strQueueName,
        'r' => $intRetentionDays, // Note the 'r' for retention, matching new index.php
        'msg' => $strPayload
    ];

    $strQuery = http_build_query($arrQueryParams);

    // The entire request is now a GET URL
    $strTargetUrl = PUBLISH_URL . '?' . $strQuery;

    // --- cURL Implementation (GET) ---
    // Renamed cURL handle $ch to $objCurl
    $objCurl = curl_init($strTargetUrl);

    // Set cURL options for GET
    curl_setopt($objCurl, CURLOPT_RETURNTRANSFER, true); // Return the response as a string
    curl_setopt($objCurl, CURLOPT_HTTPGET, true);        // Ensure method is GET
    curl_setopt($objCurl, CURLOPT_TIMEOUT, 5);           // 5 second timeout
    
    // Execute the request
    $strResponse = curl_exec($objCurl);

    // --- DEBUG CHECK 1: cURL Connection Failure ---
    if (curl_errno($objCurl)) 
	{
        $strError = curl_error($objCurl);
        
        if ($blnDebug) 
		{
            echo("\n--- DEBUG INFO (cURL Connection Failure) ---\n");
            echo("Target URL: " . $strTargetUrl . "\n");
            echo("cURL Error Code: " . curl_errno($objCurl) . "\n");
            echo("cURL Error Message: " . $strError . "\n");
            echo("------------------------------------------\n");
        }

        curl_close($objCurl);
        return "cURL ERROR: Failed to connect or execute request: " . $strError;
    }

    // Capture HTTP Status Code for successful connection but potential server error
    $intHttpStatus = curl_getinfo($objCurl, CURLINFO_HTTP_CODE);

    // Close cURL session
    curl_close($objCurl);

    // --- DEBUG CHECK 2: Non-200 HTTP Status ---
    if ($intHttpStatus != 200) 
	{
        if ($blnDebug) 
		{
            echo("\n--- DEBUG INFO (Server HTTP Error) ---\n");
            echo("Target URL: " . $strTargetUrl . "\n");
            echo("HTTP Status Code: " . $intHttpStatus . "\n");
            echo("Server Output (Error Message): " . $strResponse . "\n");
            echo("--------------------------------------\n");
        }
    }

    // DEBUG CHECK 3: Empty or Invalid Response
    if ($strResponse === false) 
	{
        $strResponse = "ERROR: Response was empty or invalid.";
    }

    return $strResponse;
}

// --- Main Execution Block ---
echo("--- Starting ZOSCII MQ Test Data Publisher ---\n");

// Check if cURL is installed before attempting to run tests
if (!extension_loaded('curl')) 
{
    echo("\nFATAL ERROR: The cURL PHP extension is not installed or enabled. Please enable it in your php.ini.\n");
    echo("--- Test Halted ---\n");
    exit(1);
}

// 1. Publish 5 messages to 'invoices' queue (Retention: 1 day)
runTestBatch('invoices', 1, 5, "Sample invoice data for client ID ");

// 2. Publish 3 messages to 'logging' queue (Retention: 7 days)
runTestBatch('logging', 7, 3, "CRITICAL: System event log entry ");

// 3. Publish 2 messages to 'archive' queue (Retention: 0 days - infinite)
runTestBatch('archive', 0, 2, "High-priority long-term archival payload ");


echo("--- Test Complete. Total messages sent: 10 ---\n");
echo("Now check the result by opening statissa.php in your browser.\n");

?>
