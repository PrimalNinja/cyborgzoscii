// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"
)

const (
	ZOSCII_ROM_LOAD_MAX = 131072
)

type RomData struct {
	ptrROMData []byte
	lngROMSize int64
}

func loadRom(strFilename_a string) (*RomData, error) 
{
	var ptrRom *RomData = nil
	var ptrFile *os.File = nil
	var info os.FileInfo = nil
	var err error = nil
	var lngSize int64 = 0
	
	ptrFile, err = os.Open(strFilename_a)
	if err == nil 
	{
		defer ptrFile.Close()
		
		info, err = ptrFile.Stat()
		if err == nil 
		{
			lngSize = info.Size()
			if lngSize > ZOSCII_ROM_LOAD_MAX 
			{
				lngSize = ZOSCII_ROM_LOAD_MAX
			}
			
			var arrBuf []byte = make([]byte, lngSize)
			_, err = io.ReadFull(ptrFile, arrBuf)
			if err == nil 
			{
				ptrRom = &RomData{}
				ptrRom.ptrROMData = arrBuf
				ptrRom.lngROMSize = lngSize
			}
		}
	}
	
	return ptrRom, err
}

func decodeFile(ptrRom_a *RomData, strInputFile_a string, strOutputFile_a string) bool 
{
	var blnSuccess bool = false
	var ptrInput *os.File = nil
	var ptrOutput *os.File = nil
	var err error = nil
	
	ptrInput, err = os.Open(strInputFile_a)
	if err == nil 
	{
		defer ptrInput.Close()
		
		var ptrInputInfo os.FileInfo = nil
		ptrInputInfo, err = ptrInput.Stat()
		if err == nil 
		{
			lngInputSize := ptrInputInfo.Size()
			lngSlots := lngInputSize / 2
			
			if lngSlots >= 0 
			{
				ptrOutput, err = os.Create(strOutputFile_a)
				if err == nil 
				{
					defer ptrOutput.Close()
					
					arrAddrBuf := make([]byte, 2)
					var intI int64 = 0
					
					// Decode each slot
					for intI = 0; intI < lngSlots; intI++ 
					{
						_, err = io.ReadFull(ptrInput, arrAddrBuf)
						if err != nil 
						{
							break
						}
						
						intAddr := int64(binary.LittleEndian.Uint16(arrAddrBuf))
						if intAddr < ptrRom_a.lngROMSize 
						{
							_, err = ptrOutput.Write([]byte{ptrRom_a.ptrROMData[intAddr]})
							if err != nil 
							{
								break
							}
						}
					}
					
					if err == nil 
					{
						blnSuccess = true
					}
				}
			}
		}
	}
	
	return blnSuccess
}

func main() 
{
	var intResult int = 1
	var ptrRom *RomData = nil
	var err error = nil
	var blnDecodeOk bool = false
	
	fmt.Println("ZOSCII Decoder v20260303")
	fmt.Println("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License\n")

	strArgs := os.Args
	if len(strArgs) == 4
	{
		ptrRom, err = loadRom(strArgs[1])
		if err == nil && ptrRom != nil 
		{
			blnDecodeOk = decodeFile(ptrRom, strArgs[2], strArgs[3])
			
			if blnDecodeOk 
			{
				intResult = 0
			} 
			else 
			{
				fmt.Fprintf(os.Stderr, "Decode failed\n")
			}
		} 
		else 
		{
			if err != nil 
			{
				fmt.Fprintf(os.Stderr, "Failed to load ROM: %v\n", err)
			} 
			else 
			{
				fmt.Fprintf(os.Stderr, "Failed to load ROM\n")
			}
		}
	} 
	else 
	{
		fmt.Fprintf(os.Stderr, "Usage: %s <romfile> <encoded> <output>\n", strArgs[0])
	}
	
	os.Exit(intResult)
}