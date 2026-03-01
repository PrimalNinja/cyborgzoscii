#!/usr/bin/env python3
# Cyborg ZOSCII v20260301
# (c) 2026 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import random
import struct
import os

class ByteAddresses:
    def __init__(self):
        self.ptrAddresses = []
        self.intCount = 0

class RomData:
    def __init__(self):
        self.ptrROMData = b''
        self.lngROMSize = 0
        self.arrLookup = [ByteAddresses() for _ in range(256)]

ZOSCII_ROM_LOAD_MAX = 131072

def buildLookupTable(ptrRom_a):
    arrCounts = [0] * 256
    lngROMSize = 0
    
    # ROM addresses are 16-bit, so only use first 64KB
    lngROMSize = ptrRom_a.lngROMSize
    if lngROMSize > 65536:
        lngROMSize = 65536
    
    # Count occurrences
    for lngI in range(lngROMSize):
        arrCounts[ptrRom_a.ptrROMData[lngI]] += 1
    
    # Allocate memory for each byte value
    for intI in range(256):
        if arrCounts[intI] > 0:
            ptrRom_a.arrLookup[intI].ptrAddresses = [0] * arrCounts[intI]
            ptrRom_a.arrLookup[intI].intCount = 0
    
    # Fill addresses
    for lngI in range(lngROMSize):
        by = ptrRom_a.ptrROMData[lngI]
        ptrRom_a.arrLookup[by].ptrAddresses[ptrRom_a.arrLookup[by].intCount] = lngI
        ptrRom_a.arrLookup[by].intCount += 1

def loadRom(strFilename_a):
    ptrRom = None
    
    if os.path.exists(strFilename_a):
        try:
            with open(strFilename_a, 'rb') as ptrFile:
                arrBuf = ptrFile.read(ZOSCII_ROM_LOAD_MAX)
                if arrBuf:
                    ptrRom = RomData()
                    ptrRom.ptrROMData = arrBuf
                    ptrRom.lngROMSize = len(arrBuf)
                    
                    # Pre-build lookup table for reuse across multiple encodes
                    buildLookupTable(ptrRom)
        except IOError:
            ptrRom = None
    
    return ptrRom

def unloadRom(ptrRom_a):
    # In Python, garbage collector handles this, but method kept for symmetry
    ptrRom_a.ptrROMData = b''
    ptrRom_a.lngROMSize = 0
    ptrRom_a.arrLookup = [ByteAddresses() for _ in range(256)]

def encodeFile(ptrRom_a, strInputFile_a, strOutputFile_a):
    blnSuccess = False
    ptrInput = None
    ptrOutput = None
    
    if os.path.exists(strInputFile_a):
        try:
            ptrInput = open(strInputFile_a, 'rb')
            ptrOutput = open(strOutputFile_a, 'wb')
            
            # Stream-encode input
            while True:
                intCh = ptrInput.read(1)
                if not intCh:
                    break
                by = intCh[0]
                if ptrRom_a.arrLookup[by].intCount > 0:
                    intRandomIdx = random.randint(0, ptrRom_a.arrLookup[by].intCount - 1)
                    intAddress = ptrRom_a.arrLookup[by].ptrAddresses[intRandomIdx]
                    ptrOutput.write(struct.pack('<H', intAddress))
            
            blnSuccess = True
            ptrOutput.close()
            ptrInput.close()
        except IOError:
            if ptrOutput:
                ptrOutput.close()
            if ptrInput:
                ptrInput.close()
    
    return blnSuccess

def main():
    intResult = 1
    ptrRom = None
    blnEncodeOk = False
    
    print("ZOSCII Encoder")
    print("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n")
    
    if len(sys.argv) == 4:
        random.seed()
        
        ptrRom = loadRom(sys.argv[1])
        if ptrRom:
            blnEncodeOk = encodeFile(ptrRom, sys.argv[2], sys.argv[3])
            
            if blnEncodeOk:
                intResult = 0
                print("Encode successful!")
            else:
                print("Encode failed", file=sys.stderr)
            
            unloadRom(ptrRom)
        else:
            print("Failed to load ROM", file=sys.stderr)
    else:
        print(f"Usage: {sys.argv[0]} <romfile> <inputdatafile> <encodedoutput>", file=sys.stderr)
    
    sys.exit(intResult)

if __name__ == "__main__":
    main()