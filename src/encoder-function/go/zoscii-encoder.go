// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Package zoscii provides ZOSCII encoding functionality
package zoscii

import (
	"fmt"
	"math/rand"
	"time"
)

// MemoryBlock represents a block of memory in the ROM
type MemoryBlock struct {
	Start int
	Size  int
}

// Result contains the encoded addresses and statistics
type Result struct {
	Addresses   []int
	InputCounts [256]int
	ROMCounts   [256]int
}

// ConverterFunc is a function type for character conversion
type ConverterFunc func(charCode int, unmappableChar int) int

// ToZOSCII converts a string to ZOSCII address sequence
func ToZOSCII(binaryData []byte, inputString string, memoryBlocks []MemoryBlock, converter ConverterFunc, unmappableChar int) Result {
	startTime := time.Now()

	var byteCounts [256]int
	var byteAddresses [256][]int
	var inputCounts [256]int
	resultCount := 0
	debugMissing := 0

	// Pass 1: Count occurrences by iterating through blocks
	for _, block := range memoryBlocks {
		end := block.Start + block.Size
		if end > len(binaryData) {
			end = len(binaryData)
		}
		for address := block.Start; address < end; address++ {
			byteCounts[binaryData[address]]++
		}
	}

	// Pass 2: Pre-allocate exact-sized slices
	for i := 0; i < 256; i++ {
		if byteCounts[i] > 0 {
			byteAddresses[i] = make([]int, 0, byteCounts[i])
		}
	}

	// Pass 3: Populate arrays by iterating through blocks
	for _, block := range memoryBlocks {
		end := block.Start + block.Size
		if end > len(binaryData) {
			end = len(binaryData)
		}
		for address := block.Start; address < end; address++ {
			byteVal := binaryData[address]
			byteAddresses[byteVal] = append(byteAddresses[byteVal], address)
		}
	}

	// Count valid characters for result array size
	for i := 0; i < len(inputString); i++ {
		index := int(inputString[i])
		if converter != nil {
			index = converter(index, unmappableChar)
		}
		if index >= 0 && index < 256 && len(byteAddresses[index]) > 0 {
			resultCount++
		} else {
			debugMissing++
			if debugMissing <= 10 {
				fmt.Printf("Missing character: '%c' (code %d -> %d)\n", inputString[i], int(inputString[i]), index)
			}
		}
	}

	fmt.Printf("Characters found in ROM: %d\n", resultCount)
	fmt.Printf("Characters missing from ROM: %d\n", debugMissing)

	// Build result array with random addresses
	addresses := make([]int, 0, resultCount)

	for i := 0; i < len(inputString); i++ {
		index := int(inputString[i])
		if converter != nil {
			index = converter(index, unmappableChar)
		}

		if index >= 0 && index < 256 && len(byteAddresses[index]) > 0 {
			inputCounts[index]++
			randomPick := rand.Intn(len(byteAddresses[index]))
			addresses = append(addresses, byteAddresses[index][randomPick])
		}
	}

	elapsed := time.Since(startTime)

	fmt.Println("ZOSCII Performance:")
	fmt.Printf("- Input length: %d chars\n", len(inputString))
	fmt.Printf("- Memory blocks: %d\n", len(memoryBlocks))
	fmt.Printf("- Execution time: %.2fms\n", float64(elapsed.Microseconds())/1000.0)
	fmt.Printf("- Output addresses: %d\n", len(addresses))

	return Result{
		Addresses:   addresses,
		InputCounts: inputCounts,
		ROMCounts:   byteCounts,
	}
}

// PetsciiToAscii converts PETSCII character codes to ASCII
func PetsciiToAscii(petsciiChar int, unmappableChar int) int {
	petsciiMap := [256]int{
		// 0-31: Control characters
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

		// 32-63: Space, digits, punctuation (direct ASCII mapping)
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

		// 64-95: @A-Z[\]^_ (direct ASCII mapping)
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

		// 96-255: Everything else mapped to unmappable
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	}

	if petsciiChar < 0 || petsciiChar > 255 {
		return unmappableChar
	}

	result := petsciiMap[petsciiChar]
	if result == -1 {
		return unmappableChar
	}
	return result
}

// EbcdicToAscii converts EBCDIC character codes to ASCII
func EbcdicToAscii(ebcdicChar int, unmappableChar int) int {
	ebcdicMap := [256]int{
		// 0-63: Control/special
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

		// 64-79: Space and some punctuation
		32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 46, 60, 40, 43, 124,

		// 80-95: & and punctuation
		38, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, 36, -1, 41, 59, -1,

		// 96-111: - and punctuation
		45, 47, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 37, 95, 62, 63,

		// 112-127: More punctuation
		-1, -1, -1, -1, -1, -1, -1, -1, -1, 96, 58, 35, 64, 39, 61, 34,

		// 128: Control
		-1,

		// 129-137: a-i
		97, 98, 99, 100, 101, 102, 103, 104, 105,

		// 138-144: Control/special
		-1, -1, -1, -1, -1, -1, -1,

		// 145-153: j-r
		106, 107, 108, 109, 110, 111, 112, 113, 114,

		// 154-161: Control/special
		-1, -1, -1, -1, -1, -1, -1, -1,

		// 162-169: s-z
		115, 116, 117, 118, 119, 120, 121, 122,

		// 170-192: Control/special
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1,

		// 193-201: A-I
		65, 66, 67, 68, 69, 70, 71, 72, 73,

		// 202-208: Control/special
		-1, -1, -1, -1, -1, -1, -1,

		// 209-217: J-R
		74, 75, 76, 77, 78, 79, 80, 81, 82,

		// 218-225: Control/special
		-1, -1, -1, -1, -1, -1, -1, -1,

		// 226-233: S-Z
		83, 84, 85, 86, 87, 88, 89, 90,

		// 234-239: Control/special
		-1, -1, -1, -1, -1, -1,

		// 240-249: 0-9
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57,

		// 250-255: Control/special
		-1, -1, -1, -1, -1, -1,
	}

	if ebcdicChar < 0 || ebcdicChar > 255 {
		return unmappableChar
	}

	result := ebcdicMap[ebcdicChar]
	if result == -1 {
		return unmappableChar
	}
	return result
}