// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Windows & Linux Version

using System;
using System.IO;
using System.Runtime.InteropServices;

struct ByteAddresses
{
    public uint[] Addresses;
    public uint Count;
}

class Program
{
    static void Main(string[] arrArgs_a)
    {
        int intBittage = 16; // default
        int intOffset = 0;

		Console.WriteLine("ZOSCII Encoder");
		Console.WriteLine("(c) 2025 Cyborg Unicorn Pty Ltd - MIT License");

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

        if (arrArgs_a.Length != 3 + intOffset)
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <inputdatafile> <encodedoutput>");
            Environment.Exit(1);
        }

        Random objRand = new Random();

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

        // Build address lookup tables
        ByteAddresses[] arrLookup = new ByteAddresses[256];
        uint[] arrROMCounts = new uint[256];

        // Initialize lookup arrays
        for (int intI = 0; intI < 256; intI++)
        {
            arrLookup[intI] = new ByteAddresses();
        }

        // Count occurrences
        for (long lngI = 0; lngI < lngROMSize; lngI++)
        {
            arrROMCounts[pROMData[lngI]]++;
        }

        // Allocate address arrays
        for (int intI = 0; intI < 256; intI++)
        {
            arrLookup[intI].Addresses = new uint[arrROMCounts[intI]];
            arrLookup[intI].Count = 0;
        }

        // Populate address arrays
        for (long lngI = 0; lngI < lngROMSize; lngI++)
        {
            byte by = pROMData[lngI];
            arrLookup[by].Addresses[arrLookup[by].Count++] = (uint)lngI;
        }

        // Process input file and write output
        try
        {
            using (FileStream fInput = new FileStream(arrArgs_a[1 + intOffset], FileMode.Open, FileAccess.Read))
            using (FileStream fOutput = new FileStream(arrArgs_a[2 + intOffset], FileMode.Create, FileAccess.Write))
            {
                int ch;
                while ((ch = fInput.ReadByte()) != -1)
                {
                    byte by = (byte)ch;
                    if (arrLookup[by].Count > 0)
                    {
                        uint intRandomIdx = (uint)objRand.Next((int)arrLookup[by].Count);
                        uint intAddress = arrLookup[by].Addresses[intRandomIdx];

                        if (intBittage == 16)
                        {
                            ushort intAddress16 = (ushort)intAddress;
                            byte[] arrBuffer = BitConverter.GetBytes(intAddress16);
                            fOutput.Write(arrBuffer, 0, sizeof(ushort));
                        }
                        else
                        {
                            byte[] arrBuffer = BitConverter.GetBytes(intAddress);
                            fOutput.Write(arrBuffer, 0, sizeof(uint));
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error processing files: {ex.Message}");
            Environment.Exit(1);
        }
    }
}
