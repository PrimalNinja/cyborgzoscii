// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
    static void Main(string[] arrArgs_a)
    {
        int intBittage = 16; // default
        int intOffset = 0;

		Console.WriteLine("ZOSCII Decoder");
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
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <encodedinput> <outputdatafile>");
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

        // Process input file and write output
        try
        {
            using (FileStream fInput = new FileStream(arrArgs_a[1 + intOffset], FileMode.Open, FileAccess.Read))
            using (FileStream fOutput = new FileStream(arrArgs_a[2 + intOffset], FileMode.Create, FileAccess.Write))
            {
                if (intBittage == 16)
                {
                    byte[] arrBuffer = new byte[sizeof(ushort)];
                    while (fInput.Read(arrBuffer, 0, sizeof(ushort)) == sizeof(ushort))
                    {
                        ushort intAddress = BitConverter.ToUInt16(arrBuffer, 0);
                        if (intAddress < lngROMSize)
                        {
                            fOutput.WriteByte(pROMData[intAddress]);
                        }
                    }
                }
                else
                {
                    byte[] arrBuffer = new byte[sizeof(uint)];
                    while (fInput.Read(arrBuffer, 0, sizeof(uint)) == sizeof(uint))
                    {
                        uint intAddress = BitConverter.ToUInt32(arrBuffer, 0);
                        if (intAddress < lngROMSize)
                        {
                            fOutput.WriteByte(pROMData[intAddress]);
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
