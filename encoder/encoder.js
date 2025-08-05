var g_arrRomData = null;
var g_strTextData = "";
var g_objEncodingResult = null;
var g_BITTAGE = 16;		// 16 or 32 bits

$(document).ready(function() 
{
    setupDropZones();
    setupButtons();
});

function checkEncodeReady() 
{
    var blnReady = g_arrRomData && g_strTextData && g_strTextData.length > 0;
    $("#encodeBtn").prop("disabled", !blnReady);
}

function displayAnalysis(objResult_a) 
{
    var strHtml = "<h3>Encoding Results</h3>";
    strHtml += "<div class='row'>";
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Input Information</h5>";
    strHtml += "<table class='table table-sm analysis-table'>";

	var strDisplayText = objResult_a.originalText.length > 100 ? 
		objResult_a.originalText.substring(0, 100) + "..." : 
		objResult_a.originalText;
	strHtml += "<tr><td><strong>Original Text:</strong></td><td>" + escapeHtml(strDisplayText) + "</td></tr>";

    strHtml += "<tr><td><strong>Text Length:</strong></td><td>" + objResult_a.originalText.length + " characters</td></tr>";
    strHtml += "<tr><td><strong>Encoding:</strong></td><td>" + objResult_a.encoding.toUpperCase() + "</td></tr>";
    strHtml += "<tr><td><strong>Addresses Generated:</strong></td><td>" + objResult_a.addressCount + "</td></tr>";
    strHtml += "</table>";
    strHtml += "</div>";
    
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Address List</h5>";
    strHtml += "<div style='max-height: 200px; overflow-y: auto;'>";
    strHtml += "<table class='table table-sm analysis-table'>";
    strHtml += "<thead><tr><th>Index</th><th>Address (Hex)</th><th>Address (Dec)</th></tr></thead>";
    strHtml += "<tbody>";
    
	var intMaxRows = 50;
	for (var intI = 0; intI < Math.min(objResult_a.addresses.length, intMaxRows); intI++) 
	{
		var intAddr = objResult_a.addresses[intI];
		if (intAddr !== undefined)
		{
			strHtml += "<tr>";
			strHtml += "<td>" + intI + "</td>";
			strHtml += "<td>0x" + intAddr.toString(16).toUpperCase() + "</td>";
			strHtml += "<td>" + intAddr + "</td>";
			strHtml += "</tr>";
		}
	}
	if (objResult_a.addresses.length > intMaxRows)
	{
		strHtml += "<tr><td colspan='3'><em>... and " + (objResult_a.addresses.length - intMaxRows) + " more addresses (use 'Generate Address File' to get all)</em></td></tr>";
	}
    
    strHtml += "</tbody></table>";
    strHtml += "</div>";
    strHtml += "</div>";
    strHtml += "</div>";
    
    strHtml += "<div class='mt-4'>";
    strHtml += "<h5>Generate Viewer</h5>";
    strHtml += "<p>Create a standalone viewer file that can decode this ZOSCII data:</p>";
    strHtml += "<button class='btn btn-success' id='generateAddressFileBtn'>Generate Address File</button>";
    strHtml += "<div id='viewerResult' class='mt-3'></div>";
    strHtml += "</div>";
    
    $("#analysisContent").html(strHtml);
    
    // Setup generate viewer button
    $("#generateAddressFileBtn").click(function() 
    {
        $(this).prop("disabled", true).text("Generating...");
        
        // Create binary file with addresses
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
        
        // Download only the binary address file
        downloadFile(arrAddressBytes, 'zoscii_addresses_' + new Date().getTime() + '.bin', 'application/octet-stream');
        
        var strResultHtml = "<div class='alert alert-success'>";
        strResultHtml += "<strong>Address file generated successfully!</strong><br>";
        strResultHtml += "Binary address file downloaded. Use with separate ZOSCII viewer.";
        strResultHtml += "</div>";
        $("#viewerResult").html(strResultHtml);
        
        $(this).prop("disabled", false).text("Generate Address File");
    });
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
		var arrMemoryBlocks = [
			{start: 0, size: g_arrRomData.length}
		];
        
        if (strEncoding === 'petscii')
        {
            fnConverter = petsciiToAscii;
        }
        else if (strEncoding === 'ebcdic')
        {
            fnConverter = ebcdicToAscii;
        }
       
        var arrAddresses = toZOSCII(g_arrRomData, g_strTextData, arrMemoryBlocks, fnConverter, 42, g_BITTAGE);
        
        g_objEncodingResult = {
            success: true,
            addresses: arrAddresses,
            originalText: g_strTextData,
            encoding: strEncoding,
            addressCount: arrAddresses.length
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

