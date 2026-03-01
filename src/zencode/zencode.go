// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"math/rand"
	"os"
	"time"
)

type ByteAddresses struct 
{
	ptrAddresses []uint16
	intCount     uint32
}

const (
	ZOSCII_ROM_LOAD_MAX = 131072
)

type RomData struct 
{
	ptrROMData []byte
	lngROMSize int64
	arrLookup  [256]ByteAddresses
}

func buildLookupTable(ptrRom_a *RomData) 
{
	var arrCounts [256]uint32
	var lngROMSize int64 = 0
	var lngI int64 = 0
	var intI int = 0
	
	// Initialize lookup array
	for intI = 0; intI < 256; intI++ 
	{
		ptrRom_a.arrLookup[intI].ptrAddresses = nil
		ptrRom_a.arrLookup[intI].intCount = 0
	}
	
	// ROM addresses are 16-bit, so only use first 64KB
	lngROMSize = ptrRom_a.lngROMSize
	if lngROMSize > 65536 
	{
		lngROMSize = 65536
	}
	
	// Count occurrences
	for lngI = 0; lngI < lngROMSize; lngI++ 
	{
		arrCounts[ptrRom_a.ptrROMData[lngI]]++
	}
	
	// Allocate memory for each byte value
	for intI = 0; intI < 256; intI++ 
	{
		if arrCounts[intI] > 0 
		{
			ptrRom_a.arrLookup[intI].ptrAddresses = make([]uint16, 0, arrCounts[intI])
			ptrRom_a.arrLookup[intI].intCount = 0
		}
	}
	
	// Fill addresses
	for lngI = 0; lngI < lngROMSize; lngI++ 
	{
		var by byte = ptrRom_a.ptrROMData[lngI]
		ptrRom_a.arrLookup[by].ptrAddresses = append(ptrRom_a.arrLookup[by].ptrAddresses, uint16(lngI))
		ptrRom_a.arrLookup[by].intCount++
	}
}

func loadRom(strFilename_a string) (*RomData, error) 
{
	var ptrRom *RomData = nil
	var ptrFile *os.File = nil
	var err error = nil
	
	ptrFile, err = os.Open(strFilename_a)
	if err == nil 
	{
		defer ptrFile.Close()
		
		var arrBuf []byte = make([]byte, ZOSCII_ROM_LOAD_MAX)
		var n int = 0
		
		n, err = io.ReadFull(ptrFile, arrBuf)
		if err == nil || err == io.ErrUnexpectedEOF 
		{
			ptrRom = &RomData{}
			ptrRom.ptrROMData = arrBuf[:n]
			ptrRom.lngROMSize = int64(len(ptrRom.ptrROMData))
			
			// Pre-build lookup table for reuse across multiple encodes
			buildLookupTable(ptrRom)
		} 
	}
	
	return ptrRom, err
}

func unloadRom(ptrRom_a *RomData) 
{
	// In Go, garbage collector handles this, but method kept for symmetry
	ptrRom_a.ptrROMData = nil
	ptrRom_a.lngROMSize = 0
	for intI := 0; intI < 256; intI++ 
	{
		ptrRom_a.arrLookup[intI].ptrAddresses = nil
		ptrRom_a.arrLookup[intI].intCount = 0
	}
}

func encodeFile(ptrRom_a *RomData, strInputFile_a string, strOutputFile_a string) bool 
{
	var blnSuccess bool = false
	var ptrInput *os.File = nil
	var ptrOutput *os.File = nil
	var err error = nil
	var arrBuf []byte = make([]byte, 1)
	
	ptrInput, err = os.Open(strInputFile_a)
	if err == nil 
	{
		defer ptrInput.Close()
		
		ptrOutput, err = os.Create(strOutputFile_a)
		if err == nil 
		{
			defer ptrOutput.Close()
			
			// Stream-encode input
			for 
			{
				_, err = ptrInput.Read(arrBuf)
				if err != nil 
				{
					break
				}
				
				var by byte = arrBuf[0]
				if ptrRom_a.arrLookup[by].intCount > 0 
				{
					var intRandomIdx int = rand.Intn(int(ptrRom_a.arrLookup[by].intCount))
					var intAddress uint16 = ptrRom_a.arrLookup[by].ptrAddresses[intRandomIdx]
					binary.Write(ptrOutput, binary.LittleEndian, intAddress)
				}
			}
			
			blnSuccess = true
		}
	}
	
	return blnSuccess
}

func main() 
{
	var intResult int = 1
	var ptrRom *RomData = nil
	var err error = nil
	var blnEncodeOk bool = false
	
	fmt.Println("ZOSCII Encoder")
	fmt.Println("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n")

	strArgs := os.Args
	if len(strArgs) == 4
	{
		rand.Seed(time.Now().UnixNano())
		
		ptrRom, err = loadRom(strArgs[1])
		if err == nil && ptrRom != nil 
		{
			blnEncodeOk = encodeFile(ptrRom, strArgs[2], strArgs[3])
			
			if blnEncodeOk 
			{
				intResult = 0
			} 
			else 
			{
				fmt.Fprintf(os.Stderr, "Encode failed\n")
			}
			
			unloadRom(ptrRom)
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
		fmt.Fprintf(os.Stderr, "Usage: %s <romfile> <inputdatafile> <encodedoutput>\n", strArgs[0])
	}
	
	os.Exit(intResult)
}