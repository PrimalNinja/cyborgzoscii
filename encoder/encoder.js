var g_arrRomData = null;
var g_strTextData = "";
var g_objEncodingResult = null;

$(document).ready(function() 
{
    fnSetupDropZones();
    fnSetupButtons();
});

function fnSetupDropZones() 
{
    // ROM file drop zone
    fnSetupDropZone("#romDropZone", function(objFile) 
    {
        var objReader = new FileReader();
        objReader.onload = function(objEvent) 
        {
            g_arrRomData = new Uint8Array(objEvent.target.result);
            $("#romDropZone").addClass("has-file").html("<div>ROM file loaded: " + objFile.name + " (" + objFile.size + " bytes)</div>");
            fnCheckEncodeReady();
        };
        objReader.readAsArrayBuffer(objFile);
    });
    
    // Text data drop zone
    fnSetupDropZone("#zosciiDropZone", function(objFile) 
    {
        var objReader = new FileReader();
        objReader.onload = function(objEvent) 
        {
            g_strTextData = objEvent.target.result;
            $("#textInput").val(g_strTextData);
            $("#zosciiDropZone").addClass("has-file").html("<div>Text file loaded: " + objFile.name + "</div>");
            fnCheckEncodeReady();
        };
        objReader.readAsText(objFile);
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
        fnCheckEncodeReady();
    });
}

function fnSetupDropZone(strSelector, fnOnFileHandler) 
{
    var objDropZone = $(strSelector);
    
    objDropZone.on("dragover", function(objEvent) 
    {
        objEvent.preventDefault();
        $(this).addClass("dragover");
    });
    
    objDropZone.on("dragleave", function(objEvent) 
    {
        objEvent.preventDefault();
        $(this).removeClass("dragover");
    });
    
    objDropZone.on("drop", function(objEvent) 
    {
        objEvent.preventDefault();
        $(this).removeClass("dragover");
        
        var arrFiles = objEvent.originalEvent.dataTransfer.files;
        if (arrFiles.length > 0) 
        {
            fnOnFileHandler(arrFiles[0]);
        }
    });
}

function fnSetupButtons() 
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
        
        fnCheckEncodeReady();
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
       
        var arrAddresses = toZOSCII(g_arrRomData, g_strTextData, arrMemoryBlocks, fnConverter, 42);
        
        g_objEncodingResult = {
            success: true,
            addresses: arrAddresses,
            originalText: g_strTextData,
            encoding: strEncoding,
            addressCount: arrAddresses.length
        };
        
        fnDisplayAnalysis(g_objEncodingResult);
        $("#analysis-tab").click();
        $(this).prop("disabled", false).text("Encode");
    });
}

function fnCheckEncodeReady() 
{
    var blnReady = g_arrRomData && g_strTextData && g_strTextData.length > 0;
    $("#encodeBtn").prop("disabled", !blnReady);
}

function fnDisplayAnalysis(objResult) 
{
    var strHtml = "<h3>Encoding Results</h3>";
    strHtml += "<div class='row'>";
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Input Information</h5>";
    strHtml += "<table class='table table-sm analysis-table'>";
    strHtml += "<tr><td><strong>Original Text:</strong></td><td>" + fnEscapeHtml(objResult.originalText) + "</td></tr>";
    strHtml += "<tr><td><strong>Text Length:</strong></td><td>" + objResult.originalText.length + " characters</td></tr>";
    strHtml += "<tr><td><strong>Encoding:</strong></td><td>" + objResult.encoding.toUpperCase() + "</td></tr>";
    strHtml += "<tr><td><strong>Addresses Generated:</strong></td><td>" + objResult.addressCount + "</td></tr>";
    strHtml += "</table>";
    strHtml += "</div>";
    
    strHtml += "<div class='col-md-6'>";
    strHtml += "<h5>Address List</h5>";
    strHtml += "<div style='max-height: 200px; overflow-y: auto;'>";
    strHtml += "<table class='table table-sm analysis-table'>";
    strHtml += "<thead><tr><th>Char</th><th>Address (Hex)</th><th>Address (Dec)</th></tr></thead>";
    strHtml += "<tbody>";
    
    for (var intI = 0; intI < objResult.addresses.length && intI < objResult.originalText.length; intI++) 
    {
        var strChar = objResult.originalText.charAt(intI);
        var intAddr = objResult.addresses[intI];
        strHtml += "<tr>";
        strHtml += "<td>" + fnEscapeHtml(strChar) + "</td>";
        strHtml += "<td>0x" + intAddr.toString(16).toUpperCase() + "</td>";
        strHtml += "<td>" + intAddr + "</td>";
        strHtml += "</tr>";
    }
    
    strHtml += "</tbody></table>";
    strHtml += "</div>";
    strHtml += "</div>";
    strHtml += "</div>";
    
    strHtml += "<div class='mt-4'>";
    strHtml += "<h5>Generate Viewer</h5>";
    strHtml += "<p>Create a standalone viewer file that can decode this ZOSCII data:</p>";
    strHtml += "<button class='btn btn-success' id='generateViewerBtn'>Generate Viewer File</button>";
    strHtml += "<div id='viewerResult' class='mt-3'></div>";
    strHtml += "</div>";
    
    $("#analysisContent").html(strHtml);
    
    // Setup generate viewer button
	$("#generateViewerBtn").click(function() 
	{
		$(this).prop("disabled", true).text("Generating...");
		
		// Create binary file with addresses
		var arrAddressBytes = new Uint8Array(g_objEncodingResult.addresses.length * 4);
		for (var intI = 0; intI < g_objEncodingResult.addresses.length; intI++)
		{
			var intAddress = g_objEncodingResult.addresses[intI];
			arrAddressBytes[intI * 4] = intAddress & 0xFF;
			arrAddressBytes[intI * 4 + 1] = (intAddress >> 8) & 0xFF;
			arrAddressBytes[intI * 4 + 2] = (intAddress >> 16) & 0xFF;
			arrAddressBytes[intI * 4 + 3] = (intAddress >> 24) & 0xFF;
		}
		
		// Download only the binary address file
		fnDownloadFile(arrAddressBytes, 'zoscii_addresses_' + new Date().getTime() + '.bin', 'application/octet-stream');
		
		var strResultHtml = "<div class='alert alert-success'>";
		strResultHtml += "<strong>Address file generated successfully!</strong><br>";
		strResultHtml += "Binary address file downloaded. Use with separate ZOSCII viewer.";
		strResultHtml += "</div>";
		$("#viewerResult").html(strResultHtml);
		
		$(this).prop("disabled", false).text("Generate Address File");
	});
}

function fnDownloadFile(data, strFilename, strMimeType) 
{
    var objBlob = new Blob([data], {type: strMimeType});
    var objUrl = URL.createObjectURL(objBlob);
    var objLink = document.createElement('a');
    objLink.href = objUrl;
    objLink.download = strFilename;
    document.body.appendChild(objLink);
    objLink.click();
    document.body.removeChild(objLink);
    URL.revokeObjectURL(objUrl);
}

function fnEscapeHtml(strText) 
{
    var objDiv = document.createElement('div');
    objDiv.textContent = strText;
    return objDiv.innerHTML;
}
