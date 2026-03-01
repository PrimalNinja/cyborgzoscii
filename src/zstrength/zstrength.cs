// Cyborg ZOSCII v20260301
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;

class ByteAddresses
{
    public uint[] ptrAddresses;
    public uint intCount;
}

class RomData
{
    public byte[] ptrROMData;
    public long lngROMSize;
    public uint[] arrROMCounts;
}

class Program
{
    private const int ZOSCII_ROM_LOAD_MAX = 131072;

    private static void PrintLargeNumber(double dblExponent_a)
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

    private static RomData LoadRom(string strFilename_a)
    {
        RomData ptrRom = null;
        
        try
        {
            using (FileStream ptrStream = new FileStream(strFilename_a, FileMode.Open, FileAccess.Read))
            {
                long lngLoad = Math.Min(ptrStream.Length, ZOSCII_ROM_LOAD_MAX);
                
                ptrRom = new RomData();
                ptrRom.lngROMSize = lngLoad;
                ptrRom.ptrROMData = new byte[lngLoad];
                ptrRom.arrROMCounts = new uint[256];
                
                ptrStream.Read(ptrRom.ptrROMData, 0, (int)lngLoad);
                
                // Count ROM byte occurrences
                for (long lngI = 0; lngI < ptrRom.lngROMSize; lngI++)
                {
                    ptrRom.arrROMCounts[ptrRom.ptrROMData[lngI]]++;
                }
            }
        }
        catch
        {
            ptrRom = null;
        }
        
        return ptrRom;
    }

    private static void UnloadRom(RomData ptrRom_a)
    {
        // In C#, garbage collector handles this, but method kept for symmetry
        ptrRom_a.ptrROMData = null;
        ptrRom_a.arrROMCounts = null;
        ptrRom_a.lngROMSize = 0;
    }

    private static bool AnalyzeFile(RomData ptrRom_a, string strInputFile_a)
    {
        bool blnSuccess = false;
        uint[] arrInputCounts = new uint[256];
        int intInputLength = 0;
        int intCharsUsed = 0;
        double dblGeneralStrength = 0.0;
        double dblFileStrength = 0.0;
        double dblUtilisation = 0.0;
        
        try
        {
            using (FileStream ptrInput = new FileStream(strInputFile_a, FileMode.Open, FileAccess.Read))
            {
                // Count input character occurrences
                int intCh;
                while ((intCh = ptrInput.ReadByte()) != -1)
                {
                    byte by = (byte)intCh;
                    arrInputCounts[by]++;
                    intInputLength++;
                }
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
            for (int intI = 0; intI < 256; intI++)
            {
                if (ptrRom_a.arrROMCounts[intI] > 0)
                {
                    dblGeneralStrength += Math.Log10(ptrRom_a.arrROMCounts[intI]);
                }
                if (arrInputCounts[intI] > 0 && ptrRom_a.arrROMCounts[intI] > 0)
                {
                    dblFileStrength += arrInputCounts[intI] * Math.Log10(ptrRom_a.arrROMCounts[intI]);
                }
            }
            
            dblUtilisation = (intCharsUsed / 256.0) * 100.0;
            
            Console.WriteLine("ROM Strength Analysis");
            Console.WriteLine("=====================");
            Console.WriteLine();
            
            Console.WriteLine("Input Information:");
            Console.WriteLine($"- Text Length: {intInputLength} characters");
            Console.WriteLine($"- Characters Utilized: {intCharsUsed} of 256 ({dblUtilisation:F1}%)");
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
                if (ptrRom_a.arrROMCounts[intI] > 0 || arrInputCounts[intI] > 0)
                {
                    char chDisplay = (intI >= 32 && intI <= 126) ? (char)intI : ' ';
                    Console.WriteLine($"0x{intI:X2}  {intI,3}  {ptrRom_a.arrROMCounts[intI],9}  {arrInputCounts[intI],11}    {chDisplay}");
                }
            }
            
            blnSuccess = true;
        }
        catch
        {
            blnSuccess = false;
        }
        
        return blnSuccess;
    }

    static void Main(string[] strArgs_a)
    {
        int intResult = 1;
        RomData ptrRom = null;
        bool blnAnalyzeOk = false;
        
        Console.WriteLine("ZOSCII ROM Strength Analyzer");
        Console.WriteLine("(c) 2026 Cyborg Unicorn Pty Ltd v20260301 - MIT License");
        Console.WriteLine();

        if (strArgs_a.Length == 2)
        {
            ptrRom = LoadRom(strArgs_a[0]);
            if (ptrRom != null)
            {
                blnAnalyzeOk = AnalyzeFile(ptrRom, strArgs_a[1]);
                
                if (blnAnalyzeOk)
                {
                    intResult = 0;
                }
                else
                {
                    Console.Error.WriteLine("Analysis failed");
                }
                
                UnloadRom(ptrRom);
            }
            else
            {
                Console.Error.WriteLine("Failed to load ROM");
            }
        }
        else
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} <romfile> <inputdatafile>");
        }
        
        Environment.Exit(intResult);
    }
}