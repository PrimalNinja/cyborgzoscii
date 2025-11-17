function ZOSCIIMQPublisher(strComponentID_a, objOptions_a)
{
	var m_objThis = this;
	var m_strComponentID = strComponentID_a;
	
	// helpers
	function element(strScope_a, strClass_a)
	{
		return $('.' + strClass_a, '#' + strScope_a);
	}
	
	// initialisation
	function initialise()
	{
        element(m_strComponentID, 'gePublishFileButton').on('click', PublishFile_onClick);
	}

	// event handlers
	function PublishFile_onClick() 
	{
		var objPublishFileInput = element(m_strComponentID, 'gePublishFile')[0];
		var objROMFileInput = element(m_strComponentID, 'geROMFile')[0];
		var objStatus = element(m_strComponentID, 'geStatus');
		var strQueueName = element(m_strComponentID, 'geQueueName').val();
		var strQueueURL = element(m_strComponentID, 'geQueueURL').val();
		var strRetention = element(m_strComponentID, 'geRetention').val();

		if (objROMFileInput.files[0]) 
		{
			if (objPublishFileInput.files[0]) 
			{
				var objROMFile = objROMFileInput.files[0];
				var objPublishFile = objPublishFileInput.files[0];
				
				objStatus.html('Reading ROM file...');

				var objROMReader = new FileReader();
				objROMReader.onload = function(e) 
				{
					var arrROM = new Uint8Array(e.target.result);
					objStatus.html('Reading publish file...');

					var objPublishReader = new FileReader();
					objPublishReader.onload = function(e) 
					{
						var arrPublishData = new Uint8Array(e.target.result);
						objStatus.html('Encoding...');

						setTimeout(function() 
						{
							var intMaxSize = 65536;
							var intSize = arrROM.length;
console.log(intSize);
							if (intSize > intMaxSize)
							{
								intSize = intMaxSize;
							}
							
							var arrMemoryBlocks = [
								{start: 0, size: intSize}
							];
console.log(arrMemoryBlocks);

							var objZOSResult = toZOSCII(arrROM, arrPublishData, arrMemoryBlocks, null, 0);

							var arrZOSBinary = new Uint8Array(objZOSResult.addresses.length * 2);
							for (var intI = 0; intI < objZOSResult.addresses.length; intI++) 
							{
								var intAddr = objZOSResult.addresses[intI];
								arrZOSBinary[intI * 2] = intAddr & 0xFF;
								arrZOSBinary[intI * 2 + 1] = (intAddr >> 8) & 0xFF;
							}

							objStatus.html('Uploading...');

							var objFormData = new FormData();
							objFormData.append('action', 'publish');
							objFormData.append('q', strQueueName);
							objFormData.append('r', strRetention);
							objFormData.append('msg', new Blob([arrZOSBinary], { type: objPublishFile.type }));

							$.ajax({
								url: strQueueURL,
								type: 'POST',
								data: objFormData,
								processData: false,
								contentType: false,
								dataType: 'json',
								success: function(objResponse_a) 
								{
									if (objResponse_a.error && objResponse_a.error.length > 0) 
									{
										objStatus.html('Error: ' + objResponse_a.error);
									} 
									else 
									{
										objStatus.html('Published! Original: ' + arrPublishData.length + ' bytes, Encoded: ' + arrZOSBinary.length + ' bytes');
									}
								},
								error: function(objXHR_a, strStatus_a, strError_a) 
								{
									objStatus.html('Error: ' + objXHR_a.status + ' - ' + objXHR_a.responseText);
								}
							});
						}, 100);
					};
					objPublishReader.readAsArrayBuffer(objPublishFile);
				};
				objROMReader.readAsArrayBuffer(objROMFile);
			}
			else
			{
				objStatus.html('Please select publish file');
			}
		}
		else
		{
			objStatus.html('Please select ROM file');
		}
	}
	
	initialise();
}
