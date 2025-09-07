// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
    static void PrintLargeNumber(double exponent)
    {
        if (exponent < 3)
        {
            Console.Write($"~{Math.Pow(10, exponent):F0} permutations");
        }
        else if (exponent < 6)
        {
            Console.Write($"~{Math.Pow(10, exponent) / 1000.0:F1} thousand permutations");
        }
        else if (exponent < 9)
        {
            Console.Write($"~{Math.Pow(10, exponent) / 1000000.0:F1} million permutations");
        }
        else if (exponent < 12)
        {
            Console.Write($"~{Math.Pow(10, exponent) / 1000000000.0:F1} billion permutations");
        }
        else if (exponent < 15)
        {
            Console.Write($"~{Math.Pow(10, exponent) / 1000000000000.0:F1} trillion permutations");
        }
        else if (exponent < 82)
        {
            Console.Write($"More than all atoms in the observable universe (10^{exponent:F0} permutations)");
        }
        else if (exponent < 1000)
        {
            Console.Write($"Incomprehensibly massive (10^{exponent:F0} permutations)");
        }
        else
        {
            Console.Write($"Astronomically secure (10^{exponent / 1000000.0:F1}M permutations)");
        }
    }

    static void Main(string[] args)
    {
        int bitWidth = 16; // default
        int argOffset = 0;

        if (args.Length >= 1 && args[0] == "-32")
        {
            bitWidth = 32;
            argOffset = 1;
        }
        else if (args.Length >= 1 && args[0] == "-16")
        {
            bitWidth = 16;
            argOffset = 1;
        }

        if (args.Length != 2 + argOffset)
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <inputdatafile>");
            Environment.Exit(1);
        }

        // Read ROM file
        byte[] romData;
        try
        {
            romData = File.ReadAllBytes(args[0 + argOffset]);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error opening ROM file: {ex.Message}");
            Environment.Exit(1);
        }

        long romSize = romData.Length;
        long maxSize = bitWidth == 16 ? 65536 : 4294967296L;
        if (romSize > maxSize)
        {
            Array.Resize(ref romData, (int)maxSize);
            romSize = maxSize;
        }

        // Count ROM byte occurrences
        uint[] romCounts = new uint[256];
        uint[] inputCounts = new uint[256];

        for (long i = 0; i < romSize; i++)
        {
            romCounts[romData[i]]++;
        }

        // Count input character occurrences
        int inputLength = 0;
        int charactersUsed = 0;

        try
        {
            using (FileStream inputFile = new FileStream(args[1 + argOffset], FileMode.Open, FileAccess.Read))
            {
                int c;
                while ((c = inputFile.ReadByte()) != -1)
                {
                    byte b = (byte)c;
                    inputCounts[b]++;
                    inputLength++;
                }
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error opening input file: {ex.Message}");
            Environment.Exit(1);
        }

        // Count characters utilized
        for (int i = 0; i < 256; i++)
        {
            if (inputCounts[i] > 0)
            {
                charactersUsed++;
            }
        }

        // Calculate ROM strength metrics
        double generalStrength = 0.0;
        double fileStrength = 0.0;

        for (int i = 0; i < 256; i++)
        {
            if (romCounts[i] > 0)
            {
                generalStrength += Math.Log10(romCounts[i]);
            }
            if (inputCounts[i] > 0 && romCounts[i] > 0)
            {
                fileStrength += inputCounts[i] * Math.Log10(romCounts[i]);
            }
        }

        double utilization = (charactersUsed / 256.0) * 100.0;

        Console.WriteLine($"ROM Strength Analysis ({bitWidth}-bit)");
        Console.WriteLine("===============================");
        Console.WriteLine();
        Console.WriteLine("Input Information:");
        Console.WriteLine($"- Text Length: {inputLength} characters");
        Console.WriteLine($"- Characters Utilized: {charactersUsed} of 256 ({utilization:F1}%)");
        Console.WriteLine();

        Console.Write($"General ROM Capacity: ~10^{generalStrength:F0} (");
        PrintLargeNumber(generalStrength);
        Console.WriteLine(")");

        Console.Write($"This File Security: ~10^{fileStrength:F0} (");
        PrintLargeNumber(fileStrength);
        Console.WriteLine(")");
        Console.WriteLine();

        Console.WriteLine("Byte Analysis:");
        Console.WriteLine("Byte  Dec  ROM Count  Input Count  Char");
        Console.WriteLine("----  ---  ---------  -----------  ----");

        for (int i = 0; i < 256; i++)
        {
            if (romCounts[i] > 0 || inputCounts[i] > 0)
            {
                char displayChar = (i >= 32 && i <= 126) ? (char)i : ' ';
                Console.WriteLine($"0x{i:X2}  {i,3}  {romCounts[i],9}  {inputCounts[i],11}    {displayChar}");
            }
        }
    }
}