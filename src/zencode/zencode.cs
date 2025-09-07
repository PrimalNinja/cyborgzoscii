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
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} [-16|-32] <romfile> <inputdatafile> <encodedoutput>");
            Environment.Exit(1);
        }

        Random rand = new Random();

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

        // Build address lookup tables
        ByteAddresses[] lookup = new ByteAddresses[256];
        uint[] counts = new uint[256];

        // Initialize lookup arrays
        for (int i = 0; i < 256; i++)
        {
            lookup[i] = new ByteAddresses();
        }

        // Count occurrences
        for (long i = 0; i < romSize; i++)
        {
            counts[romData[i]]++;
        }

        // Allocate address arrays
        for (int i = 0; i < 256; i++)
        {
            lookup[i].Addresses = new uint[counts[i]];
            lookup[i].Count = 0;
        }

        // Populate address arrays
        for (long i = 0; i < romSize; i++)
        {
            byte b = romData[i];
            lookup[b].Addresses[lookup[b].Count++] = (uint)i;
        }

        // Process input file and write output
        try
        {
            using (FileStream inputFile = new FileStream(args[1 + argOffset], FileMode.Open, FileAccess.Read))
            using (FileStream outputFile = new FileStream(args[2 + argOffset], FileMode.Create, FileAccess.Write))
            {
                int c;
                while ((c = inputFile.ReadByte()) != -1)
                {
                    byte b = (byte)c;
                    if (lookup[b].Count > 0)
                    {
                        uint randomIdx = (uint)rand.Next((int)lookup[b].Count);
                        uint address = lookup[b].Addresses[randomIdx];

                        if (bitWidth == 16)
                        {
                            ushort addr16 = (ushort)address;
                            byte[] buffer = BitConverter.GetBytes(addr16);
                            outputFile.Write(buffer, 0, sizeof(ushort));
                        }
                        else
                        {
                            byte[] buffer = BitConverter.GetBytes(address);
                            outputFile.Write(buffer, 0, sizeof(uint));
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