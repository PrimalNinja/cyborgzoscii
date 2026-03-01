#!/usr/bin/env python3
# Cyborg ZOSCII v20260301
# (c) 2026 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import struct
import os

class RomData:
    def __init__(self):
        self.ptrROMData = b''
        self.lngROMSize = 0

ZOSCII_ROM_LOAD_MAX = 131072

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
        except IOError:
            ptrRom = None
    
    return ptrRom

def unloadRom(ptrRom_a):
    # In Python, garbage collector handles this, but method kept for symmetry
    ptrRom_a.ptrROMData = b''
    ptrRom_a.lngROMSize = 0

def decodeFile(ptrRom_a, strInputFile_a, strOutputFile_a):
    blnSuccess = False
    ptrInput = None
    ptrOutput = None
    
    if os.path.exists(strInputFile_a):
        try:
            ptrInput = open(strInputFile_a, 'rb')
            
            # Get input file size
            ptrInput.seek(0, 2)
            lngInputSize = ptrInput.tell()
            ptrInput.seek(0, 0)
            
            lngSlots = lngInputSize // 2
            
            if lngSlots >= 0:
                ptrOutput = open(strOutputFile_a, 'wb')
                
                # Decode each slot
                for intI in range(lngSlots):
                    arrBuf = ptrInput.read(2)
                    if len(arrBuf) != 2:
                        break
                    intAddr = struct.unpack('<H', arrBuf)[0]
                    if intAddr < ptrRom_a.lngROMSize:
                        ptrOutput.write(bytes([ptrRom_a.ptrROMData[intAddr]]))
                
                if lngSlots > 0 and intI == lngSlots - 1:
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
    blnDecodeOk = False
    
    print("ZOSCII Decoder")
    print("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License\n")
    
    if len(sys.argv) == 4:
        ptrRom = loadRom(sys.argv[1])
        if ptrRom:
            blnDecodeOk = decodeFile(ptrRom, sys.argv[2], sys.argv[3])
            
            if blnDecodeOk:
                intResult = 0
                print("Decode successful!")
            else:
                print("Decode failed", file=sys.stderr)
            
            unloadRom(ptrRom)
        else:
            print("Failed to load ROM", file=sys.stderr)
    else:
        print(f"Usage: {sys.argv[0]} <romfile> <encoded> <output>", file=sys.stderr)
    
    sys.exit(intResult)

if __name__ == "__main__":
    main()