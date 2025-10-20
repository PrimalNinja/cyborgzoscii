// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

var g_ANALYSELEAKAGE = false;	// pattern matching of addresses do not necessarily mean reduced security given high entropy ROMs
var g_arrRomData = null;
var g_strTextData = "";
var g_objEncodingResult = null;
var g_BITTAGE = 16;		// 16 or 32 bits
var g_arrBinaryInputData = null; 

var g_objLeakageTracker = {
    addressUsage: {},           // address -> count
    positionMap: {},            // address -> [positions where used]
    leakedPatterns: [],         // Array of leaked frequency patterns
    securityReduction: 0.0      // Running security loss calculation
};

window.onload = function()
{
    setupDropZones();
    setupButtons();
    initializeChannelNumber();
};

function initializeChannelNumber()
{
    var intRandomChannel = Math.floor(Math.random() * 10001);
    document.getElementById("txtChannelNumber").value = intRandomChannel;
}

function calculateROMStrengthGeneral(objEncodingResult_a)
{
    var intI;
    var fltLogSum = 0.0;
    
    for (intI = 0; intI < 256; intI++)
    {
        if (objEncodingResult_a.romCounts[intI] > 0)
        {
            fltLogSum += Math.log10(objEncodingResult_a.romCounts[intI]);
        }
    }
    
    return fltLogSum;
}

// now caters for duplicate which have two real meanings...
// 1. it can indicate less security if the type of data encoded is known
// 2. no impact to security if the type of data encoded is NOT known
//
// note: leakage is only currently displayed for files larger than 65535 bytes because of performance reasons
//		 and anything near there with a good ROM doesn't affect security strength
//		 (the leakage indicator is purely to identify known data for small encoded values)
function calculateROMStrengthFileWithLeakage(objEncodingResult_a, blnDataTypeKnown_a)
{
	var blnDataTypeKnown = blnDataTypeKnown_a;
	if (blnDataTypeKnown === undefined) { blnDataTypeKnown = false; }

    var intI;
    var fltLogSum = 0.0;
    
    if (!blnDataTypeKnown) 
    {
        // Scenario 2: Data type unknown - use original calculation
        // Address repetitions don't reveal semantic meaning
        for (intI = 0; intI < 256; intI++)
        {
            if (objEncodingResult_a.inputCounts[intI] > 0 && objEncodingResult_a.romCounts[intI] > 0)
            {
                fltLogSum += objEncodingResult_a.inputCounts[intI] * Math.log10(objEncodingResult_a.romCounts[intI]);
            }
        }
        return fltLogSum;
    }
    else
    {
        // Scenario 1: Data type known - calculate leakage reduction for THIS FILE
        
        // Build frequency distribution for this specific file
        var arrFrequencies = [];
        var intTotalInputLength = 0;
        
        for (intI = 0; intI < 256; intI++)
        {
            if (objEncodingResult_a.inputCounts[intI] > 0)
            {
                arrFrequencies.push({
                    frequency: objEncodingResult_a.inputCounts[intI],
                    romCount: objEncodingResult_a.romCounts[intI]
                });
                intTotalInputLength += objEncodingResult_a.inputCounts[intI];
            }
        }
        
        // Sort by frequency (highest leak risk first)
		arrFrequencies.sort(function(a, b) 
		{
			return b.frequency - a.frequency;
		});
        
        // Calculate expected unique addresses for THIS FILE's encoding
        var fltExpectedUniqueAddresses = 0;
        
        for (intI = 0; intI < arrFrequencies.length; intI++)
        {
            var objFreq = arrFrequencies[intI];
            // Expected unique addresses for this character type
            var fltUnique = objFreq.romCount * (1 - Math.pow(1 - 1/objFreq.romCount, objFreq.frequency));
            fltExpectedUniqueAddresses += fltUnique;
        }
        
        // File security is based on unique address count for THIS FILE
        return Math.log10(fltExpectedUniqueAddresses);
    }
}

function checkEncodeReady() 
{
    var blnReady = g_arrRomData && ((g_strTextData && g_strTextData.length > 0) || (g_arrBinaryInputData && g_arrBinaryInputData.length > 0));
    document.getElementById("verifyBtn").disabled = !blnReady;
    document.getElementById("submitBtn").disabled = !g_objEncodingResult;
}

function clearLeakageTracker()
{
    g_objLeakageTracker.addressUsage = {};
    g_objLeakageTracker.positionMap = {};
    g_objLeakageTracker.leakedPatterns = [];
    g_objLeakageTracker.securityReduction = 0.0;
}

function displayAnalysis(objEncodingResult_a) 
{
	var intI;
    var strHtml = '<div class="scrollable-content"><h3>Encoding Results</h3>';
	
    strHtml += "<div class='row'>";
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Input Information</h5>";
	strHtml += "<table class='table table-sm analysis-table'>";
	strHtml += "<colgroup><col style='width: 40%;'><col style='width: 60%;'></colgroup>";

    var strDisplayText = "";
	if (objEncodingResult_a.originalText.length > 100)
	{
        strDisplayText = objEncodingResult_a.originalText.substring(0, 100) + "...";
	}
	else
	{
        strDisplayText = objEncodingResult_a.originalText;
	}
    strHtml += "<tr><td>>Original Input:</td><td>" + escapeHtml(strDisplayText) + "</td></tr>";

    strHtml += "<tr><td>>Input Length:</td><td>" + objEncodingResult_a.inputLength + " bytes</td></tr>";
    strHtml += "<tr><td>>Encoding:</td><td>" + objEncodingResult_a.encoding.toUpperCase() + "</td></tr>";
    strHtml += "<tr><td>>Addresses Generated:</td><td>" + objEncodingResult_a.addressCount + "</td></tr>";
    strHtml += "</table>";
    strHtml += "</div>";
    
	strHtml += displayROMStrength(objEncodingResult_a);

    // --- Generate Viewer Section ---
    strHtml += "<div class='mt-4'>";
    strHtml += "<h5>Download Address File</h5>";
    strHtml += "<p>Create the encoded ZOSCII data file:</p>";
    strHtml += "<button class='btn btn-success' id='generateAddressFileBtn'>Download Address File</button>";
    strHtml += "<div id='viewerResult' class='mt-3'></div>";
    strHtml += "</div>";

	if (g_ANALYSELEAKAGE)
	{
		if ((g_arrBinaryInputData && g_arrBinaryInputData.length <= 65535) || 
			(!g_arrBinaryInputData && g_strTextData && g_strTextData.length <= 65535))
		{
			strHtml += displayLeakageAnalysis();
		}
	}
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Ready to Submit</h5>";
    strHtml += "<p>Address file will be submitted to the server upon clicking 'Submit to Server'</p>";
    strHtml += "</div>";
    strHtml += "</div>";
	
    // --- ROM Strength Section ---
	strHtml += "<div class='mt-4'>";
	strHtml += "<h5>Character Usage</h5>";
    strHtml += "<table class='table table-sm analysis-table'><thead><tr><th>Byte</th><th>Dec</th><th>ROM Count</th><th>Input Count</th><th>Char</th></tr></thead><tbody>";
    
	for (var strByte = 0; strByte < 256; strByte++) 
	{
        var intROMCount = objEncodingResult_a.romCounts[strByte];
		var intInputCount = objEncodingResult_a.inputCounts[strByte];
        var strChar = '';
        var strStyle = "";
		
        if (strByte >= 32 && strByte <= 126) 
		{
			// Printable ASCII
            strChar = String.fromCharCode(strByte);
            if (intROMCount >= 5) 
			{
                strStyle = "color:#155724;background:#d4edda;"; // green
            } 
			else 
			{
                strStyle = "color:#721c24;background:#f8d7da;"; // red
            }
        } 
		else 
		{
            strChar = '&nbsp;';
            if (intROMCount >= 5) 
			{
				strStyle = "color:#6c757d;background:#f8f9fa;"; // gray
            } 
			else 
			{
                strStyle = "color:#721c24;background:#f8d7da;"; // red
            }
        }

		var strHexByte = strByte.toString(16).toUpperCase();
		if (strHexByte.length < 2)
		{
			strHexByte = "0" + strHexByte;
		}
		
        strHtml += "<tr style='" + strStyle + "'>";
		strHtml += "<td>0x" + strHexByte + "</td>";
        strHtml += "<td>"+strByte+"</td>";
        strHtml += "<td>"+intROMCount+"</td>";
		strHtml += "<td>"+intInputCount+"</td>";
        strHtml += "<td>"+strChar+"</td>";
        strHtml += "</tr>";
    }
    strHtml += "</tbody></table></div></div>";
    
    document.getElementById("analysisContent").innerHTML = strHtml;

    // Setup generate viewer button
    document.getElementById("generateAddressFileBtn").addEventListener("click", function() 
    {
        this.disabled = true;
        this.textContent = "Generating...";
        
        var arrAddressBytes;
        var intAddress;
        var intI;
        if (g_BITTAGE === 16)
        {
            arrAddressBytes = new Uint8Array(g_objEncodingResult.addresses.length * 2);
            for (intI = 0; intI < g_objEncodingResult.addresses.length; intI++)
            {
                intAddress = g_objEncodingResult.addresses[intI];
                arrAddressBytes[intI * 2] = intAddress & 0xFF;
                arrAddressBytes[intI * 2 + 1] = (intAddress >> 8) & 0xFF;
            }
        }
        else if (g_BITTAGE === 32)
        {
            arrAddressBytes = new Uint8Array(g_objEncodingResult.addresses.length * 4);
            for (intI = 0; intI < g_objEncodingResult.addresses.length; intI++)
            {
                intAddress = g_objEncodingResult.addresses[intI];
                arrAddressBytes[intI * 4] = intAddress & 0xFF;
                arrAddressBytes[intI * 4 + 1] = (intAddress >> 8) & 0xFF;
                arrAddressBytes[intI * 4 + 2] = (intAddress >> 16) & 0xFF;
                arrAddressBytes[intI * 4 + 3] = (intAddress >> 24) & 0xFF;
            }
        }
        downloadFile(arrAddressBytes, 'zoscii_addresses_' + new Date().getTime() + '.bin', 'application/octet-stream');
        
        var strResultHtml = "<div class='alert alert-success'>";
        strResultHtml += "<strong>Address file generated successfully!</strong><br>";
        strResultHtml += "Binary address file downloaded. Use with separate ZOSCII viewer.";
        strResultHtml += "</div>";
        document.getElementById("viewerResult").innerHTML = strResultHtml;
        
        this.disabled = false;
        this.textContent = "Download Address File";
    });
}

function displayLeakageAnalysis()
{
    var intUniqueAddresses = 0;
    var strAddress;
    var strHtml = "";
    
    // Count unique addresses
    for (strAddress in g_objLeakageTracker.addressUsage) 
    {
        if (g_objLeakageTracker.addressUsage.hasOwnProperty(strAddress)) 
        {
            intUniqueAddresses++;
        }
    }
    
    strHtml += "<div class='mt-4'>";
    strHtml += "<h5>Leakage Analysis</h5>";
    strHtml += "<table class='table table-sm analysis-table'>";
    strHtml += "<colgroup><col style='width: 40%;'><col style='width: 60%;'></colgroup>";
    strHtml += "<tr><td>Total Unique Addresses:</td><td>" + intUniqueAddresses + "</td></tr>";
    strHtml += "<tr><td>Total Leaked Patterns:</td><td>" + g_objLeakageTracker.leakedPatterns.length + "</td></tr>";
    strHtml += "</table>";
    
    if (g_objLeakageTracker.leakedPatterns.length > 0)
    {
        strHtml += "<h6>Detected Leakages</h6>";
        strHtml += "<table class='table table-sm analysis-table'>";
        strHtml += "<thead><tr><th>Address (Hex)</th><th>Frequency</th><th>Positions</th><th>Security Impact</th></tr></thead>";
        strHtml += "<tbody>";
        
        for (var intI = 0; intI < g_objLeakageTracker.leakedPatterns.length; intI++) 
        {
            var objPattern = g_objLeakageTracker.leakedPatterns[intI];
            strHtml += "<tr style='background-color: #f8d7da;'>";  // Light red background for leakages
            strHtml += "<td>0x" + objPattern.address.toString(16).toUpperCase() + "</td>";
            strHtml += "<td>" + objPattern.frequency + "x used</td>";
            strHtml += "<td>[" + objPattern.positions.join(", ") + "]</td>";
            strHtml += "<td>" + objPattern.frequency + "-frequency pattern revealed</td>";
            strHtml += "</tr>";
        }
        
        strHtml += "</tbody></table>";
    }
    else
    {
        strHtml += "<p style='color: #28a745;'><strong>No address leakage detected - optimal security</strong></p>";
    }
    
    strHtml += "</div>";
    
    return strHtml;
}

function displayROMStrength(objEncodingResult_a)
{
    var fltGeneralStrength = calculateROMStrengthGeneral(objEncodingResult_a);
    var fltFileStrength = calculateROMStrengthFileWithLeakage(objEncodingResult_a);
    var intCharactersUsed = 0;
    
    for (var intI = 0; intI < 256; intI++)
    {
        if (objEncodingResult_a.inputCounts[intI] > 0)
        {
            intCharactersUsed++;
        }
    }
    
    var fltUtilization = (intCharactersUsed / 256.0) * 100.0;
    
	var strHtml = "<div class='mt-4 col-md-6'>";
	strHtml += "<h5>ROM Strength Analysis</h5>";
	strHtml += "<table class='table table-sm analysis-table'>";
	strHtml += "<colgroup><col style='width: 40%;'><col style='width: 30%;'><col style='width: 30%;'></colgroup>";
	strHtml += "<tr><td>General ROM Capacity:</td><td>~10^" + fltGeneralStrength.toFixed(0) + "</td><td>" + exponentToLayman(fltGeneralStrength) + "</td></tr>";
	strHtml += "<tr><td>This File Security:</td><td>~10^" + fltFileStrength.toFixed(0) + "</td><td>" + exponentToLayman(fltFileStrength) + "</td></tr>";
	strHtml += "<tr><td>Characters Utilized:</td><td>" + intCharactersUsed + " of 256 (" + fltUtilization.toFixed(1) + "%)</td><td></td></tr>";
	strHtml += "</table>";
	strHtml += "</div>";
    
    return strHtml;
}

function downloadFile(objData_a, strFilename_a, strMimeType_a) 
{
    var objBlob = new Blob([objData_a], {type: strMimeType_a});
    var objUrl = URL.createObjectURL(objBlob);
    var objLink = document.createElement('a');
    objLink.href = objUrl;
    objLink.download = strFilename_a;
    document.body.appendChild(objLink);
    objLink.click();
    document.body.removeChild(objLink);
    URL.revokeObjectURL(objUrl);
}

function escapeHtml(strText_a) 
{
    var objDiv = document.createElement('div');
    objDiv.textContent = strText_a;
    return objDiv.innerHTML;
}

function exponentToLayman(fltExponent_a)
{
    var intRounded = Math.round(fltExponent_a);
    var strResult = "";

    if (intRounded <= 0)
    {
        strResult = "1";
    }
    else if (intRounded === 1)
    {
        strResult = "10";
    }
    else
    {
        strResult = "a 1 with " + intRounded.toLocaleString() + " zeros after it";
    }

    return strResult;
}

function formatLargeExponent(fltExponent_a)
{
	var strResult = "";
	
    if (fltExponent_a < 3)
    {
        strResult = "~" + Math.pow(10, fltExponent_a).toFixed(0) + " permutations";
    }
    else if (fltExponent_a < 6)
    {
        strResult = "~" + (Math.pow(10, fltExponent_a) / 1000).toFixed(1) + " thousand permutations";
    }
    else if (fltExponent_a < 9)
    {
        strResult = "~" + (Math.pow(10, fltExponent_a) / 1000000).toFixed(1) + " million permutations";
    }
    else if (fltExponent_a < 12)
    {
        strResult = "~" + (Math.pow(10, fltExponent_a) / 1000000000).toFixed(1) + " billion permutations";
    }
    else if (fltExponent_a < 15)
    {
        strResult = "~" + (Math.pow(10, fltExponent_a) / 1000000000000).toFixed(1) + " trillion permutations";
    }
    else if (fltExponent_a < 82)
    {
        strResult = "More than all atoms in the observable universe (10^" + fltExponent_a.toFixed(0) + " permutations)";
    }
    else if (fltExponent_a < 1000)
    {
        strResult = "Incomprehensibly massive (10^" + fltExponent_a.toFixed(0) + " permutations)";
    }
    else if (fltExponent_a < 1000000)
    {
        strResult = "Beyond all physical comparison (10^" + (fltExponent_a / 1000).toFixed(0) + " thousand permutations)";
    }
    else
    {
        strResult = "Astronomically secure (10^" + (fltExponent_a / 1000000).toFixed(1) + " million permutations)";
    }
	
	return strResult;
}

function makeAjaxRequest(strUrl_a, objOptions_a, cbSuccess_a, cbError_a) 
{
    var objXhr = new XMLHttpRequest();
    var strMethod = objOptions_a.method;
    var strResponseType = objOptions_a.responseType;
    var objData = objOptions_a.data;
    
    if (!strMethod) 
    {
        strMethod = 'GET';
    }
    if (!strResponseType) 
    {
        strResponseType = 'text';
    }
    
    objXhr.open(strMethod, strUrl_a);
    objXhr.responseType = strResponseType;
    
    objXhr.onload = function() 
    {
        cbSuccess_a(objXhr.response);
    };
    
    objXhr.onerror = function() 
    {
        cbError_a(objXhr.statusText);
    };
    
    if (objData) 
    {
        objXhr.send(objData);
    } 
    else 
    {
        objXhr.send();
    }
}

function setupButtons() 
{
	document.getElementById("clearBtn").addEventListener("click", function() 
	{
		g_arrRomData = null;
		g_strTextData = "";
		g_objEncodingResult = null;
		g_arrBinaryInputData = null;
		
		// Reset file inputs
		document.getElementById("romFileInput").value = "";
		document.getElementById("textFileInput").value = "";
		
		// Reset drop zones
		var objRomDropZone = document.getElementById("romDropZone");
		objRomDropZone.classList.remove("has-file");
		objRomDropZone.innerHTML = "<div>Drop ROM FILE here to encode or verify quality<br>or click to browse</div>";
		
		var objZosciiDropZone = document.getElementById("zosciiDropZone");
		objZosciiDropZone.classList.remove("has-file");
		objZosciiDropZone.innerHTML = "<div>Drop BINARY or TEXT FILE here<br>or click to browse</div>";
		
		// Reset analysis content
		document.getElementById("analysisContent").innerHTML = "<p class='text-muted'>Verify a ROM first to see quality analysis results.</p>";
		
		checkEncodeReady();
	});
    
	document.getElementById("verifyBtn").addEventListener("click", function() 
	{
		if (!g_arrRomData || (!g_strTextData && !g_arrBinaryInputData)) 
		{
			return;
		}
		
		this.disabled = true;
		this.textContent = "Verifying...";
        
        var strEncoding = "binary";
        var cbConverter = null;
		
		var intMaxSize;
		if (g_BITTAGE === 16)
		{
			intMaxSize = 65536;
		}
		else if (g_BITTAGE === 32)
		{
			intMaxSize = 4294967296;
		}
		
		var intSize = g_arrRomData.length;
		if (intSize > intMaxSize)
		{
			intSize = intMaxSize;
		}
		
		var arrMemoryBlocks = [
			{start: 0, size: intSize}
		];
        
		var varMixedInput = g_arrBinaryInputData || g_strTextData;
		var objResult = toZOSCII(g_arrRomData, varMixedInput, arrMemoryBlocks, cbConverter, 42);
        
		var strDisplayText;
		var intInputLength;
		if (g_arrBinaryInputData) 
		{
			strDisplayText = "Binary file data";
			intInputLength = g_arrBinaryInputData.length;
		} 
		else 
		{
			strDisplayText = g_strTextData;
			intInputLength = g_strTextData.length;
		}

		g_objEncodingResult = {
			success: true,
			addresses: objResult.addresses,
			originalText: strDisplayText,
			inputLength: intInputLength,
			encoding: strEncoding,
			addressCount: objResult.addresses.length,
			inputCounts: objResult.inputCounts,
			romCounts: objResult.romCounts
		};
        
        displayAnalysis(g_objEncodingResult);
        document.getElementById("analysis-tab").click();
        this.disabled = false;
        this.textContent = "Verify ROM";
        
        checkEncodeReady();
    });

	document.getElementById("submitBtn").addEventListener("click", function() 
	{
		if (!g_objEncodingResult) 
		{
			alert("Please verify the ROM first");
			return;
		}
		
		submitToServer(g_objEncodingResult.addresses);
		this.disabled = true; // Disable the submit button
		g_objEncodingResult = null; // Reset the encoding result
		checkEncodeReady(); // Update button states
    });
}

function submitToServer(arrAddresses_a) 
{
    var strChannelNumber = document.getElementById("txtChannelNumber").value.trim();
    var intRetentionWeeks = parseInt(document.getElementById("txtRetentionWeeks").value, 10) || 52;
    var intI;
    var arrBinary = new Uint8Array(arrAddresses_a.length * 2);
    for (intI = 0; intI < arrAddresses_a.length; intI++) 
	{
        var intAddress = arrAddresses_a[intI];
        arrBinary[intI * 2] = intAddress & 0xFF;
        arrBinary[intI * 2 + 1] = (intAddress >> 8) & 0xFF;
    }
    var objBlob = new Blob([arrBinary], {type: 'application/octet-stream'});
    var objFormData = new FormData();
    objFormData.append('addressfile', objBlob, 'message.bin');
    objFormData.append('channel', strChannelNumber);
    objFormData.append('retention', intRetentionWeeks.toString());
    makeAjaxRequest(document.getElementById("txtServerUrl").value.trim() + "/index.php", {
        method: 'POST',
        data: objFormData,
        responseType: 'json'
    }, function(objResponse_a) 
	{
        if (objResponse_a.success) 
		{
            console.log("Message submitted:", objResponse_a);
            alert("Successfully submitted to channel " + strChannelNumber);
        } 
		else 
		{
            console.error("Upload failed:", objResponse_a.error);
            alert("Failed to submit message: " + objResponse_a.error);
        }
    }, function(strError_a) 
	{
        console.error("Upload failed:", strError_a);
        alert("Failed to submit message: " + strError_a);
    });
}

function setupDropZone(strSelector_a, cbOnFileHandler) 
{
    var objDropZone = document.querySelector(strSelector_a);
    
    objDropZone.addEventListener("dragover", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        this.classList.add("dragover");
    });
    
    objDropZone.addEventListener("dragleave", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        this.classList.remove("dragover");
    });
    
    objDropZone.addEventListener("drop", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        this.classList.remove("dragover");
        
        var arrFiles = objEvent_a.dataTransfer.files;
        if (arrFiles.length > 0) 
        {
            cbOnFileHandler(arrFiles[0]);
        }
    });
}

function setupDropZones() 
{
    // ROM file drop zone
    setupDropZone("#romDropZone", function(objFile_a) 
    {
        var objReader = new FileReader();
        objReader.onload = function(objEvent_a) 
        {
            g_arrRomData = new Uint8Array(objEvent_a.target.result);
            var objRomDropZone = document.getElementById("romDropZone");
            objRomDropZone.classList.add("has-file");
            objRomDropZone.innerHTML = "<div>ROM file loaded: " + objFile_a.name + " (" + objFile_a.size + " bytes)</div>";
            checkEncodeReady();
        };
        objReader.readAsArrayBuffer(objFile_a);
    });

    // ROM drop zone click handler
    document.getElementById("romDropZone").addEventListener("click", function() 
    {
        document.getElementById("romFileInput").click();
    });
    
    // ROM file input change handler
    document.getElementById("romFileInput").addEventListener("change", function() 
    {
        if (this.files.length > 0) 
        {
            var objFile = this.files[0];
            var objReader = new FileReader();
            objReader.onload = function(objEvent_a) 
            {
                g_arrRomData = new Uint8Array(objEvent_a.target.result);
                var objRomDropZone = document.getElementById("romDropZone");
                objRomDropZone.classList.add("has-file");
                objRomDropZone.innerHTML = "<div>ROM file loaded: " + objFile.name + " (" + objFile.size + " bytes)</div>";
                checkEncodeReady();
            };
            objReader.readAsArrayBuffer(objFile);
        }
    });
	
    // UNIFIED input file handler
    function handleInputFile(objFile_a) 
    {
        // Clear all previous data
        g_arrBinaryInputData = null;
        g_strTextData = "";
        
        // Always read as binary - let toZOSCII handle it
        var objReader = new FileReader();
        objReader.onload = function(objEvent_a) 
        {
            g_arrBinaryInputData = new Uint8Array(objEvent_a.target.result);
            var objZosciiDropZone = document.getElementById("zosciiDropZone");
            objZosciiDropZone.classList.add("has-file");
            objZosciiDropZone.innerHTML = "<div>File loaded: " + objFile_a.name + " (" + objFile_a.size + " bytes)</div>";
            checkEncodeReady();
        };
        objReader.readAsArrayBuffer(objFile_a);
    }

    // Input zone drop handler
    setupDropZone("#zosciiDropZone", handleInputFile);
    
    // Input zone click handler  
    document.getElementById("zosciiDropZone").addEventListener("click", function() 
    {
        document.getElementById("textFileInput").click();
    });
    
    // File picker handler
    document.getElementById("textFileInput").addEventListener("change", function() 
    {
        if (this.files.length > 0) 
        {
            handleInputFile(this.files[0]);
        }
    });
}

function showTab(strTabName_a) 
{
	var intI;
	
    // Hide all tab panes
    var arrTabPanes = document.querySelectorAll('.tab-pane');
    for (intI = 0; intI < arrTabPanes.length; intI++) 
    {
        arrTabPanes[intI].classList.remove('active');
    }
    
    // Remove active class from all nav links
    var arrNavLinks = document.querySelectorAll('.nav-link');
    for (intI = 0; intI < arrNavLinks.length; intI++) 
    {
        arrNavLinks[intI].classList.remove('active');
    }
    
    // Show selected tab pane
    document.getElementById(strTabName_a).classList.add('active');
    
    // Add active class to selected nav link
    document.getElementById(strTabName_a + '-tab').classList.add('active');
}

function trackAddressUsage(intAddress, intPosition) 
{
    // Track this address usage
    if (!g_objLeakageTracker.addressUsage[intAddress]) 
	{
        g_objLeakageTracker.addressUsage[intAddress] = 0;
        g_objLeakageTracker.positionMap[intAddress] = [];
    }
    
    g_objLeakageTracker.addressUsage[intAddress]++;
    g_objLeakageTracker.positionMap[intAddress].push(intPosition);
    
    // Check for new leakage
    if (g_objLeakageTracker.addressUsage[intAddress] > 1) 
	{
        // LEAKAGE DETECTED!
        var arrPositions = g_objLeakageTracker.positionMap[intAddress];
        var intFrequency = arrPositions.length;
        
        g_objLeakageTracker.leakedPatterns.push({
            address: intAddress,
            frequency: intFrequency,
            positions: arrPositions.slice(), // copy array
            leakType: "FREQUENCY_PATTERN"
        });
        
        //console.log("LEAKAGE: Address 0x" + intAddress.toString(16).toUpperCase() + " reveals " + intFrequency + "-frequency pattern at positions [" + arrPositions.join(', ') + "]");
    }
}

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
			if (g_ANALYSELEAKAGE && arrInputData_a.length <= 65535)
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
