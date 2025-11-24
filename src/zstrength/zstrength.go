// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
    "fmt"
    "math"
    "os"
)

func printLargeNumber(exponent float64) {
    if exponent < 3 {
        fmt.Printf("~%.0f permutations", math.Pow(10, exponent))
    } else if exponent < 6 {
        fmt.Printf("~%.1f thousand permutations", math.Pow(10, exponent)/1000.0)
    } else if exponent < 9 {
        fmt.Printf("~%.1f million permutations", math.Pow(10, exponent)/1000000.0)
    } else if exponent < 12 {
        fmt.Printf("~%.1f billion permutations", math.Pow(10, exponent)/1000000000.0)
    } else if exponent < 15 {
        fmt.Printf("~%.1f trillion permutations", math.Pow(10, exponent)/1000000000000.0)
    } else if exponent < 82 {
        fmt.Printf("More than all atoms in the observable universe (10^%.0f permutations)", exponent)
    } else if exponent < 1000 {
        fmt.Printf("Incomprehensibly massive (10^%.0f permutations)", exponent)
    } else {
        fmt.Printf("Astronomically secure (10^%.1fM permutations)", exponent/1000000.0)
    }
}

func main() {
    fmt.Println("ZOSCII ROM Strength Analyzer")
    fmt.Println("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License\n")

    bittage := 16 // default
    offset := 0

    args := os.Args

    if len(args) >= 2 && args[1] == "-32" {
        bittage = 32
        offset = 1
    } else if len(args) >= 2 && args[1] == "-16" {
        bittage = 16
        offset = 1
    }

    if len(args) != 3+offset {
        fmt.Fprintf(os.Stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile>\n", args[0])
        os.Exit(1)
    }

    // Read ROM file
    romData, err := os.ReadFile(args[1+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening ROM file: %v\n", err)
        os.Exit(1)
    }

    romSize := int64(len(romData))

    // Check ROM size limit based on bit width
    var maxSize int64
    if bittage == 16 {
        maxSize = 65536
    } else {
        maxSize = 4294967296
    }

    if romSize > maxSize {
        romSize = maxSize
        romData = romData[:romSize]
    }

    // Count ROM byte occurrences
    var romCounts [256]uint32
    var inputCounts [256]uint32

    for i := int64(0); i < romSize; i++ {
        romCounts[romData[i]]++
    }

    // Read input file
    inputData, err := os.ReadFile(args[2+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening input file: %v\n", err)
        os.Exit(1)
    }

    // Count input character occurrences
    inputLength := len(inputData)
    charsUsed := 0

    for _, by := range inputData {
        inputCounts[by]++
    }

    // Count characters utilized
    for i := 0; i < 256; i++ {
        if inputCounts[i] > 0 {
            charsUsed++
        }
    }

    // Calculate ROM strength metrics
    generalStrength := 0.0
    fileStrength := 0.0

    for i := 0; i < 256; i++ {
        if romCounts[i] > 0 {
            generalStrength += math.Log10(float64(romCounts[i]))
        }
        if inputCounts[i] > 0 && romCounts[i] > 0 {
            fileStrength += float64(inputCounts[i]) * math.Log10(float64(romCounts[i]))
        }
    }

    utilisation := (float64(charsUsed) / 256.0) * 100.0

    fmt.Printf("ROM Strength Analysis (%d-bit)\n", bittage)
    fmt.Println("===============================\n")

    fmt.Println("Input Information:")
    fmt.Printf("- Text Length: %d characters\n", inputLength)
    fmt.Printf("- Characters Utilized: %d of 256 (%.1f%%)\n", charsUsed, utilisation)
    fmt.Println()

    fmt.Printf("General ROM Capacity: ~10^%.0f (", generalStrength)
    printLargeNumber(generalStrength)
    fmt.Println(")")

    fmt.Printf("This File Security: ~10^%.0f (", fileStrength)
    printLargeNumber(fileStrength)
    fmt.Println(")\n")

    fmt.Println("Byte Analysis:")
    fmt.Println("Byte  Dec  ROM Count  Input Count  Char")
    fmt.Println("----  ---  ---------  -----------  ----")

    for i := 0; i < 256; i++ {
        if romCounts[i] > 0 || inputCounts[i] > 0 {
            var display rune
            if i >= 32 && i <= 126 {
                display = rune(i)
            } else {
                display = ' '
            }
            fmt.Printf("0x%02X  %3d  %9d  %11d    %c\n",
                i, i, romCounts[i], inputCounts[i], display)
        }
    }
}