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
    } 
    else 
    {
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
