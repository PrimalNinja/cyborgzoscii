// options:
// 		callbacks:
//			onStart, onStop, onPrevious, onNext, onToggle(<pausedflag>)
//		booleans:
//			clearImageAfterSong
//			clearTextAfterImage
//			clearTextAfterSong
//		imageDelay:	seconds between image changes if a song isn't playing
//		maxTextLength: maximum number of text characters to display
function ZOSCIIRadioPlayer(strComponentID_a, objOptions_a) 
{
	var m_objThis = this;
    var m_strComponentID = strComponentID_a;
	
	var m_MINTRACKSIZE = 1024000;

	var m_arrOscBars = [];
	var m_arrProgressSquares = [];
	var m_blnPaused = false;
	var m_intProgressSquares = 30;
	var m_objAnalyser = null;
	var m_objAudioContext = null;
	var m_objSourceNode = null;
	var m_strPreviousType = '';
    var m_arrHistoryStack = [];
    var m_arrQueues = [];
    var m_arrROM = null;
    var m_blnFetchNextRunning = false;
    var m_blnFirstTrack = true;
    var m_blnPlaying = false;
    var m_intCurrentQueueIndex = 0;
    var m_intTrackCounter = 0;
    var m_intWaitCountdown = 0;
    var m_objFetchNextTimer = null;
    var m_objOptions = objOptions_a || {};
    var m_objPlayer;
    var m_objWaitCountdownTimer = null;
    var m_strBehavior = 'wait';
    var m_strCurrentPointer = '';
    var m_strQueueURL = '';
	
	// defaults
	m_objOptions.clearImageAfterSong = m_objOptions.clearImageAfterSong || false;
	m_objOptions.clearTextAfterImage = m_objOptions.clearTextAfterImage || false;
	m_objOptions.clearTextAfterSong = m_objOptions.clearTextAfterSong || false;
	m_objOptions.imageDelay = m_objOptions.imageDelay || 10;
	m_objOptions.maxTextLength = m_objOptions.maxTextLength || 16384;

	// helpers
    function addToPlayLog(strFilename_a, blnDecoded_a) 
	{
        m_intTrackCounter++;
        var objLog = element(m_strComponentID, 'gePlayLog');
        var strEntry = m_intTrackCounter + '. ' + strFilename_a + (blnDecoded_a ? ' (decoded)' : '') + '\n';
        objLog.html(objLog.html() + strEntry);
        //objLog[0].scrollTop = objLog[0].scrollHeight;
    }
	
	function animateOscilloscope() 
	{
		if (m_objAnalyser)
		{
			requestAnimationFrame(animateOscilloscope);
			var arrDataArray = new Uint8Array(m_objAnalyser.frequencyBinCount);
			m_objAnalyser.getByteFrequencyData(arrDataArray);
			var objContainer = element(m_strComponentID, 'geOscilloscope');

			for (var intI = 0; intI < m_arrOscBars.length; intI++) 
			{
				var intHeight = ((arrDataArray[intI] / 255) * objContainer.height()) + 4;
				m_arrOscBars[intI].css('height', intHeight + 'px');
			}
		}
	}

	function decodeUTF8(arrBytes) 
	{
		var strResult = '';
		var intI = 0;
		
		while (intI < arrBytes.length) 
		{
			var intByte = arrBytes[intI++];
			
			if (intByte < 0x80) 
			{
				// Single-byte character (ASCII)
				strResult += String.fromCharCode(intByte);
			} 
			else if (intByte < 0xE0) 
			{
				// Two-byte character
				strResult += String.fromCharCode(((intByte & 0x1F) << 6) | (arrBytes[intI++] & 0x3F));
			} 
			else if (intByte < 0xF0) 
			{
				// Three-byte character
				strResult += String.fromCharCode(((intByte & 0x0F) << 12) | ((arrBytes[intI++] & 0x3F) << 6) | (arrBytes[intI++] & 0x3F));
			} 
			else 
			{
				// Four-byte character (needs surrogate pairs)
				var intCodePoint = ((intByte & 0x07) << 18) | ((arrBytes[intI++] & 0x3F) << 12) | ((arrBytes[intI++] & 0x3F) << 6) | (arrBytes[intI++] & 0x3F);
				intCodePoint -= 0x10000;
				strResult += String.fromCharCode(0xD800 + (intCodePoint >> 10), 0xDC00 + (intCodePoint & 0x3FF));
			}
		}
		
		return strResult;
	}

	function element(strScope_a, strClass_a)
	{
		return $('.' + strClass_a, '#' + strScope_a);
	}
	
	function getID3v1Info(binData_a) 
	{
		var objInfo = {
			title: 'Unknown',
			artist: 'Unknown',
			album: 'Unknown',
			year: 'Unknown',
			comment: 'Unknown',
			genre: 'Unknown'
		};

		if (binData_a && binData_a.length >= 128) 
		{
			var arrID3 = binData_a.slice(binData_a.length - 128);
			var strHeader = String.fromCharCode.apply(null, arrID3.slice(0,3));
			if (strHeader === 'TAG') 
			{
				// use latin1 decoding instead of naive fromCharCode
				function latin1(arr_a) 
				{ 
					return String.fromCharCode.apply(null, arr_a).replace(/\0/g,'').trim(); 
				}

				objInfo.title   = latin1(arrID3.slice(3, 33));
				objInfo.artist  = latin1(arrID3.slice(33, 63));
				objInfo.album   = latin1(arrID3.slice(63, 93));
				objInfo.year    = latin1(arrID3.slice(93, 97));
				objInfo.comment = latin1(arrID3.slice(97, 127));
				objInfo.genre   = arrID3[127]; // optional: keep as numeric index
			}
		}

		return objInfo;
	}

	function initialiseOscilloscope() 
	{
		var objContainer = element(m_strComponentID, 'geOscilloscope');
		objContainer.html('');

		var intBarCount = 32;
		m_arrOscBars = [];
		for (var intI = 0; intI < intBarCount; intI++) 
		{
			var objBar = $('<div class="gsOscBar"></div>');
			objContainer.append(objBar);
			m_arrOscBars.push(objBar);
		}
	}

	function initialiseProgressBar() 
	{
		var objProgressBar = element(m_strComponentID, 'geProgressBar');
		objProgressBar.html(''); // clear previous

		m_arrProgressSquares = []; // reset array
		for (var intI = 0; intI < m_intProgressSquares; intI++)
		{
			var objSquare = $('<div class="gsProgressSquare"></div>');
			objSquare.attr('data-counter', intI);
			objProgressBar.append(objSquare);
			m_arrProgressSquares.push(objSquare);
		}
	}

	function isFunction(fn_a)
	{
		return $.isFunction(fn_a);
	}

	function htmlEncode(str_a) 
	{
		return String(str_a || '').replace(/&/g, '&amp;')
								.replace(/</g, '&lt;')
								.replace(/>/g, '&gt;')
								.replace(/"/g, '&quot;')
								.replace(/'/g, '&#39;');
	}

	function latin1(arr_a) 
	{
		return String.fromCharCode.apply(null, arr_a).replace(/\0/g, '').trim();
	}

	function setupAudioContext()
	{
		if (!m_objAudioContext) 
		{
			m_objAudioContext = new (window.AudioContext || window.webkitAudioContext)();
			m_objAnalyser = m_objAudioContext.createAnalyser();
			m_objAnalyser.fftSize = 64;
			m_objSourceNode = m_objAudioContext.createMediaElementSource(m_objPlayer);
			m_objSourceNode.connect(m_objAnalyser);
			m_objAnalyser.connect(m_objAudioContext.destination);
			animateOscilloscope();
		}
	}

	function updateNowPlaying(arrDecodedData_a) 
	{
		var objID3Info = getID3v1Info(arrDecodedData_a);
		var strTrackInfo = 'Playing ' + htmlEncode(objID3Info.title);
		if (objID3Info.artist) 
		{
			strTrackInfo += ' - ' + htmlEncode(objID3Info.artist);
		}
		
		if (objID3Info.album || objID3Info.year) 
		{
			strTrackInfo += ' [' + htmlEncode(objID3Info.album);
			if (objID3Info.album && objID3Info.year)
			{
				strTrackInfo += ' | ';
			}
			strTrackInfo += htmlEncode(objID3Info.year) + ']';
		}
		
		element(m_strComponentID, 'geNowPlaying').html(strTrackInfo);
	}

	function updateProgress(intPercent_a) 
	{
		var intSquaresToLight = Math.floor((intPercent_a / 100) * m_intProgressSquares);
		for (var intI = 0; intI < m_intProgressSquares; intI++) 
		{
			if (intI < intSquaresToLight) 
			{
				m_arrProgressSquares[intI].addClass('active');
			} 
			else 
			{
				m_arrProgressSquares[intI].removeClass('active');
			}
		}
	}

    function updateStatus(strMessage_a) 
	{
        element(m_strComponentID, 'geStatus').html(strMessage_a);
    }

	function zosciiDecode(arrROM_a, arrZOSBinary_a) 
	{
		var intDecodedLength = arrZOSBinary_a.length / 2;
		var arrResult = new Uint8Array(intDecodedLength);

		for (var intI = 0; intI < intDecodedLength; intI++) 
		{
			var intAddr = arrZOSBinary_a[intI * 2] | (arrZOSBinary_a[intI * 2 + 1] << 8);
			arrResult[intI] = arrROM_a[intAddr];
		}

		return arrResult;
	}

	// initialisation
    function initialise() 
	{
        // Get player element reference
        m_objPlayer = element(m_strComponentID, 'gePlayer')[0];

        // Bind player events
        m_objPlayer.onended = onPlayerEnded;
        m_objPlayer.onerror = onPlayerError;
        m_objPlayer.addEventListener('error', onPlayerFormatError, false);
		
		initialiseProgressBar();
		initialiseOscilloscope();

        // Bind button events
        element(m_strComponentID, 'geStartButton').on('click', StartPlayer_onClick);
        element(m_strComponentID, 'geStopButton').on('click', StopPlayer_onClick);
        element(m_strComponentID, 'geToggleButton').on('click', Toggle_onClick);
        element(m_strComponentID, 'gePreviousButton').on('click', PreviousTrack_onClick);
        element(m_strComponentID, 'geNextButton').on('click', NextTrack_onClick);
		element(m_strComponentID, 'gsProgressSquare').on('click', ProgressBar_onClick);

		m_objPlayer.onplay = function() 
		{
			if (m_objPlayer.audioContext) 
			{
				m_objPlayer.audioContext.resume();
			}
		};
    }
	
	// handlers
	function identifyType(arrDecodedData_a)
	{
		var strType = 'text';
		
		// JPEG: FF D8 FF
		if (arrDecodedData_a[0] === 0xFF && arrDecodedData_a[1] === 0xD8 && arrDecodedData_a[2] === 0xFF)
		{
			strType = 'jpg';
		}
		
		// MP3: FF FB or FF F3 (MPEG sync) or ID3 tag (49 44 33)
		if ((arrDecodedData_a[0] === 0xFF && (arrDecodedData_a[1] === 0xFB || arrDecodedData_a[1] === 0xF3)) ||
			(arrDecodedData_a[0] === 0x49 && arrDecodedData_a[1] === 0x44 && arrDecodedData_a[2] === 0x33))
		{
			strType = 'mp3';
		}
		
		return strType;
	}
	
	// Play MP3
	function playMP3(strType_a, strFilename_a, arrDecodedData_a) 
	{
        if (m_blnFirstTrack) 
		{
            m_arrHistoryStack.push({ queue: m_arrQueues[m_intCurrentQueueIndex], type: '', pointer: '' });
            m_blnFirstTrack = false;
        }

		if (arrDecodedData_a.length > m_MINTRACKSIZE)
		{
			m_arrHistoryStack.push({
				queue: m_arrQueues[m_intCurrentQueueIndex],
				type: strType_a, 
				pointer: m_strCurrentPointer
			});
		}

		playTrack(strFilename_a, arrDecodedData_a, new Blob([arrDecodedData_a], {type:'audio/mpeg'}), true);
	}

	// Display JPEG
	function showJPG(strType_a, strFilename_a, arrDecodedData_a) 
	{
		var objBlob = new Blob([arrDecodedData_a], {type: 'image/jpeg'});
		var strImageURL = URL.createObjectURL(objBlob);
		
        // if (m_blnFirstTrack) 
		// {
            // m_arrHistoryStack.push({ queue: m_arrQueues[m_intCurrentQueueIndex], type: '', pointer: '' });
            // m_blnFirstTrack = false;
        // }

        // m_arrHistoryStack.push({
            // queue: m_arrQueues[m_intCurrentQueueIndex],
			// type: strType_a, 
            // pointer: m_strCurrentPointer
        // });

		if (m_objOptions.clearTextAfterImage) 
		{
			element(m_strComponentID, 'geText').html('');
			element(m_strComponentID, 'geText').hide();
		}

		element(m_strComponentID, 'geImage').html('<img src="' + strImageURL + '" style="max-width:100%;">');
		updateNowPlaying(arrDecodedData_a);
		updateStatus('Displaying ' + strFilename_a);
		addToPlayLog(strFilename_a, true);
		
		scheduleNext();
	}

	// Display text file
	function showText(strType_a, strFilename_a, arrDecodedData_a) 
	{
		// if (m_blnFirstTrack) 
		// {
			// m_arrHistoryStack.push({ queue: m_arrQueues[m_intCurrentQueueIndex], type: '', pointer: '' });
			// m_blnFirstTrack = false;
		// }

		// m_arrHistoryStack.push({
			// queue: m_arrQueues[m_intCurrentQueueIndex],
			// type: strType_a, 
			// pointer: m_strCurrentPointer
		// });

		if (arrDecodedData_a.length > m_objOptions.maxTextLength)
		{
			element(m_strComponentID, 'geText').text('[File too large to display: ' + arrDecodedData_a.length + ' bytes]');
			element(m_strComponentID, 'geText').hide();
			updateNowPlaying(arrDecodedData_a);
			updateStatus('Skipped large text file: ' + strFilename_a);
			addToPlayLog(strFilename_a + ' [TXT - too large]', true);
		}
		else
		{
			// ✅ USE decodeUTF8 HERE instead of the loop
			var strText = decodeUTF8(arrDecodedData_a);
			
			element(m_strComponentID, 'geText').html(htmlEncode(strText));
			element(m_strComponentID, 'geText').show();
			element(m_strComponentID, 'geText').scrollTop(0);
			updateNowPlaying(arrDecodedData_a);
			updateStatus('Showing ' + strFilename_a);
			addToPlayLog(strFilename_a + ' [TXT]', true);
		}
		
		scheduleNext();
	}
	
	// end of Queue behaviours
	function nextQueue()
	{
		m_intCurrentQueueIndex++;

		if (m_intCurrentQueueIndex >= m_arrQueues.length) 
		{
			m_intCurrentQueueIndex = 0;
		}

		m_strCurrentPointer = '';
		updateStatus('End of channel - moving to next channel...');
	}
	
	function randomQueue()
	{
		m_intCurrentQueueIndex = Math.floor(Math.random() * m_arrQueues.length);
		m_strCurrentPointer = '';
		updateStatus('End of channel - switching to random channel...');
	}
	
	function repeatAllQueues()
	{
		m_intCurrentQueueIndex = (m_intCurrentQueueIndex + 1) % m_arrQueues.length;
		m_strCurrentPointer = '';
		updateStatus('End of channel - moving to next channel (repeat all)...');
	}
	
	function repeatQueue()
	{
		m_strCurrentPointer = '';
		updateStatus('End of channel - repeating current channel...');
	}
	
	function stopAtEnd()
	{
		m_blnPlaying = false;
		m_objPlayer.pause();
		updateNowPlaying(null);
		updateStatus('Playback stopped at end of channel.');
	}
	
	function knownBehaviourEnd()
	{
		updateStatus('End of channel - unknown behavior, waiting 30s...');
		setTimeout(function() 
		{
			if (m_blnPlaying) fetchNext();
		}, 30000);
	}
	
	function waitAtEnd()
	{
		startWaitCountdown(30);
	}
	
	// player functions
    function handleEndOfQueue() 
	{
		var blnFetchNext = true;
        updateStatus('End of channel reached.');

        switch (m_strBehavior) 
		{
            case 'nextQueue':
				nextQueue();
                break;

            case 'repeatQueue':
				repeatQueue();
                break;

            case 'repeatAllQueues':
				repeatAllQueues();
                break;

            case 'stopAtEnd':
				stopAtEnd();
                blnFetchNext = false;
				break;

            case 'waitAtEnd':
				waitAtEnd();
                blnFetchNext = false;
				break;

            case 'randomQueue':
				randomQueue();
				break;

            default:
				knownBehaviourEnd();
                blnFetchNext = false;
				break;
        }

        if (blnFetchNext && m_blnPlaying) 
		{
			fetchNext();
		}
    }

    function onPlayerEnded() 
	{
        if (m_blnPlaying)
		{
			if (m_objOptions.clearImageAfterSong) 
			{
				element(m_strComponentID, 'geImage').html('');
			}
			
			if (m_objOptions.clearTextAfterSong) 
			{
				element(m_strComponentID, 'geText').html('');
				element(m_strComponentID, 'geText').hide();
			}

			scheduleNext();
		}
    }

    function onPlayerError() 
	{
        if (m_blnPlaying)
		{
			var strCurrentFilename = m_objPlayer.src.split('/').pop();
			updateStatus('Playback error (wrong ROM or corrupted file) - skipping to next...');
			if (strCurrentFilename) 
			{
				addToPlayLog(strCurrentFilename + ' (SKIPPED - playback error)', false);
			}
			scheduleNext();
		}
    }

    function onPlayerFormatError() 
	{
        if (m_blnPlaying)
		{
			updateStatus('Invalid audio format - skipping to next...');
			scheduleNext();
		}
    }

    function playTrack(strFilename_a, arrDecodedData_a, objBlob_a, blnIsDecoded_a) 
	{
        console.log('playing:' + strFilename_a);
        
        m_strCurrentPointer = strFilename_a;
        m_objPlayer.src = URL.createObjectURL(objBlob_a);
        updateNowPlaying(arrDecodedData_a);
        updateStatus('Playing ' + strFilename_a);
        addToPlayLog(strFilename_a, blnIsDecoded_a);

		var objPlayPromise = m_objPlayer.play();
		if (objPlayPromise !== undefined) 
		{
			objPlayPromise.catch(function() 
			{
				addToPlayLog(strFilename_a + ' (SKIPPED play error)', false);
				scheduleNext();
			});
		}
    }

    function scheduleNext() 
	{
        if (m_blnPlaying) 
		{
            fetchNext();
        }
    }

	function startProgressBar() 
	{
		updateProgress(0);

		// Resume audio context if suspended
		if (m_objPlayer.audioContext && m_objPlayer.audioContext.state === 'suspended') 
		{
			m_objPlayer.audioContext.resume();
		}

		m_objPlayer.ontimeupdate = function() 
		{
			var intPercent = (m_objPlayer.currentTime / m_objPlayer.duration) * 100;
			updateProgress(intPercent);
		};
	}

    function startWaitCountdown(intSeconds_a) 
	{
        if (m_objWaitCountdownTimer) 
		{
            clearInterval(m_objWaitCountdownTimer);
        }

        m_intWaitCountdown = intSeconds_a;
        updateStatus('Waiting ' + m_intWaitCountdown + 's for new songs...');

        m_objWaitCountdownTimer = setInterval(function() 
		{
            m_intWaitCountdown--;
            updateStatus('Waiting ' + m_intWaitCountdown + 's for new songs...');

            if (m_intWaitCountdown <= 0) 
			{
                clearInterval(m_objWaitCountdownTimer);
                m_objWaitCountdownTimer = null;
                
                updateStatus('');

                if (m_blnPlaying) fetchNext();
            }
        }, m_objOptions.imageDelay * 1000);
    }

    function switchQueue(intNextIndex_a) 
	{
        m_intCurrentQueueIndex = intNextIndex_a % m_arrQueues.length;
        m_strCurrentPointer = '';
        updateStatus('Switched to channel: ' + m_arrQueues[m_intCurrentQueueIndex]);
    }

	// button events
    function NextTrack_onClick() 
	{
        if (m_blnPlaying && !m_blnFetchNextRunning) 
		{
			if (isFunction(m_objOptions.onNext))
			{
				m_objOptions.onNext();
			}

			updateStatus('Skipping to next track...');
			
			fetchNext();
		}
    }

    function PreviousTrack_onClick() 
	{
        // Check if history exists AND no fetch is currently running
        if (m_arrHistoryStack.length && !m_blnFetchNextRunning) 
		{
			if (isFunction(m_objOptions.onPrevious))
			{
				m_objOptions.onPrevious();
			}

			var objLast3 = null;
			var objLast2 = null;
			var objLast1 = null;
			try
			{
				objLast3 = m_arrHistoryStack.pop();
				objLast2 = m_arrHistoryStack.pop();
				objLast1 = m_arrHistoryStack.pop();
			}
			catch(objError_a)
			{
				//m_arrHistoryStack.push({ queue: m_arrQueues[m_intCurrentQueueIndex], type: '', pointer: '' });
			}
			
			var objLast = objLast1 || objLast2 || objLast3;

			if (objLast !== null)
			{
				m_arrHistoryStack.push(objLast);
				console.log(m_arrHistoryStack);

				m_intCurrentQueueIndex = m_arrQueues.indexOf(objLast.queue);
				m_strCurrentPointer = objLast.pointer;

				updateStatus('Playing previous track from channel: ' + objLast.queue);
			}
			
			fetchNext();
		}
		else
		{
            updateStatus('No previous track available.');
        }
    }
	
	function ProgressBar_onClick(objEvent_a)
	{
		var intSquareIndex = parseInt($(this).attr('data-counter'), 10);
		var fltPercent = intSquareIndex / m_intProgressSquares;

		if (m_objPlayer && m_objPlayer.duration) 
		{
			m_objPlayer.currentTime = fltPercent * m_objPlayer.duration;
			updateProgress(fltPercent * 100);
		}
	}

	function StartPlayer_onClick() 
	{
		var blnValid = true;
		
		// Get values and validate
		m_strQueueURL = element(m_strComponentID, 'geQueueUrl').val();
		m_strBehavior = element(m_strComponentID, 'geBehavior').val();

		element(m_strComponentID, 'geImage').html('');
		element(m_strComponentID, 'geText').html('');
		element(m_strComponentID, 'geText').hide();
		
		// Validation
		if (!m_strQueueURL || m_strQueueURL.trim() === '') 
		{
			updateStatus('Error: Please enter a Web Ratio Station URL');
			blnValid = false;
		}
		
		// Check ROM file
		var objRomFileInput = element(m_strComponentID, 'geROMFile')[0];
		if (blnValid && !objRomFileInput.files[0])
		{
			updateStatus('Error: Please select a ROM file');
			blnValid = false;
		}
		
		// Call onStart to get queues and change UI
		if (blnValid && isFunction(m_objOptions.onStart))
		{
			m_arrQueues = m_objOptions.onStart();
			
			if (!m_arrQueues || m_arrQueues.length === 0) 
			{
				updateStatus('Error: Please select at least one channel');
				// Revert UI changes
				if (isFunction(m_objOptions.onStop))
				{
					m_objOptions.onStop();
				}
				blnValid = false;
			}
		}
		
		if (blnValid)
		{
			m_blnPaused = false;
			element(m_strComponentID, 'geToggleButton').text('Pause');
			setupAudioContext();
			
			m_intCurrentQueueIndex = (m_strBehavior === 'randomQueue') ? Math.floor(Math.random() * m_arrQueues.length) : 0;
			m_strCurrentPointer = '';
			m_blnFetchNextRunning = false;
			m_blnPlaying = true;
			
			updateStatus('Loading ROM...');
			var objReader = new FileReader();
			objReader.onload = function(objEvent) 
			{
				m_arrROM = new Uint8Array(objEvent.target.result);
				updateStatus('ROM loaded (' + m_arrROM.length + ' bytes)');
				startProgressBar();
				fetchNext();
			};
			objReader.readAsArrayBuffer(objRomFileInput.files[0]);
		}
	}

	function OLD_StartPlayer_onClick() 
	{
		var blnValid = true;
		
		// Get values and validate BEFORE calling onStart
		m_strQueueURL = element(m_strComponentID, 'geQueueUrl').val();
		m_strBehavior = element(m_strComponentID, 'geBehavior').val();
		
		// Validation
		if (!m_strQueueURL || m_strQueueURL.trim() === '') 
		{
			updateStatus('Error: Please enter a Web Radio Station URL');
			blnValid = false;
		}
		
		if (blnValid)
		{
			// Get queues from callback
			if (isFunction(m_objOptions.onStart))
			{
				m_arrQueues = m_objOptions.onStart();
			}
			
			if (!m_arrQueues || m_arrQueues.length === 0) 
			{
				updateStatus('Error: Please select at least one channel');
				// Revert UI changes made by onStart
				if (isFunction(m_objOptions.onStop))
				{
					m_objOptions.onStop();
				}
				blnValid = false;
			}
		}
		
		if (blnValid)
		{
			m_blnPaused = false;
			element(m_strComponentID, 'geToggleButton').text('Pause');
			setupAudioContext();
			
			m_intCurrentQueueIndex = (m_strBehavior === 'randomQueue') ? Math.floor(Math.random() * m_arrQueues.length) : 0;
			m_strCurrentPointer = '';
			m_blnFetchNextRunning = false;
			m_blnPlaying = true;
			
			var objRomFileInput = element(m_strComponentID, 'geROMFile')[0];
			if (objRomFileInput.files[0]) 
			{
				updateStatus('Loading ROM...');
				var objReader = new FileReader();
				objReader.onload = function(objEvent) 
				{
					m_arrROM = new Uint8Array(objEvent.target.result);
					updateStatus('ROM loaded (' + m_arrROM.length + ' bytes)');
					startProgressBar();
					fetchNext();
				};
				objReader.readAsArrayBuffer(objRomFileInput.files[0]);
			} 
			else 
			{
				updateStatus('Starting (no ROM - encoded songs will be skipped)');
				startProgressBar();
				fetchNext();
			}
		}
	}

    function StopPlayer_onClick() 
	{
        m_blnPlaying = false;
        m_objPlayer.pause();
        m_objPlayer.src = '';
        updateStatus('Stopped');
        updateNowPlaying(null);
		
        if (m_objFetchNextTimer) 
		{
            clearTimeout(m_objFetchNextTimer);
            m_objFetchNextTimer = null;
        }
		
		element(m_strComponentID, 'geImage').html('');
		element(m_strComponentID, 'geText').html('');
		element(m_strComponentID, 'geText').hide();
		element(m_strComponentID, 'geStatus').html('Ready to play...');
		element(m_strComponentID, 'geNowPlaying').html('No track playing');

		if (isFunction(m_objOptions.onStop))
		{
			m_objOptions.onStop();
		}
    }
	
	function Toggle_onClick()
	{
		if (!m_blnPaused)
		{
			m_objPlayer.pause();
			element(m_strComponentID, 'geToggleButton').text('Play');
		}
		else
		{
			m_objPlayer.play();
			element(m_strComponentID, 'geToggleButton').text('Pause');
		}
		
		m_blnPaused = !m_blnPaused;

		if (isFunction(m_objOptions.onToggle))
		{
			m_objOptions.onToggle(m_blnPaused);
		}
	}

	// network functions
	function fetchNext() 
	{
		if (m_blnPlaying && !m_blnFetchNextRunning)
		{
			m_blnFetchNextRunning = true;

			var strCurrentQueue = m_arrQueues[m_intCurrentQueueIndex];
			var objXHR = new XMLHttpRequest();
			objXHR.open('POST', m_strQueueURL, true);
			objXHR.responseType = 'blob';
			objXHR.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');

			var strPostBody = 'action=fetch&q=' + encodeURIComponent(strCurrentQueue) +
							  '&after=' + encodeURIComponent(m_strCurrentPointer);
			console.log(strPostBody);

			objXHR.onload = function() 
			{
				m_blnFetchNextRunning = false;
				if (!m_blnPlaying) return;

				if (objXHR.status === 200 && objXHR.response && objXHR.response.size > 0)
				{
					var strContentDisp = objXHR.getResponseHeader('Content-Disposition') || '';
					var arrMatches = strContentDisp.match(/filename="(.+)"/);
					var strFilename = arrMatches ? arrMatches[1].trim() : null;

					if (strFilename && strFilename !== m_strCurrentPointer)
					{
						m_strCurrentPointer = strFilename;

						var objReader = new FileReader();
						objReader.onload = function(objEvent) 
						{
							fetchedNext(strFilename, new Uint8Array(objEvent.target.result));
						};
						objReader.readAsArrayBuffer(objXHR.response);
					}
					else
					{
						handleEndOfQueue();
					}
				}
				else if (objXHR.status !== 200)
				{
					updateStatus('HTTP ' + objXHR.status + ' - retrying in 30s...');
					m_objFetchNextTimer = setTimeout(fetchNext, 30000);
				}
				else
				{
					handleEndOfQueue();
				}
			};

			objXHR.onerror = function() 
			{
				m_blnFetchNextRunning = false;
				if (m_blnPlaying) 
				{
					m_objFetchNextTimer = setTimeout(fetchNext, 30000);
				}
			};

			objXHR.send(strPostBody);
		}
	}

	// New function to handle fetched data
	function fetchedNext(strFilename_a, arrData_a) 
	{
		if (m_blnPlaying) 
		{
			var arrDecoded = null;
			var blnDecodeError = false;

			// Attempt to decode
			try 
			{
				arrDecoded = zosciiDecode(m_arrROM, arrData_a);
				if (!arrDecoded || arrDecoded.length === 0) 
				{
					addToPlayLog(strFilename_a + ' (SKIPPED decode error)', false);
					blnDecodeError = true;
				}
			} 
			catch (objError) 
			{
				addToPlayLog(strFilename_a + ' (SKIPPED decode error)', false);
				blnDecodeError = true;
			}

			// Only handle the file if decoding succeeded
			if (!blnDecodeError) 
			{
				var strType = identifyType(arrDecoded);
				
				function dispatch()
				{
					if (strType === 'mp3') 
					{
						playMP3(strType, strFilename_a, arrDecoded);
					}
					else if (strType === 'jpg') 
					{
						showJPG(strType, strFilename_a, arrDecoded);
					}
					else if (strType === 'text')
					{
						showText(strType, strFilename_a, arrDecoded);
					}
					else
					{
						// unsupported type
						scheduleNext();
					}
				}

				if (m_strPreviousType === 'jpg')
				{
					setTimeout(function()
					{
						dispatch();
					}, 2000);
				}
				else
				{
					dispatch();
				}

				m_strPreviousType = strType;
			} 
			else 
			{
				// If decoding failed, move to the next track
				scheduleNext();
			}
		}
	}

	initialise();
}