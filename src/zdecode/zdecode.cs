// Cyborg ZOSCII v20250908
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
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

        if (args.Length != 3 + argOffset)
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <encodedinput> <outputdatafile>");
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

        // Process input file and write output
        try
        {
            using (FileStream inputFile = new FileStream(args[1 + argOffset], FileMode.Open, FileAccess.Read))
            using (FileStream outputFile = new FileStream(args[2 + argOffset], FileMode.Create, FileAccess.Write))
            {
                if (bitWidth == 16)
                {
                    byte[] buffer = new byte[sizeof(ushort)];
                    while (inputFile.Read(buffer, 0, sizeof(ushort)) == sizeof(ushort))
                    {
                        ushort address = BitConverter.ToUInt16(buffer, 0);
                        if (address < romSize)
                        {
                            outputFile.WriteByte(romData[address]);
                        }
                    }
                }
                else
                {
                    byte[] buffer = new byte[sizeof(uint)];
                    while (inputFile.Read(buffer, 0, sizeof(uint)) == sizeof(uint))
                    {
                        uint address = BitConverter.ToUInt32(buffer, 0);
                        if (address < romSize)
                        {
                            outputFile.WriteByte(romData[address]);
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