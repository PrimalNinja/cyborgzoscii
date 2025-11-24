// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.
// Windows & Linux Version

package main

import (
    "encoding/binary"
    "fmt"
    "io"
    "os"
)

func main() {
    fmt.Println("ZOSCII Decoder")
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
        fmt.Fprintf(os.Stderr, "Usage: %s [-16|-32] <romfile> <encodedinput> <outputdatafile>\n", args[0])
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

    // Open encoded input file
    fInput, err := os.Open(args[2+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening encoded input file: %v\n", err)
        os.Exit(1)
    }
    defer fInput.Close()

    // Create output file
    fOutput, err := os.Create(args[3+offset])
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening output file: %v\n", err)
        os.Exit(1)
    }
    defer fOutput.Close()

    // Decode data
    if bittage == 16 {
        var address16 uint16
        for {
            err := binary.Read(fInput, binary.LittleEndian, &address16)
            if err == io.EOF {
                break
            }
            if err != nil {
                fmt.Fprintf(os.Stderr, "Error reading input: %v\n", err)
                os.Exit(1)
            }
            
            if int64(address16) < romSize {
                fOutput.Write([]byte{romData[address16]})
            }
        }
    } else {
        var address uint32
        for {
            err := binary.Read(fInput, binary.LittleEndian, &address)
            if err == io.EOF {
                break
            }
            if err != nil {
                fmt.Fprintf(os.Stderr, "Error reading input: %v\n", err)
                os.Exit(1)
            }
            
            if int64(address) < romSize {
                fOutput.Write([]byte{romData[address]})
            }
        }
    }
}