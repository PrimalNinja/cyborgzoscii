<?php

// Cyborg ZOSCII MQ v20261003
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Octopus Adapter Template (adapters/example.php)
//
// Rename this file to your adapter name (e.g. adapters/myprocessor.php)
// and invoke it via:
//   php octopus.php --url=... --sq=sourcequeue --tq=targetqueue --adapter=myprocessor
//
// This function is called once per message fetched from the source queue.
//
// Parameters:
//   $strName_a    - The original message filename (YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin)
//   $binContent_a - The raw binary content of the message
//   $arrParams_a  - Associative array of any extra parameters passed to octopus
//                   (CLI: --key=value beyond url/sq/tq/adapter; HTTP: any extra GET/POST keys)
//
// Return:
//   string  - Processed content to publish/store (may be same as input, transformed, etc.)
//   null    - Skip this message; octopus advances the pointer and moves on
//   false   - Same as null; skip this message
//
// --- Example: XML-to-JSON translation ---
// This adapter expects messages containing XML like:
//
//   [xml declaration: version="1.0"]
//   <order>
//     <order_id>ORD-001</order_id>
//     <customer>Acme Corp</customer>
//     <amount>149.95</amount>
//     <currency>AUD</currency>
//     <status>pending</status>
//   </order>
//
// It translates each message to a JSON object and publishes it to the target queue.

function octopusProcess($strName_a, $binContent_a, $arrParams_a)
{
	echo("  Processing: " . $strName_a . " (" . strlen($binContent_a) . " bytes)\n");

	// Parse the incoming XML
	$objXML = @simplexml_load_string($binContent_a);

	if ($objXML === false)
	{
		// Not valid XML — skip this message
		echo("  WARNING: Could not parse XML in " . $strName_a . " — skipping.\n");
		return null;
	}

	// Map XML fields to an associative array
	// Extend or rename these fields to match your actual schema
	$arrData = [
		'order_id' => isset($objXML->order_id) ? (string)$objXML->order_id : '',
		'customer' => isset($objXML->customer) ? (string)$objXML->customer : '',
		'amount'   => isset($objXML->amount)   ? (float)(string)$objXML->amount : 0.0,
		'currency' => isset($objXML->currency) ? (string)$objXML->currency : '',
		'status'   => isset($objXML->status)   ? (string)$objXML->status   : '',
	];

	// Encode to JSON
	$strJSON = json_encode($arrData);

	if ($strJSON === false)
	{
		echo("  WARNING: JSON encoding failed for " . $strName_a . " — skipping.\n");
		return null;
	}

	echo("  Converted to JSON: " . $strJSON . "\n");

	return $strJSON;
}