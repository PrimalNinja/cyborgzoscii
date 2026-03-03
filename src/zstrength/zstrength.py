#!/usr/bin/env python3
# Cyborg ZOSCII v20260303
# (c) 2026 Cyborg Unicorn Pty Ltd.
# This software is released under MIT License.

import sys
import math
import os

class ByteAddresses:
    def __init__(self):
        self.ptrAddresses = []
        self.intCount = 0

class RomData:
    def __init__(self):
        self.ptrROMData = b''
        self.lngROMSize = 0
        self.arrROMCounts = [0] * 256
        self.arrROMCountsHigh = [0] * 256

ZOSCII_ROM_LOAD_MAX = 131072

def printLargeNumber(dblExponent_a):
    if dblExponent_a < 3:
        print(f"~{10**dblExponent_a:.0f} permutations", end='')
    elif dblExponent_a < 6:
        print(f"~{10**dblExponent_a / 1000.0:.1f} thousand permutations", end='')
    elif dblExponent_a < 9:
        print(f"~{10**dblExponent_a / 1000000.0:.1f} million permutations", end='')
    elif dblExponent_a < 12:
        print(f"~{10**dblExponent_a / 1000000000.0:.1f} billion permutations", end='')
    elif dblExponent_a < 15:
        print(f"~{10**dblExponent_a / 1000000000000.0:.1f} trillion permutations", end='')
    elif dblExponent_a < 82:
        print(f"More than all atoms in the observable universe (10^{dblExponent_a:.0f} permutations)", end='')
    elif dblExponent_a < 1000:
        print(f"Incomprehensibly massive (10^{dblExponent_a:.0f} permutations)", end='')
    else:
        print(f"Astronomically secure (10^{dblExponent_a / 1000000.0:.1f}M permutations)", end='')

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
                    
                    # Count ROM byte occurrences - first 64KB (encoding range)
                    lngLowSize = min(ptrRom.lngROMSize, 65536)
                    for lngI in range(lngLowSize):
                        ptrRom.arrROMCounts[ptrRom.ptrROMData[lngI]] += 1
                    
                    # Count ROM byte occurrences - second 64KB (if present)
                    for lngI in range(65536, ptrRom.lngROMSize):
                        ptrRom.arrROMCountsHigh[ptrRom.ptrROMData[lngI]] += 1
        except IOError:
            ptrRom = None
    
    return ptrRom

def unloadRom(ptrRom_a):
    # In Python, garbage collector handles this, but method kept for symmetry
    ptrRom_a.ptrROMData = b''
    ptrRom_a.lngROMSize = 0
    ptrRom_a.arrROMCounts = [0] * 256
    ptrRom_a.arrROMCountsHigh = [0] * 256

def analyzeFile(ptrRom_a, strInputFile_a):
    blnSuccess = False
    arrInputCounts = [0] * 256
    intInputLength = 0
    intCharsUsed = 0
    dblGeneralStrength = 0.0
    dblFileStrength = 0.0
    dblUtilisation = 0.0
    
    if os.path.exists(strInputFile_a):
        try:
            with open(strInputFile_a, 'rb') as ptrInput:
                inputData = ptrInput.read()
            
            intInputLength = len(inputData)
            
            # Count input character occurrences
            for by in inputData:
                arrInputCounts[by] += 1
            
            # Count characters utilized
            for intI in range(256):
                if arrInputCounts[intI] > 0:
                    intCharsUsed += 1
            
            # Calculate ROM strength metrics
            for intI in range(256):
                if ptrRom_a.arrROMCounts[intI] > 0:
                    dblGeneralStrength += math.log10(ptrRom_a.arrROMCounts[intI])
                if arrInputCounts[intI] > 0 and ptrRom_a.arrROMCounts[intI] > 0:
                    dblFileStrength += arrInputCounts[intI] * math.log10(ptrRom_a.arrROMCounts[intI])
            
            dblUtilisation = (intCharsUsed / 256.0) * 100.0
            
            print("ROM Strength Analysis")
            print("=====================")
            print()
            
            print("Input Information:")
            print(f"- Text Length: {intInputLength} characters")
            print(f"- Characters Utilized: {intCharsUsed} of 256 ({dblUtilisation:.1f}%)")
            print()
            
            print(f"General ROM Capacity: ~10^{dblGeneralStrength:.0f} (", end='')
            printLargeNumber(dblGeneralStrength)
            print(")")
            
            print(f"This File Security: ~10^{dblFileStrength:.0f} (", end='')
            printLargeNumber(dblFileStrength)
            print(")")
            print()
            
            print("Byte Analysis:")
            print("Byte  Dec  ROM Lo 64K  ROM Hi 64K  Input Count  Char")
            print("----  ---  ----------  ----------  -----------  ----")
            
            for intI in range(256):
                if ptrRom_a.arrROMCounts[intI] > 0 or ptrRom_a.arrROMCountsHigh[intI] > 0 or arrInputCounts[intI] > 0:
                    chDisplay = chr(intI) if 32 <= intI <= 126 else ' '
                    print(f"0x{intI:02X}  {intI:3d}  {ptrRom_a.arrROMCounts[intI]:10d}  {ptrRom_a.arrROMCountsHigh[intI]:10d}  {arrInputCounts[intI]:11d}    {chDisplay}")
            
            blnSuccess = True
        except IOError:
            blnSuccess = False
    
    return blnSuccess

def main():
    intResult = 1
    ptrRom = None
    blnAnalyzeOk = False
    
    print("ZOSCII ROM Strength Analyzer v20260303")
    print("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License")
    print()
    
    if len(sys.argv) == 3:
        ptrRom = loadRom(sys.argv[1])
        if ptrRom:
            blnAnalyzeOk = analyzeFile(ptrRom, sys.argv[2])
            
            if blnAnalyzeOk:
                intResult = 0
            else:
                print("Analysis failed", file=sys.stderr)
            
            unloadRom(ptrRom)
        else:
            print("Failed to load ROM", file=sys.stderr)
    else:
        print(f"Usage: {sys.argv[0]} <romfile> <inputdatafile>", file=sys.stderr)
    
    sys.exit(intResult)

if __name__ == "__main__":
    main()