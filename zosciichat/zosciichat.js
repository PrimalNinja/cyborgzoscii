var g_arrDecodedMsgs = [];
var g_arrMessageMeta = [];
var g_arrUserRom = null;
var g_intCurrentPage = 1;
var g_intPageSize = 10;
var g_objCharMap = {};
var g_objLastAddrFile = null;
var g_strServerEndpoint = '';
var g_strChannelNumber = '1';

var g_orwellInspiredQuotes = [
	"The truth becomes whatever those in power declare it to be.",
	"In a world of constant surveillance, privacy becomes rebellion.",
	"When words lose their meaning, thought becomes impossible.",
	"The past is malleable when those who control it rewrite history.",
	"Freedom is the right to tell people what they do not want to hear.",
	"In times of universal deceit, speaking truth becomes a revolutionary act.",
	"The most effective way to destroy people is to deny their understanding of history.",
	"Power corrupts, but absolute power creates its own reality.",
	"When everyone is watched, no one is truly free.",
	"The manipulation of language is the manipulation of thought itself.",
	"In a surveillance state, paranoia becomes rational behavior.",
	"The greatest enemy of clear thinking is the illusion of knowledge.",
	"When the government controls information, it controls the people.",
	"Rebellion begins with a single thought kept secret.",
	"The price of freedom is eternal vigilance against those who would take it.",
	"In a world without privacy, love becomes an act of defiance.",
	"Those who control the present control the past and future.",
	"The destruction of words is the destruction of ideas.",
	"Power seeks to make the abnormal seem normal.",
	"When questioning authority becomes criminal, authority has become criminal.",
	"The most dangerous person to a tyrant is someone who can think for themselves.",
	"In totalitarian systems, loyalty is measured by blind obedience.",
	"The individual mind is the last refuge of freedom.",
	"When truth becomes treason, the state has lost its legitimacy.",
	"The constant rewriting of history makes all knowledge uncertain.",
	"Fear is the most powerful tool of oppression.",
	"In a monitored society, solitude becomes precious and rare.",
	"The corruption of language leads to the corruption of thought.",
	"When everyone must conform, authenticity becomes rebellion.",
	"The state's greatest victory is making citizens police themselves.",
	"In the absence of truth, propaganda fills the void.",
	"The most insidious tyranny is the one that claims to protect you.",
	"When the past is erased, the future can be rewritten at will.",
	"Dissent is the highest form of patriotism in a free society.",
	"The price of comfort is often the surrender of liberty.",
	"In a world of lies, the truth-teller becomes an outlaw.",
	"Power maintains itself by keeping people ignorant and divided.",
	"The destruction of memory is the destruction of identity.",
	"When authority cannot be questioned, it ceases to be legitimate.",
	"The individual conscience is the last check on absolute power.",
	"In surveillance states, everyone becomes both watched and watcher.",
	"The manipulation of facts creates a malleable reality.",
	"When freedom of thought dies, humanity dies with it.",
	"The state's ideal citizen thinks only what they are told to think.",
	"In the war against truth, the first casualty is language itself.",
	"The most dangerous book is the one that makes you question everything.",
	"When privacy disappears, so does the possibility of dissent.",
	"The rewriting of history is the engineering of the future.",
	"In totalitarian systems, love of truth becomes a form of mental illness.",
	"The ultimate goal of tyranny is to make resistance inconceivable."
];

// helpers

function buildCharacterMap()
{
	g_objCharMap = {};
	
	for (var i = 0; i < g_arrUserRom.length; i++)
	{
		var charCode = g_arrUserRom[i];
		if (!g_objCharMap[charCode])
		{
			g_objCharMap[charCode] = [];
		}
		g_objCharMap[charCode].push(i);
	}
	
	console.log("Character map built:", Object.keys(g_objCharMap).length, "unique characters");
}

function decodeMessagesForPage()
{
	console.log("Decoding messages for page, ROM loaded:", g_arrUserRom !== null);
	console.log("Messages to decode:", g_arrMessageMeta.length);
	
	g_arrDecodedMsgs = [];
	var arrToDecode = g_arrMessageMeta.slice(0, g_intPageSize);
	var intDecoded = 0;
	
	if (arrToDecode.length === 0)
	{
		console.log("No messages to decode");
		renderMessages([], g_intCurrentPage);
		return;
	}
	
	var intI;
	for (intI = 0; intI < arrToDecode.length; intI++)
	{
		(function(objMeta_a)
		{
			makeAjaxRequest(g_strServerEndpoint + '/index.php?channel=' + g_strChannelNumber + '&file=' + objMeta_a.binFilename,
				{method: 'GET', responseType: 'arraybuffer'},
				function(objData_a)
				{
					var arrRaw = new Uint8Array(objData_a);
					var arrAddresses = [];
					var intJ;
					
					for (intJ = 0; intJ + 1 < arrRaw.length; intJ += 2)
					{
						arrAddresses.push(arrRaw[intJ] | (arrRaw[intJ + 1] << 8));
					}
					
					var strMessage = "";
					for (intJ = 0; intJ < arrAddresses.length; intJ++)
					{
						var intAddress = arrAddresses[intJ];
						if (g_arrUserRom && intAddress < g_arrUserRom.length)
						{
							strMessage += String.fromCharCode(g_arrUserRom[intAddress]);
						}
					}

					if (validateFile(strMessage, 0.9))
					{
						g_arrDecodedMsgs.push({message: strMessage, meta: objMeta_a});
					}
					else
					{
						strMessage = getRandomQuote();
						g_arrDecodedMsgs.push({message: strMessage, meta: objMeta_a});
					}
					
					intDecoded++;
					if (intDecoded === arrToDecode.length)
					{
						g_arrDecodedMsgs.sort(function(a, b)
						{
							return (b.meta.date || 0) - (a.meta.date || 0);
						});
						
						renderMessages(g_arrDecodedMsgs, g_intCurrentPage);
					}
				},
				function(strError_a)
				{
					console.error("Failed to fetch message:", strError_a);
				}
			);
		})(arrToDecode[intI]);
	}
}

function getRandomAddressForChar(charCode_a)
{
	if (!g_objCharMap[charCode_a] || g_objCharMap[charCode_a].length === 0)
	{
		return -1;
	}
	
	var arrAddresses = g_objCharMap[charCode_a];
	var intRandomIndex = Math.floor(Math.random() * arrAddresses.length);
	return arrAddresses[intRandomIndex];
}

function getRandomQuote() 
{
  var intRandomIndex = Math.floor(Math.random() * g_orwellInspiredQuotes.length);
  return g_orwellInspiredQuotes[intRandomIndex];
}

function showDropZone()
{
	document.getElementById("divMessageEntry").style.display = "none";
	document.getElementById("divMessageList").style.display = "none";
	document.getElementById("divPagination").style.display = "none";
	document.getElementById("txtChannelNumber").style.display = "none";

	document.getElementById("txtServerUrl").style.display = "block";
	document.getElementById("divLanding").style.display = "block";
}

function showMessages()
{
	document.getElementById("divLanding").style.display = "none";
	document.getElementById("txtServerUrl").style.display = "none";

	document.getElementById("divMessageEntry").style.display = "block";
	document.getElementById("divMessageList").style.display = "block";
	document.getElementById("divPagination").style.display = "block";
	document.getElementById("txtChannelNumber").style.display = "block";
	document.getElementById("txtMessage").focus();
}

function validateFile(strDecoded_a, floatMinPercentage_a)
{
	if (strDecoded_a.length === 0) return false;
	
	var intVisibleAsciiCount = 0;
	var intI;
	
	for (intI = 0; intI < strDecoded_a.length; intI++)
	{
		var intCharCode = strDecoded_a.charCodeAt(intI);
		// Visible ASCII characters are 32-126 (space through tilde)
		if (intCharCode >= 32 && intCharCode <= 126)
		{
			intVisibleAsciiCount++;
		}
	}
	
	var floatActualPercentage = intVisibleAsciiCount / strDecoded_a.length;
	return floatActualPercentage >= floatMinPercentage_a;
}

// rendering

function decodeAndShow(objAddrFile_a)
{
	var objReader = new FileReader();
	
	objReader.onload = function(e)
	{
		var arrRaw = new Uint8Array(e.target.result);
		var arrAddresses = [];
		var intI;
		
		for (intI = 0; intI + 1 < arrRaw.length; intI += 2)
		{
			arrAddresses.push(arrRaw[intI] | (arrRaw[intI + 1] << 8));
		}
		
		var intJ;
		var strDecoded = "";

		for (intJ = 0; intJ < arrAddresses.length; intJ++)
		{
			var addr = arrAddresses[intJ];
			if (g_arrUserRom && addr < g_arrUserRom.length)
			{
				strDecoded += String.fromCharCode(g_arrUserRom[addr]);
			}
		}

		// if (!validateFile(strDecoded, 0.9))
		// {
			// alert("This message does not look valid (it must have at least one character that appears three times).");
			// showDropZone();
			// return;
		// }

		renderMessages([{ 
			message: strDecoded,
			meta: {filename: objAddrFile_a.name}
		}], 1);
	};
	
	objReader.readAsArrayBuffer(objAddrFile_a);
	showMessages();
}

function renderMessages(arrList_a, intPage_a)
{
	var objDiv;
	var objMessageList = document.getElementById("divMessageList");
	objMessageList.innerHTML = "";
	
	if (arrList_a.length === 0)
	{
		objDiv = document.createElement("div");
		objDiv.className = "msg-bubble";
		objDiv.textContent = "No decodable messages for your ROM.";
		objMessageList.appendChild(objDiv);
	}
	else
	{
		for (var intI = 0; intI < arrList_a.length; intI++)
		{
			if (intI >= g_intPageSize)
			{
				break;
			}
			else
			{
				objDiv = document.createElement("div");
				objDiv.className = "msg-bubble";
				objDiv.textContent = arrList_a[intI].message;
				objMessageList.appendChild(objDiv);
			}
		}
	}
	
	document.getElementById("divMessageList").style.display = "block";
	document.getElementById("divPagination").style.display = "block";
	document.getElementById("spanPageInfo").textContent = "Page " + intPage_a;
	document.getElementById("btnPrev").disabled = intPage_a <= 1;
	document.getElementById("btnNext").disabled = g_arrMessageMeta.length < g_intPageSize;
	
	showMessages();
}

// webservices

function fetchMessageMetas(intPage_a)
{
    console.log("Fetching messages for page:", intPage_a);
    
    getJSON(g_strServerEndpoint + "/index.php?channel=" + g_strChannelNumber + "&limit=" + g_intPageSize + "&page=" + intPage_a, 
        function(arrMeta_a) 
        {
            console.log("Fetched metadata:", arrMeta_a);
            console.log("Number of messages:", arrMeta_a.length);
            
            if (arrMeta_a.length === 0)
            {
                console.log("No messages found");
                renderMessages([], g_intCurrentPage);
                return;
            }
            
            g_arrMessageMeta = arrMeta_a;
            decodeMessagesForPage();
        }, 
        function(strError_a) 
        {
            console.error("Failed to fetch messages:", strError_a);
            alert("Failed to load messages: " + strError_a + "\nCheck console for details");
            showDropZone();
        }
    );
}

function getJSON(strUrl_a, cbSuccess_a, cbError_a) 
{
    makeAjaxRequest(strUrl_a, {}, function(strResponse_a) 
    {
        var arrParsed = JSON.parse(strResponse_a);
        cbSuccess_a(arrParsed);
    }, cbError_a);
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

// events

document.getElementById("txtServerUrl").addEventListener("input", function()
{
    g_strServerEndpoint = this.value.trim();
});

document.getElementById("btnRefresh").addEventListener("click", function()
{
	g_strChannelNumber = document.getElementById("txtChannelNumber").value.trim();
	fetchMessageMetas(g_intCurrentPage);
});

document.getElementById("btnSubmit").addEventListener("click", function()
{
	var intAddress;
	var intI;
	var strMessage = document.getElementById("txtMessage").value.trim();
	
	if (strMessage.length === 0)
	{
		alert("Please enter a message");
		return;
	}
	
	if (!g_arrUserRom)
	{
		alert("ROM not loaded");
		return;
	}
	
	var arrAddresses = [];
	for (intI = 0; intI < strMessage.length; intI++)
	{
		var strCharCode = strMessage.charCodeAt(intI);
		intAddress = getRandomAddressForChar(strCharCode);
		if (intAddress !== -1)
		{
			arrAddresses.push(intAddress);
		}
	}
	
	if (arrAddresses.length === 0)
	{
		alert("No characters from your message were found in the ROM");
		return;
	}
	
	g_strChannelNumber = document.getElementById("txtChannelNumber").value.trim();
	
	var arrBinary = new Uint8Array(arrAddresses.length * 2);
	for (intI = 0; intI < arrAddresses.length; intI++)
	{
		intAddress = arrAddresses[intI];
		arrBinary[intI * 2] = intAddress & 0xFF;
		arrBinary[intI * 2 + 1] = (intAddress >> 8) & 0xFF;
	}
	
	var objBlob = new Blob([arrBinary], {type: 'application/octet-stream'});
	var objFormData = new FormData();
	objFormData.append('addressfile', objBlob, 'message.bin');
	objFormData.append('channel', g_strChannelNumber);
	
	makeAjaxRequest(g_strServerEndpoint + "/index.php", 
		{method: 'POST', data: objFormData},
		function(objResponse_a)
		{
			console.log("Message submitted:", objResponse_a);
			document.getElementById("txtMessage").value = '';
			fetchMessageMetas(g_intCurrentPage);
		},
		function(strError_a)
		{
			console.error("Upload failed:", strError_a);
			alert("Failed to submit message: " + strError_a);
		}
	);
});

// Click to browse functionality
document.getElementById("divDropZone").addEventListener("click", function()
{
    document.getElementById("fileInput").click();
});

// Handle file selection from browse dialog
document.getElementById("fileInput").addEventListener("change", function(objEvent_a)
{
    var arrFiles = objEvent_a.target.files;
    if (arrFiles.length > 0)
    {
        var objFile = arrFiles[0];
        
        var objReader = new FileReader();
        objReader.onload = function(e)
        {
            g_arrUserRom = new Uint8Array(e.target.result);

            if (g_arrUserRom.length > 65536) 
            {
                g_arrUserRom = g_arrUserRom.slice(0, 65536);
            }
            
            buildCharacterMap();
            
            g_intCurrentPage = 1;
            fetchMessageMetas(g_intCurrentPage);
        };
        objReader.readAsArrayBuffer(objFile);
    }
});

document.getElementById("divDropZone").addEventListener("dragover", function(objEvent_a)
{
	objEvent_a.preventDefault();
	objEvent_a.target.classList.add("dragover");
});

document.getElementById("divDropZone").addEventListener("dragleave", function(objEvent_a)
{
	objEvent_a.preventDefault();
	objEvent_a.target.classList.remove("dragover");
});

document.getElementById("divDropZone").addEventListener("drop", function(objEvent_a)
{
	objEvent_a.preventDefault();
	objEvent_a.target.classList.remove("dragover");
	
	var arrFiles = objEvent_a.dataTransfer.files;
	if (arrFiles.length > 0)
	{
		var objFile = arrFiles[0];
		
		var objReader = new FileReader();
		objReader.onload = function(e)
		{
			g_arrUserRom = new Uint8Array(e.target.result);

			if (g_arrUserRom.length > 65536) 
			{
				g_arrUserRom = g_arrUserRom.slice(0, 65536);
			}
			
			buildCharacterMap();
			
			g_intCurrentPage = 1;
			fetchMessageMetas(g_intCurrentPage);
		};
		objReader.readAsArrayBuffer(objFile);
	}
});

document.getElementById("btnNext").addEventListener("click", function()
{
	g_intCurrentPage++;
	fetchMessageMetas(g_intCurrentPage);
});

document.getElementById("btnPrev").addEventListener("click", function()
{
	if (g_intCurrentPage > 1)
	{
		g_intCurrentPage--;
		fetchMessageMetas(g_intCurrentPage);
	}
});

showDropZone();
