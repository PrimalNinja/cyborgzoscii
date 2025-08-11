// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

/**
 * Converts a string or binary data to a ZOSCII address sequence.
 * @param {Uint8Array} arrBinaryData_a - The ROM/binary data to search.
 * @param {(string|Uint8Array)} mixedInputData_a - The data to convert.
 * @param {Array<Object>} arrMemoryBlocks_a - An array of memory block objects.
 * @returns {Object} An object containing the addresses and character counts.
 */
function toZOSCII(arrBinaryData_a, mixedInputData_a, arrMemoryBlocks_a) 
{
	const arrByteAddresses = new Array(256);
	const arrByteCounts = new Array(256);
	const arrInputCounts = new Array(256);
	const arrOffsets = new Array(256);

	let arrResult;
	let arrInputData_a;
	let blnIsString = false;
	let intBlock;
	let intI;
	let intDebugMissing = 0;
	let intResultCount = 0;
	let intResultIndex = 0;

	const intStartTime = new Date().getTime();
	
	if (typeof mixedInputData_a === 'string')
	{
		arrInputData_a = new TextEncoder().encode(mixedInputData_a);
		blnIsString = true;
	}
	else
	{
		arrInputData_a = mixedInputData_a;
		blnIsString = false;
	}
	
	for (intI = 0; intI < 256; intI++)
	{
		arrByteCounts[intI] = 0;
		arrInputCounts[intI] = 0;
	}
	
	// Pass 1: Count occurrences by iterating through blocks.
	for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
	{
		const objBlock = arrMemoryBlocks_a[intBlock];
		for (let intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
		{
			const intByte = arrBinaryData_a[intAddress];
			arrByteCounts[intByte]++;
		}
	}
	
	// Pass 2: Pre-allocate exact-sized arrays.
	for (intI = 0; intI < 256; intI++)
	{
		arrByteAddresses[intI] = new Array(arrByteCounts[intI]);
		arrOffsets[intI] = 0;
	}
	
	// Pass 3: Populate arrays by iterating through blocks.
	for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
	{
		const objBlock = arrMemoryBlocks_a[intBlock];
		for (let intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
		{
			const intByte = arrBinaryData_a[intAddress];
			arrByteAddresses[intByte][arrOffsets[intByte]] = intAddress;
			arrOffsets[intByte]++;
		}
	}
	
	// Build result array with random addresses.
	for (intI = 0; intI < arrInputData_a.length; intI++)
	{
		const intIndex = arrInputData_a[intI];
		
		if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
		{
			intResultCount++;
		}
		else
		{
			intDebugMissing++;
			if (intDebugMissing <= 10)
			{
				let strHexByte = arrInputData_a[intI].toString(16).toUpperCase();
				if (strHexByte.length < 2)
				{
					strHexByte = "0" + strHexByte;
				}
				
				if (blnIsString)
				{
					const missingChar = String.fromCharCode(arrInputData_a[intI]);
					const logMessage = "Missing character: '" + missingChar + "' (code " + arrInputData_a[intI] + "/0x" + strHexByte + " -> " + intIndex + ")";
					console.log(logMessage);
				}
				else
				{
					const logMessage = "Missing byte: " + arrInputData_a[intI];
					const codeMessage = "(0x" + strHexByte + " -> " + intIndex + ")";
					console.log(logMessage + " " + codeMessage);
				}
			}
		}
	}
	
	if (blnIsString)
	{
		console.log("Characters found in ROM: " + intResultCount);
		console.log("Characters missing from ROM: " + intDebugMissing);
	}
	else
	{
		console.log("Bytes found in ROM: " + intResultCount);
		console.log("Bytes missing from ROM: " + intDebugMissing);
	}
	
	arrResult = new Array(intResultCount);
	
	for (intI = 0; intI < arrInputData_a.length; intI++)
	{
		const intIndex = arrInputData_a[intI];
		
		if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
		{
			arrInputCounts[intIndex]++;
			const intRandomPick = Math.floor(Math.random() * arrByteAddresses[intIndex].length);
			arrResult[intResultIndex] = arrByteAddresses[intIndex][intRandomPick];
			intResultIndex++;
		}
	}
	
	const intEndTime = new Date().getTime();
	const intElapsedMs = intEndTime - intStartTime;
	
	console.log("ZOSCII Performance:");
	console.log("- Binary size: " + arrBinaryData_a.length + " bytes");
	console.log("- Input length: " + arrInputData_a.length + (blnIsString ? " characters" : " bytes"));
	console.log("- Memory blocks: " + arrMemoryBlocks_a.length);
	console.log("- Execution time: " + intElapsedMs + "ms");
	console.log("- Output addresses: " + arrResult.length);
	
	return {
		addresses: arrResult,
		inputCounts: arrInputCounts,
		romCounts: arrByteCounts
	};
};
