// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
    "fmt"
    "math"
    "os"
)

type ByteAddresses struct {
    ptrAddresses []uint16
    intCount     uint32
}

type RomData struct {
    ptrROMData   []byte
    lngROMSize   int64
    arrROMCounts   [256]uint32
    arrROMCountsHigh [256]uint32
}

const ZOSCII_ROM_LOAD_MAX = 131072

func printLargeNumber(dblExponent_a float64) {
    if dblExponent_a < 3 {
        fmt.Printf("~%.0f permutations", math.Pow(10, dblExponent_a))
    } else if dblExponent_a < 6 {
        fmt.Printf("~%.1f thousand permutations", math.Pow(10, dblExponent_a)/1000.0)
    } else if dblExponent_a < 9 {
        fmt.Printf("~%.1f million permutations", math.Pow(10, dblExponent_a)/1000000.0)
    } else if dblExponent_a < 12 {
        fmt.Printf("~%.1f billion permutations", math.Pow(10, dblExponent_a)/1000000000.0)
    } else if dblExponent_a < 15 {
        fmt.Printf("~%.1f trillion permutations", math.Pow(10, dblExponent_a)/1000000000000.0)
    } else if dblExponent_a < 82 {
        fmt.Printf("More than all atoms in the observable universe (10^%.0f permutations)", dblExponent_a)
    } else if dblExponent_a < 1000 {
        fmt.Printf("Incomprehensibly massive (10^%.0f permutations)", dblExponent_a)
    } else {
        fmt.Printf("Astronomically secure (10^%.1fM permutations)", dblExponent_a/1000000.0)
    }
}

func loadRom(strFilename_a string) (*RomData, error) {
    var ptrRom *RomData = nil
    var err error = nil
    
    arrBuf, err := os.ReadFile(strFilename_a)
    if err == nil {
        lngSize := int64(len(arrBuf))
        if lngSize > ZOSCII_ROM_LOAD_MAX {
            arrBuf = arrBuf[:ZOSCII_ROM_LOAD_MAX]
            lngSize = ZOSCII_ROM_LOAD_MAX
        }
        
        ptrRom = &RomData{
            ptrROMData: arrBuf,
            lngROMSize: lngSize,
        }
        
        // Count ROM byte occurrences - first 64KB (encoding range)
        lngLowSize := lngSize
        if lngLowSize > 65536 {
            lngLowSize = 65536
        }
        for lngI := int64(0); lngI < lngLowSize; lngI++ {
            ptrRom.arrROMCounts[ptrRom.ptrROMData[lngI]]++
        }
        
        // Count ROM byte occurrences - second 64KB (if present)
        for lngI := int64(65536); lngI < lngSize; lngI++ {
            ptrRom.arrROMCountsHigh[ptrRom.ptrROMData[lngI]]++
        }
    }
    
    return ptrRom, err
}

func unloadRom(ptrRom_a *RomData) {
    // In Go, garbage collector handles this, but method kept for symmetry
    ptrRom_a.ptrROMData = nil
    ptrRom_a.lngROMSize = 0
    for intI := 0; intI < 256; intI++ {
        ptrRom_a.arrROMCounts[intI] = 0
        ptrRom_a.arrROMCountsHigh[intI] = 0
    }
}

func analyzeFile(ptrRom_a *RomData, strInputFile_a string) bool {
    var blnSuccess bool = false
    var arrInputCounts [256]uint32
    var intInputLength int = 0
    var intCharsUsed int = 0
    var dblGeneralStrength float64 = 0.0
    var dblFileStrength float64 = 0.0
    var dblUtilisation float64 = 0.0
    
    inputData, err := os.ReadFile(strInputFile_a)
    if err == nil {
        intInputLength = len(inputData)
        
        // Count input character occurrences
        for _, by := range inputData {
            arrInputCounts[by]++
        }
        
        // Count characters utilized
        for intI := 0; intI < 256; intI++ {
            if arrInputCounts[intI] > 0 {
                intCharsUsed++
            }
        }
        
        // Calculate ROM strength metrics
        for intI := 0; intI < 256; intI++ {
            if ptrRom_a.arrROMCounts[intI] > 0 {
                dblGeneralStrength += math.Log10(float64(ptrRom_a.arrROMCounts[intI]))
            }
            if arrInputCounts[intI] > 0 && ptrRom_a.arrROMCounts[intI] > 0 {
                dblFileStrength += float64(arrInputCounts[intI]) * math.Log10(float64(ptrRom_a.arrROMCounts[intI]))
            }
        }
        
        dblUtilisation = (float64(intCharsUsed) / 256.0) * 100.0
        
        fmt.Println("ROM Strength Analysis")
        fmt.Println("=====================")
        fmt.Println()
        
        fmt.Println("Input Information:")
        fmt.Printf("- Text Length: %d characters\n", intInputLength)
        fmt.Printf("- Characters Utilized: %d of 256 (%.1f%%)\n", intCharsUsed, dblUtilisation)
        fmt.Println()
        
        fmt.Printf("General ROM Capacity: ~10^%.0f (", dblGeneralStrength)
        printLargeNumber(dblGeneralStrength)
        fmt.Println(")")
        
        fmt.Printf("This File Security: ~10^%.0f (", dblFileStrength)
        printLargeNumber(dblFileStrength)
        fmt.Println(")")
        fmt.Println()
        
        fmt.Println("Byte Analysis:")
        fmt.Println("Byte  Dec  ROM Lo 64K  ROM Hi 64K  Input Count  Char")
        fmt.Println("----  ---  ----------  ----------  -----------  ----")
        
        for intI := 0; intI < 256; intI++ {
            if ptrRom_a.arrROMCounts[intI] > 0 || ptrRom_a.arrROMCountsHigh[intI] > 0 || arrInputCounts[intI] > 0 {
                var chDisplay rune
                if intI >= 32 && intI <= 126 {
                    chDisplay = rune(intI)
                } else {
                    chDisplay = ' '
                }
                fmt.Printf("0x%02X  %3d  %10d  %10d  %11d    %c\n",
                    intI, intI, ptrRom_a.arrROMCounts[intI], ptrRom_a.arrROMCountsHigh[intI], arrInputCounts[intI], chDisplay)
            }
        }
        
        blnSuccess = true
    }
    
    return blnSuccess
}

func main() {
    var intResult int = 1
    var ptrRom *RomData = nil
    var err error = nil
    var blnAnalyzeOk bool = false
    
    fmt.Println("ZOSCII ROM Strength Analyzer")
    fmt.Println("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License")
    fmt.Println()

    strArgs := os.Args
    if len(strArgs) == 3 {
        ptrRom, err = loadRom(strArgs[1])
        if err == nil && ptrRom != nil {
            blnAnalyzeOk = analyzeFile(ptrRom, strArgs[2])
            
            if blnAnalyzeOk {
                intResult = 0
            } else {
                fmt.Fprintf(os.Stderr, "Analysis failed\n")
            }
            
            unloadRom(ptrRom)
        } else {
            if err != nil {
                fmt.Fprintf(os.Stderr, "Failed to load ROM: %v\n", err)
            } else {
                fmt.Fprintf(os.Stderr, "Failed to load ROM\n")
            }
        }
    } else {
        fmt.Fprintf(os.Stderr, "Usage: %s <romfile> <inputdatafile>\n", strArgs[0])
    }
    
    os.Exit(intResult)
}