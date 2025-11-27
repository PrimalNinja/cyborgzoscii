// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// C# Version - Library Implementation

using System;
using System.Diagnostics;

namespace CyborgUnicorn.Zoscii
{
    /// <summary>
    /// Structure to represent memory blocks
    /// </summary>
    public struct MemoryBlock
    {
        public int Start;
        public int Size;

        public MemoryBlock(int start, int size)
        {
            Start = start;
            Size = size;
        }
    }

    /// <summary>
    /// Structure to represent the result
    /// </summary>
    public class ZosciiResult
    {
        public int[] Addresses { get; set; }
        public int AddressCount { get; set; }
        public int[] InputCounts { get; set; }
        public int[] RomCounts { get; set; }

        public ZosciiResult()
        {
            Addresses = Array.Empty<int>();
            InputCounts = new int[256];
            RomCounts = new int[256];
        }
    }

    /// <summary>
    /// Delegate type for character conversion functions
    /// </summary>
    public delegate int ConverterFunc(int charCode, int unmappableChar);

    /// <summary>
    /// ZOSCII Encoder class
    /// </summary>
    public static class ZosciiEncoder
    {
        private static readonly Random random = new Random();

        /// <summary>
        /// Function to convert string to ZOSCII address sequence
        /// </summary>
        public static ZosciiResult ToZoscii(byte[] arrBinaryData, string strInputString,
                                           MemoryBlock[] arrMemoryBlocks,
                                           ConverterFunc cbConverter = null,
                                           int intUnmappableChar = -1)
        {
            Stopwatch intStartTime = Stopwatch.StartNew();

            int intI;
            int intBlock;
            int intResultIndex = 0;
            int intResultCount = 0;
            int intDebugMissing = 0;

            int[] arrByteCounts = new int[256];
            int[][] arrByteAddresses = new int[256][];
            int[] arrOffsets = new int[256];
            int[] arrInputCounts = new int[256];
            int intAddress;
            int intByte;
            int intIndex;
            MemoryBlock objBlock;

            // Initialize byte address arrays to null
            for (intI = 0; intI < 256; intI++)
            {
                arrByteAddresses[intI] = null;
            }

            // Pass 1: Count occurrences by iterating through blocks
            for (intBlock = 0; intBlock < arrMemoryBlocks.Length; intBlock++)
            {
                objBlock = arrMemoryBlocks[intBlock];
                for (intAddress = objBlock.Start; intAddress < (objBlock.Start + objBlock.Size); intAddress++)
                {
                    intByte = arrBinaryData[intAddress];
                    arrByteCounts[intByte]++;
                }
            }

            // Pass 2: Pre-allocate exact-sized arrays
            for (intI = 0; intI < 256; intI++)
            {
                if (arrByteCounts[intI] > 0)
                {
                    arrByteAddresses[intI] = new int[arrByteCounts[intI]];
                }
            }

            // Pass 3: Populate arrays by iterating through blocks
            for (intBlock = 0; intBlock < arrMemoryBlocks.Length; intBlock++)
            {
                objBlock = arrMemoryBlocks[intBlock];
                for (intAddress = objBlock.Start; intAddress < (objBlock.Start + objBlock.Size); intAddress++)
                {
                    intByte = arrBinaryData[intAddress];
                    arrByteAddresses[intByte][arrOffsets[intByte]] = intAddress;
                    arrOffsets[intByte]++;
                }
            }

            int strLength = strInputString.Length;

            // Count valid characters for result array size
            for (intI = 0; intI < strLength; intI++)
            {
                intIndex = (byte)strInputString[intI];
                if (cbConverter != null)
                {
                    intIndex = cbConverter(intIndex, intUnmappableChar);
                }
                if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] != null && arrByteCounts[intIndex] > 0)
                {
                    intResultCount++;
                }
                else
                {
                    intDebugMissing++;
                    if (intDebugMissing <= 10)
                    {
                        Console.WriteLine($"Missing character: '{strInputString[intI]}' (code {(byte)strInputString[intI]} -> {intIndex})");
                    }
                }
            }

            Console.WriteLine($"Characters found in ROM: {intResultCount}");
            Console.WriteLine($"Characters missing from ROM: {intDebugMissing}");

            int[] arrResult = new int[intResultCount];

            for (intI = 0; intI < strLength; intI++)
            {
                intIndex = (byte)strInputString[intI];
                if (cbConverter != null)
                {
                    intIndex = cbConverter(intIndex, intUnmappableChar);
                }

                if (intIndex >= 0 && intIndex < 256 && arrByteAddresses[intIndex] != null && arrByteCounts[intIndex] > 0)
                {
                    arrInputCounts[intIndex]++;
                    int intRandomPick = random.Next(arrByteCounts[intIndex]);
                    arrResult[intResultIndex] = arrByteAddresses[intIndex][intRandomPick];
                    intResultIndex++;
                }
            }

            intStartTime.Stop();
            double intElapsedMs = intStartTime.Elapsed.TotalMilliseconds;

            Console.WriteLine("ZOSCII Performance:");
            Console.WriteLine($"- Input length: {strLength} chars");
            Console.WriteLine($"- Memory blocks: {arrMemoryBlocks.Length}");
            Console.WriteLine($"- Execution time: {intElapsedMs:F2}ms");
            Console.WriteLine($"- Output addresses: {intResultCount}");

            // Prepare result structure
            ZosciiResult result = new ZosciiResult
            {
                Addresses = arrResult,
                AddressCount = intResultCount,
                InputCounts = new int[256],
                RomCounts = new int[256]
            };

            Array.Copy(arrInputCounts, result.InputCounts, 256);
            Array.Copy(arrByteCounts, result.RomCounts, 256);

            return result;
        }

        /// <summary>
        /// Function to convert PETSCII character codes to ASCII character codes
        /// </summary>
        public static int PetsciiToAscii(int intPetsciiChar, int intUnmappableChar)
        {
            int[] arrPetsciiToAsciiMap = new int[256]
            {
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
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
            };

            if (intPetsciiChar < 0 || intPetsciiChar > 255)
            {
                return intUnmappableChar;
            }

            int result = arrPetsciiToAsciiMap[intPetsciiChar];
            return (result == -1) ? intUnmappableChar : result;
        }

        /// <summary>
        /// Function to convert EBCDIC character codes to ASCII character codes
        /// </summary>
        public static int EbcdicToAscii(int intEbcdicChar, int intUnmappableChar)
        {
            int[] arrEbcdicToAsciiMap = new int[256]
            {
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
                -1, -1, -1, -1, -1, -1
            };

            if (intEbcdicChar < 0 || intEbcdicChar > 255)
            {
                return intUnmappableChar;
            }

            int result = arrEbcdicToAsciiMap[intEbcdicChar];
            return (result == -1) ? intUnmappableChar : result;
        }
    }
}