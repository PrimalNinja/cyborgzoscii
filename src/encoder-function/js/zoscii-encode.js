// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Function to convert string or binary data to ZOSCII address sequence
// arrBinaryData_a: Uint8Array containing the ROM/binary data  
// mixedInputData_a: String or Uint8Array containing the data to convert
// arrMemoryBlocks_a: array of {start: startAddress, size: blockSize} objects
// cbConverter_a: encoding conversion function (e.g., petsciiToAscii, ebcdicToAscii) or null
// intUnmappableChar_a: the native character code to be used if it cannot be mapped to ASCII
// Returns: {addresses: array, inputCounts: array, romCounts: array}

function toZOSCII(arrBinaryData_a, mixedInputData_a, arrMemoryBlocks_a, cbConverter_a, intUnmappableChar_a)
{
    var intStartTime = new Date().getTime();
	
    var intI;
    var intBlock;
    var intResultIndex = 0;
    var intResultCount = 0;
    var intDebugMissing = 0;
    
    var arrByteCounts = new Array(256);
    var arrByteAddresses = new Array(256);
    var arrOffsets = new Array(256);
    var arrInputCounts = new Array(256);
	var intAddress;
	var intByte;
	var intIndex;
	var objBlock;
    
    // Convert input to consistent format
    var arrInputData_a;
    var blnIsString = false;
    
    if (typeof mixedInputData_a === 'string') 
	{
        // Handle string input - convert to UTF-8 bytes
        arrInputData_a = new TextEncoder().encode(mixedInputData_a);
        blnIsString = true;
    } else {
        // Handle Uint8Array input
        arrInputData_a = mixedInputData_a;
        blnIsString = false;
    }
    
    // Initialize counters
    for (intI = 0; intI < 256; intI++)
    {
        arrByteCounts[intI] = 0;
        arrInputCounts[intI] = 0;
    }
	
	clearLeakageTracker();
    
    // Pass 1: Count occurrences by iterating through blocks
    for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
    {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
        {
            intByte = arrBinaryData_a[intAddress];
            arrByteCounts[intByte]++;
        }
    }
    
    // Pass 2: Pre-allocate exact-sized arrays
    for (intI = 0; intI < 256; intI++)
    {
        arrByteAddresses[intI] = new Array(arrByteCounts[intI]);
        arrOffsets[intI] = 0;
    }
    
    // Pass 3: Populate arrays by iterating through blocks
    for (intBlock = 0; intBlock < arrMemoryBlocks_a.length; intBlock++)
    {
        objBlock = arrMemoryBlocks_a[intBlock];
        for (intAddress = objBlock.start; intAddress < (objBlock.start + objBlock.size); intAddress++)
        {
            intByte = arrBinaryData_a[intAddress];
            arrByteAddresses[intByte][arrOffsets[intByte]] = intAddress;
			arrOffsets[intByte]++;
        }
    }
    
    // Build result array with random addresses - pre-allocate and avoid push()
    for (intI = 0; intI < arrInputData_a.length; intI++)
    {
        intIndex = arrInputData_a[intI];  // Direct byte value
        
        // Apply encoding conversion if provided
        if (cbConverter_a)
        {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }
        
        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
        {
            intResultCount++;
        }
        else
        {
            intDebugMissing++;
            if (intDebugMissing <= 10)
            {
                var strHexByte = arrInputData_a[intI].toString(16).toUpperCase();
                if (strHexByte.length < 2) strHexByte = "0" + strHexByte;
                
                if (blnIsString) 
				{
                    console.log("Missing character: '" + String.fromCharCode(arrInputData_a[intI]) + "' (code " + arrInputData_a[intI] + "/0x" + strHexByte + " -> " + intIndex + ")");
                } 
				else 
				{
                    console.log("Missing byte: " + arrInputData_a[intI] + " (0x" + strHexByte + " -> " + intIndex + ")");
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

    var arrResult = new Array(intResultCount);

    for (intI = 0; intI < arrInputData_a.length; intI++)
    {
        intIndex = arrInputData_a[intI];  // Direct byte value
        
        // Apply encoding conversion if provided
        if (cbConverter_a)
        {
            intIndex = cbConverter_a(intIndex, intUnmappableChar_a);
        }

        if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] && arrByteAddresses[intIndex].length > 0)
        {
            arrInputCounts[intIndex]++;
            var intRandomPick = Math.floor(Math.random() * arrByteAddresses[intIndex].length);
            arrResult[intResultIndex] = arrByteAddresses[intIndex][intRandomPick];
			if (arrInputData_a.length <= 65535)
			{
				trackAddressUsage(arrByteAddresses[intIndex][intRandomPick], intResultIndex);
			}
			intResultIndex++;
        }
    }

    var intEndTime = new Date().getTime();
    var intElapsedMs = intEndTime - intStartTime;
    
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
}

// Function to convert PETSCII character codes to ASCII character codes
// intPetsciiChar_a: PETSCII character code (0-255)
// intUnmappableChar_a: the native character code to be used if it cannot be mapped to ASCII
// Returns: ASCII character code or intUnmappableChar_a (suggested *) for unmappable characters
function petsciiToAscii(intPetsciiChar_a, intUnmappableChar_a)
{
    var arrPetsciiToAsciiMap = [
        // 0-31: Control characters
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 32-63: Space, digits, punctuation (direct ASCII mapping)
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        
        // 64-95: @A-Z[\]^_ (direct ASCII mapping)
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        
        // 96-255: Everything else
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a
    ];
    
    return arrPetsciiToAsciiMap[intPetsciiChar_a];
}

// Function to convert EBCDIC character codes to ASCII character codes
// intEbcdicChar_a: EBCDIC character code (0-255)
// intUnmappableChar_a: the native character code to be used if it cannot be mapped to ASCII
// Returns: ASCII character code or intUnmappableChar_a (suggested) for unmappable characters
function ebcdicToAscii(intEbcdicChar_a, intUnmappableChar_a)
{
    var arrEbcdicToAsciiMap = [
        // 0-63: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 64-79: Space and some punctuation
        32, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, 46, 60, 40, 43, 124, // Space .....<(+|
        
        // 80-95: & and punctuation
        38, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, 33, 36, intUnmappableChar_a, 41, 59, intUnmappableChar_a, // &.........!$.).;.
        
        // 96-111: - and punctuation
        45, 47, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, 44, 37, 95, 62, 63, // -/........,%_>?
        
        // 112-127: More punctuation
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, 96, 58, 35, 64, 39, 61, 34, // .........`:#@'="
        
        // 128: Control
        intUnmappableChar_a,
        
        // 129-137: a-i
        97, 98, 99, 100, 101, 102, 103, 104, 105,
        
        // 138-144: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 145-153: j-r  
        106, 107, 108, 109, 110, 111, 112, 113, 114,
        
        // 154-161: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 162-169: s-z
        115, 116, 117, 118, 119, 120, 121, 122,
        
        // 170-192: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 193-201: A-I
        65, 66, 67, 68, 69, 70, 71, 72, 73,
        
        // 202-208: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 209-217: J-R
        74, 75, 76, 77, 78, 79, 80, 81, 82,
        
        // 218-225: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 226-233: S-Z
        83, 84, 85, 86, 87, 88, 89, 90,
        
        // 234-239: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a,
        
        // 240-249: 0-9
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
        
        // 250-255: Control/special
        intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a, intUnmappableChar_a
    ];
    
    return arrEbcdicToAsciiMap[intEbcdicChar_a];
}
