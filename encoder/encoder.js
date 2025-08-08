// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

var g_arrRomData = null;
var g_strTextData = "";
var g_objEncodingResult = null;
var g_BITTAGE = 16;		// 16 or 32 bits

$(document).ready(function() 
{
    setupDropZones();
    setupButtons();
});

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

function calculateROMStrengthFile(objEncodingResult_a)
{
    var intI;
    var fltLogSum = 0.0;
    
    for (intI = 0; intI < 256; intI++)
    {
        if (objEncodingResult_a.inputCounts[intI] > 0 && objEncodingResult_a.romCounts[intI] > 0)
        {
            fltLogSum += objEncodingResult_a.inputCounts[intI] * Math.log10(objEncodingResult_a.romCounts[intI]);
        }
    }
    
    return fltLogSum;
}

function checkEncodeReady() 
{
    var blnReady = g_arrRomData && g_strTextData && g_strTextData.length > 0;
    $("#encodeBtn").prop("disabled", !blnReady);
}

function displayAnalysis(objEncodingResult_a) 
{
	var intI;
    var strHtml = "<h3>Encoding Results</h3>";
	
    strHtml += "<div class='row'>";
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Input Information</h5>";
    strHtml += "<table class='table table-sm analysis-table'>";

    var strDisplayText = "";
	if (objEncodingResult_a.originalText.length > 100)
	{
        strDisplayText = objEncodingResult_a.originalText.substring(0, 100) + "...";
	}
	else
	{
        strDisplayText = objEncodingResult_a.originalText;
	}
    strHtml += "<tr><td><strong>Original Text:</strong></td><td>" + escapeHtml(strDisplayText) + "</td></tr>";

    strHtml += "<tr><td><strong>Text Length:</strong></td><td>" + objEncodingResult_a.originalText.length + " characters</td></tr>";
    strHtml += "<tr><td><strong>Encoding:</strong></td><td>" + objEncodingResult_a.encoding.toUpperCase() + "</td></tr>";
    strHtml += "<tr><td><strong>Addresses Generated:</strong></td><td>" + objEncodingResult_a.addressCount + "</td></tr>";
    strHtml += "</table>";
    strHtml += "</div>";
    
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Address List</h5>";
    strHtml += "<div style='max-height: 200px; overflow-y: auto;'>";
    strHtml += "<table class='table table-sm analysis-table'>";
    strHtml += "<thead><tr><th>Index</th><th>Address (Hex)</th><th>Address (Dec)</th></tr></thead>";
    strHtml += "<tbody>";
    
    var intMaxRows = 50;
    for (intI = 0; intI < Math.min(objEncodingResult_a.addresses.length, intMaxRows); intI++) 
    {
        var intAddr = objEncodingResult_a.addresses[intI];
        if (intAddr !== undefined)
        {
            strHtml += "<tr>";
            strHtml += "<td>" + intI + "</td>";
            strHtml += "<td>0x" + intAddr.toString(16).toUpperCase() + "</td>";
            strHtml += "<td>" + intAddr + "</td>";
            strHtml += "</tr>";
        }
    }
    if (objEncodingResult_a.addresses.length > intMaxRows)
    {
        strHtml += "<tr><td colspan='3'><em>... and " + (objEncodingResult_a.addresses.length - intMaxRows) + " more addresses (use 'Generate Address File' to get all)</em></td></tr>";
    }
    
    strHtml += "</tbody></table>";
    strHtml += "</div>";
    strHtml += "</div>";
    strHtml += "</div>";

    // --- Generate Viewer Section ---
    strHtml += "<div class='mt-4'>";
    strHtml += "<h5>Generate Address File</h5>";
    strHtml += "<p>Create the encoded ZOSCII data file:</p>";
    strHtml += "<button class='btn btn-success' id='generateAddressFileBtn'>Generate Address File</button>";
    strHtml += "<div id='viewerResult' class='mt-3'></div>";
    strHtml += "</div>";

    // --- ROM Strength Section ---
	strHtml += "<div class='mt-4'>";
	strHtml += displayROMStrength(objEncodingResult_a);
    //strHtml += "<div style='max-height:200px; overflow-y:auto;'>";
    strHtml += "<table class='table table-sm analysis-table'><thead><tr><th>Byte</th><th>Dec</th><th>ROM Count</th><th>Input Count</th><th>Char</th></tr></thead><tbody>";
    
	for (var strByte = 0; strByte < 256; strByte++) 
	{
        var intROMCount = objEncodingResult_a.romCounts[strByte];
		var intInputCount = objEncodingResult_a.inputCounts[strByte];
        var strChar = '';
        var strStyle = "";
		
        if (strByte >= 32 && strByte <= 126) 
		{ 
			// Printable ASCI
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
    strHtml += "</tbody></table></div>"; //</div>";
    
    $("#analysisContent").html(strHtml);

    // Setup generate viewer button
    $("#generateAddressFileBtn").click(function() 
    {
        $(this).prop("disabled", true).text("Generating...");
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
        $("#viewerResult").html(strResultHtml);
        $(this).prop("disabled", false).text("Generate Address File");
    });
}

function displayROMStrength(objEncodingResult_a)
{
    var fltGeneralStrength = calculateROMStrengthGeneral(objEncodingResult_a);
    var fltFileStrength = calculateROMStrengthFile(objEncodingResult_a);
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
	strHtml += "<tr><td>General ROM Capacity:</td><td>~10^" + fltGeneralStrength.toFixed(0) + "</td><td>" + exponentToLayman(fltGeneralStrength) + "</td></tr>";
	strHtml += "<tr><td>This File Security:</td><td>~10^" + fltFileStrength.toFixed(0) + "</td><td>" + exponentToLayman(fltFileStrength) + "</td></tr>";	strHtml += "<tr><td>Characters Utilized:</td><td>" + intCharactersUsed + " of 256 (" + fltUtilization.toFixed(1) + "%)</td><td></td></tr>";
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

function setupButtons() 
{
    $("#clearBtn").click(function() 
    {
        g_arrRomData = null;
        g_strTextData = "";
        g_objEncodingResult = null;
        
        $("#romDropZone").removeClass("has-file").html("<div>Drop ROMFILE here to encode ZOSCII</div>");
        $("#zosciiDropZone").removeClass("has-file").html("<div>Drop ZOSCII data here</div>");
        $("#textInput").val("");
        $("#analysisContent").html("<p class='text-muted'>Encode some data first to see analysis results.</p>");
        
        checkEncodeReady();
    });
    
    $("#encodeBtn").click(function() 
    {
        if (!g_arrRomData || !g_strTextData) 
        {
            return;
        }
        
        $(this).prop("disabled", true).text("Encoding...");
        
        var strEncoding = $("#encodingSelect").val();
        var fnConverter = null;
		
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
        
        if (strEncoding === 'petscii')
        {
            fnConverter = petsciiToAscii;
        }
        else if (strEncoding === 'ebcdic')
        {
            fnConverter = ebcdicToAscii;
        }
       
        var objResult = toZOSCII(g_arrRomData, g_strTextData, arrMemoryBlocks, fnConverter, 42);
        
        g_objEncodingResult = {
            success: true,
            addresses: objResult.addresses,
            originalText: g_strTextData,
            encoding: strEncoding,
            addressCount: objResult.addresses.length,
			inputCounts: objResult.inputCounts,
			romCounts: objResult.romCounts
        };
        
        displayAnalysis(g_objEncodingResult);
        $("#analysis-tab").click();
        $(this).prop("disabled", false).text("Encode");
    });
}

function setupDropZone(strSelector_a, cbOnFileHandler) 
{
    var objDropZone = $(strSelector_a);
    
    objDropZone.on("dragover", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        $(this).addClass("dragover");
    });
    
    objDropZone.on("dragleave", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        $(this).removeClass("dragover");
    });
    
    objDropZone.on("drop", function(objEvent_a) 
    {
        objEvent_a.preventDefault();
        $(this).removeClass("dragover");
        
        var arrFiles = objEvent_a.originalEvent.dataTransfer.files;
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
            $("#romDropZone").addClass("has-file").html("<div>ROM file loaded: " + objFile_a.name + " (" + objFile_a.size + " bytes)</div>");
            checkEncodeReady();
        };
        objReader.readAsArrayBuffer(objFile_a);
    });
    
    // Text data drop zone
    setupDropZone("#zosciiDropZone", function(objFile_a) 
    {
        var objReader = new FileReader();
        objReader.onload = function(objEvent_a) 
        {
            g_strTextData = objEvent_a.target.result;
            $("#textInput").val(g_strTextData);
            $("#zosciiDropZone").addClass("has-file").html("<div>Text file loaded: " + objFile_a.name + "</div>");
            checkEncodeReady();
        };
        objReader.readAsText(objFile_a);
    });
    
    // Text input change
    $("#textInput").on("input", function() 
    {
        g_strTextData = $(this).val();
        if (g_strTextData) 
        {
            $("#zosciiDropZone").addClass("has-file").html("<div>Text entered (" + g_strTextData.length + " characters)</div>");
        } else {
            $("#zosciiDropZone").removeClass("has-file").html("<div>Drop ZOSCII data here</div>");
        }
        checkEncodeReady();
    });
}

