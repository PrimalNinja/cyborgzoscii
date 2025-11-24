// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
    "encoding/binary"
    "fmt"
    "math/rand"
    "os"
    "time"
)

type ByteAddresses struct {
    addresses []uint32
    count     uint32
}

func main() {
    fmt.Println("ZOSCII Encoder")
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

    if len(args) != 4+offset {
        fmt.Fprintf(os.Stderr, "Usage: %s [-16|-32] <romfile> <inputdatafile> <encodedoutput>\n", args[0])
        os.Exit(1)
    }

    rand.Seed(time.Now().UnixNano())

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

    // Build address lookup tables
    var lookup [256]ByteAddresses
    var romCounts [256]uint32

    // Count occurrences
    for i := int64(0); i < romSize; i++ {
        romCounts[romData[i]]++
    }

    // Allocate address arrays
    for i := 0; i < 256; i++ {
        lookup[i].addresses = make([]uint32, 0, romCounts[i])
    }

    // Populate address arrays
    for i := int64(0); i < romSize; i++ {
        by := romData[i]
        lookup[by].addresses = append(lookup[by].addresses, uint32(i))
        lookup[by].count++
    }

    // Read input file
    inputData, err := os.ReadFile(args[2+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening input file: %v\n", err)
        os.Exit(1)
    }

    // Create output file
    fOutput, err := os.Create(args[3+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening output file: %v\n", err)
        os.Exit(1)
    }
    defer fOutput.Close()

    // Encode data
    for _, by := range inputData {
        if lookup[by].count > 0 {
            randomIdx := rand.Intn(int(lookup[by].count))
            address := lookup[by].addresses[randomIdx]

            if bittage == 16 {
                address16 := uint16(address)
                binary.Write(fOutput, binary.LittleEndian, address16)
            } else {
                binary.Write(fOutput, binary.LittleEndian, address)
            }
        }
    }
}