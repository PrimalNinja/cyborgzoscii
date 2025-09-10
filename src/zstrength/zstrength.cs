// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
    static void PrintLargeNumber(double dblExponent_a)
    {
        if (dblExponent_a < 3)
        {
            Console.Write($"~{Math.Pow(10, dblExponent_a):F0} permutations");
        }
        else if (dblExponent_a < 6)
        {
            Console.Write($"~{Math.Pow(10, dblExponent_a) / 1000.0:F1} thousand permutations");
        }
        else if (dblExponent_a < 9)
        {
            Console.Write($"~{Math.Pow(10, dblExponent_a) / 1000000.0:F1} million permutations");
        }
        else if (dblExponent_a < 12)
        {
            Console.Write($"~{Math.Pow(10, dblExponent_a) / 1000000000.0:F1} billion permutations");
        }
        else if (dblExponent_a < 15)
        {
            Console.Write($"~{Math.Pow(10, dblExponent_a) / 1000000000000.0:F1} trillion permutations");
        }
        else if (dblExponent_a < 82)
        {
            Console.Write($"More than all atoms in the observable universe (10^{dblExponent_a:F0} permutations)");
        }
        else if (dblExponent_a < 1000)
        {
            Console.Write($"Incomprehensibly massive (10^{dblExponent_a:F0} permutations)");
        }
        else
        {
            Console.Write($"Astronomically secure (10^{dblExponent_a / 1000000.0:F1}M permutations)");
        }
    }

    static void Main(string[] arrArgs_a)
    {
        int intBittage = 16; // default
        int intOffset = 0;

		Console.Write("ZOSCII ROM Strength Analyzer");
		Console.Write("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License");

        if (arrArgs_a.Length >= 1 && arrArgs_a[0] == "-32")
        {
            intBittage = 32;
            intOffset = 1;
        }
        else if (arrArgs_a.Length >= 1 && arrArgs_a[0] == "-16")
        {
            intBittage = 16;
            intOffset = 1;
        }

        if (arrArgs_a.Length != 2 + intOffset)
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <inputdatafile>");
            Environment.Exit(1);
        }

        // Read ROM file
        byte[] pROMData;
        try
        {
            pROMData = File.ReadAllBytes(arrArgs_a[0 + intOffset]);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error opening ROM file: {ex.Message}");
            Environment.Exit(1);
        }

        long lngROMSize = pROMData.Length;
        long lngMaxSize = intBittage == 16 ? 65536 : 4294967296L;
        if (lngROMSize > lngMaxSize)
        {
            Array.Resize(ref pROMData, (int)lngMaxSize);
            lngROMSize = lngMaxSize;
        }

        // Count ROM byte occurrences
        uint[] arrROMCounts = new uint[256];
        uint[] arrInputCounts = new uint[256];

        for (long lngI = 0; lngI < lngROMSize; lngI++)
        {
            arrROMCounts[pROMData[lngI]]++;
        }

        // Count input character occurrences
        int intInputLength = 0;
        int intCharsUsed = 0;

        try
        {
            using (FileStream fInput = new FileStream(arrArgs_a[1 + intOffset], FileMode.Open, FileAccess.Read))
            {
                int ch;
                while ((ch = fInput.ReadByte()) != -1)
                {
                    byte by = (byte)ch;
                    arrInputCounts[by]++;
                    intInputLength++;
                }
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error opening input file: {ex.Message}");
            Environment.Exit(1);
        }

        // Count characters utilized
        for (int intI = 0; intI < 256; intI++)
        {
            if (arrInputCounts[intI] > 0)
            {
                intCharsUsed++;
            }
        }

        // Calculate ROM strength metrics
        double dblGeneralStrength = 0.0;
        double dblFileStrength = 0.0;

        for (int intI = 0; intI < 256; intI++)
        {
            if (arrROMCounts[intI] > 0)
            {
                dblGeneralStrength += Math.Log10(arrROMCounts[intI]);
            }
            if (arrInputCounts[intI] > 0 && arrROMCounts[intI] > 0)
            {
                dblFileStrength += arrInputCounts[intI] * Math.Log10(arrROMCounts[intI]);
            }
        }

        double utilization = (intCharsUsed / 256.0) * 100.0;

        Console.WriteLine($"ROM Strength Analysis ({intBittage}-bit)");
        Console.WriteLine("===============================");
        Console.WriteLine();
        Console.WriteLine("Input Information:");
        Console.WriteLine($"- Text Length: {intInputLength} characters");
        Console.WriteLine($"- Characters Utilized: {intCharsUsed} of 256 ({utilization:F1}%)");
        Console.WriteLine();

        Console.Write($"General ROM Capacity: ~10^{dblGeneralStrength:F0} (");
        PrintLargeNumber(dblGeneralStrength);
        Console.WriteLine(")");

        Console.Write($"This File Security: ~10^{dblFileStrength:F0} (");
        PrintLargeNumber(dblFileStrength);
        Console.WriteLine(")");
        Console.WriteLine();

        Console.WriteLine("Byte Analysis:");
        Console.WriteLine("Byte  Dec  ROM Count  Input Count  Char");
        Console.WriteLine("----  ---  ---------  -----------  ----");

        for (int intI = 0; intI < 256; intI++)
        {
            if (arrROMCounts[intI] > 0 || arrInputCounts[intI] > 0)
            {
                char chDisplay = (intI >= 32 && intI <= 126) ? (char)intI : ' ';
                Console.WriteLine($"0x{intI:X2}  {intI,3}  {arrROMCounts[intI],9}  {arrInputCounts[intI],11}    {chDisplay}");
            }
        }
    }
}
