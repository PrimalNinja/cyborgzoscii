
var g_arrDecodedMsgs = [];
var g_arrMessageMeta = [];
var g_arrUserRom = null;
var g_intCurrentPage = 1;
var g_intPageSize = 10;
var g_objCharMap = {};
var g_objLastAddrFile = null;

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
			$.ajax(
			{
				url: 'messages/' + objMeta_a.binFilename,
				method: 'GET',
				xhrFields: {responseType: 'arraybuffer'},
				success: function(objData_a)
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
						
//alert(g_arrMessageMeta.length +":"+ g_intPageSize);
						renderMessages(g_arrDecodedMsgs, g_intCurrentPage);
					}
				}
			});
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
	$("#divMessageEntry").hide();
	$("#divMessageList").hide();
	$("#divPagination").hide();
	$("#divDropZone").show();
}

function showMessages()
{
	$("#divDropZone").hide();
	$("#divMessageEntry").show();
	$("#divMessageList").show();
	$("#divPagination").show();
	$("#txtMessage").focus();
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
	var blnNext = false;
	var objMessageList = $("#divMessageList").empty();
	
	if (arrList_a.length === 0)
	{
		objMessageList.append($("<div>").addClass("msg-bubble").text("No decodable messages for your ROM."));
	}
	else
	{
		for (var intI = 0; intI < arrList_a.length; intI++)
		{
			if (intI >= g_intPageSize)
			{
				blnNext = true;
			}
			else
			{
				objMessageList.append($("<div>").addClass("msg-bubble").text(arrList_a[intI].message));
			}
		}
	}
	
	$("#divMessageList").show();
	$("#divPagination").show();
	$("#spanPageInfo").text("Page " + intPage_a);
	$("#btnPrev").prop("disabled", intPage_a <= 1);
	$("#btnNext").prop("disabled", g_arrMessageMeta.length < g_intPageSize);
	
	showMessages();
}

// webservices

function fetchMessageMetas(intPage_a)
{
	console.log("Fetching messages for page:", intPage_a);
	
	$.getJSON("messages.php?limit=" + g_intPageSize + "&page=" + intPage_a).done(function(arrMeta_a)
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
	}).fail(function(jqXHR, textStatus, errorThrown)
	{
		console.error("Failed to fetch messages:", textStatus, errorThrown);
		console.error("Response:", jqXHR.responseText);
		alert("Failed to load messages: " + textStatus + "\nCheck console for details");
		showDropZone();
	});
}

// events

$("#btnSubmit").click(function()
{
	var intAddress;
	var intI;
	var strMessage = $("#txtMessage").val().trim();
	
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
	
	// Convert message to address file format using random addresses
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
	
	// Create binary data (16-bit little-endian)
	var arrBinary = new Uint8Array(arrAddresses.length * 2);
	for (intI = 0; intI < arrAddresses.length; intI++)
	{
		intAddress = arrAddresses[intI];
		arrBinary[intI * 2] = intAddress & 0xFF;
		arrBinary[intI * 2 + 1] = (intAddress >> 8) & 0xFF;
	}
	
	// Upload the binary data
	var objBlob = new Blob([arrBinary], {type: 'application/octet-stream'});
	var objFormData = new FormData();
	objFormData.append('addressfile', objBlob, 'message.bin');
	
	$.ajax({
		url: 'messages.php',
		type: 'POST',
		data: objFormData,
		processData: false,
		contentType: false,
		success: function(objResponse_a)
		{
			console.log("Message submitted:", objResponse_a);
			$("#txtMessage").val('');
			fetchMessageMetas(g_intCurrentPage);
		},
		error: function(xhr, status, error)
		{
			console.error("Upload failed:", error);
			alert("Failed to submit message: " + error);
		}
	});
});

$("#divDropZone").on("dragover", function(objEvent_a)
{
	objEvent_a.preventDefault();
	$(this).addClass("dragover");
});

$("#divDropZone").on("dragleave", function(objEvent_a)
{
	objEvent_a.preventDefault();
	$(this).removeClass("dragover");
});

$("#divDropZone").on("drop", function(objEvent_a)
{
	objEvent_a.preventDefault();
	$(this).removeClass("dragover");
	
	var arrFiles = objEvent_a.originalEvent.dataTransfer.files;
	if (arrFiles.length > 0)
	{
		var objFile = arrFiles[0];
		
		// Load ROM file
		var objReader = new FileReader();
		objReader.onload = function(e)
		{
			g_arrUserRom = new Uint8Array(e.target.result);
			
			buildCharacterMap();
			
			// Now that ROM is loaded, fetch and decode messages from web service
			g_intCurrentPage = 1;
			fetchMessageMetas(g_intCurrentPage);
		};
		objReader.readAsArrayBuffer(objFile);
	}
});


$("#btnNext").click(function()
{
	g_intCurrentPage++;
	fetchMessageMetas(g_intCurrentPage);
});

$("#btnPrev").click(function()
{
	if (g_intCurrentPage > 1)
	{
		g_intCurrentPage--;
		fetchMessageMetas(g_intCurrentPage);
	}
});

showDropZone();
