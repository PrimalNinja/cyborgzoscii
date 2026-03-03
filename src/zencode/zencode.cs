// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;

struct ByteAddresses
{
    public uint[] ptrAddresses;
    public uint intCount;
}

struct RomData
{
    public byte[] ptrROMData;
    public long lngROMSize;
    public ByteAddresses[] arrLookup;
}

class Program
{
    private const int ZOSCII_ROM_LOAD_MAX = 131072;

	private static Random ptrRand;
	
    private static void BuildLookupTable(ref RomData ptrRom_a)
    {
        uint[] arrCounts = new uint[256];
        long lngROMSize = 0;
        
        // Initialize lookup array
        ptrRom_a.arrLookup = new ByteAddresses[256];
        for (int intI = 0; intI < 256; intI++)
        {
            ptrRom_a.arrLookup[intI].ptrAddresses = null;
            ptrRom_a.arrLookup[intI].intCount = 0;
        }
        
        // ROM addresses are 16-bit, so only use first 64KB
        lngROMSize = ptrRom_a.lngROMSize;
        if (lngROMSize > 65536L)
        {
            lngROMSize = 65536L;
        }
        
        // Count occurrences
        for (long lngI = 0; lngI < lngROMSize; lngI++)
        {
            arrCounts[ptrRom_a.ptrROMData[lngI]]++;
        }
        
        // Allocate memory for each byte value
        for (int intI = 0; intI < 256; intI++)
        {
            if (arrCounts[intI] > 0)
            {
                ptrRom_a.arrLookup[intI].ptrAddresses = new uint[arrCounts[intI]];
                ptrRom_a.arrLookup[intI].intCount = 0;
            }
        }
        
        // Fill addresses
        for (long lngI = 0; lngI < lngROMSize; lngI++)
        {
            byte by = ptrRom_a.ptrROMData[lngI];
            ptrRom_a.arrLookup[by].ptrAddresses[ptrRom_a.arrLookup[by].intCount++] = (uint)lngI;
        }
		
		// Seed Random based on ROM content
		uint romHash = 0;
		for (long lngI = 0; lngI < ptrRom_a.lngROMSize; lngI++)
		{
			romHash = (romHash * 33) + ptrRom_a.ptrROMData[lngI];
		}
		
		/* XOR with current time for per-run uniqueness */
		romHash ^= (uint)Environment.TickCount;
		
		ptrRand = new Random((int)romHash);
    }

    private static RomData LoadRom(string strFilename_a)
    {
        RomData ptrRom = new RomData();
        ptrRom.ptrROMData = null;
        ptrRom.lngROMSize = 0;
        ptrRom.arrLookup = null;
        
        try
        {
            using (FileStream ptrStream = new FileStream(strFilename_a, FileMode.Open, FileAccess.Read))
            {
                long lngLoad = Math.Min(ptrStream.Length, ZOSCII_ROM_LOAD_MAX);
                ptrRom.lngROMSize = lngLoad;
                ptrRom.ptrROMData = new byte[lngLoad];
                ptrStream.Read(ptrRom.ptrROMData, 0, (int)lngLoad);
                
                // Pre-build lookup table for reuse across multiple encodes
                BuildLookupTable(ref ptrRom);
            }
        }
        catch
        {
            ptrRom.ptrROMData = null;
            ptrRom.lngROMSize = 0;
            ptrRom.arrLookup = null;
        }
        
        return ptrRom;
    }

    private static void UnloadRom(ref RomData ptrRom_a)
    {
        // In C#, garbage collector handles this, but method kept for symmetry
        ptrRom_a.ptrROMData = null;
        ptrRom_a.arrLookup = null;
        ptrRom_a.lngROMSize = 0;
    }

    private static bool EncodeFile(ref RomData ptrRom_a, string strInputFile_a, string strOutputFile_a)
    {
        bool blnSuccess = false;
        FileStream ptrInput = null;
        FileStream ptrOutput = null;
        int intCh = 0;
        
        try
        {
            ptrInput = new FileStream(strInputFile_a, FileMode.Open, FileAccess.Read);
            
            try
            {
                ptrOutput = new FileStream(strOutputFile_a, FileMode.Create, FileAccess.Write);
                
                // Stream-encode input
                intCh = ptrInput.ReadByte();
                while (intCh != -1)
                {
                    byte by = (byte)intCh;
                    if (ptrRom_a.arrLookup[by].intCount > 0)
                    {
                        uint intRandomIdx = (uint)ptrRand.Next((int)ptrRom_a.arrLookup[by].intCount);
                        ushort intAddress = (ushort)ptrRom_a.arrLookup[by].ptrAddresses[intRandomIdx];
                        ptrOutput.Write(BitConverter.GetBytes(intAddress), 0, 2);
                    }
                    intCh = ptrInput.ReadByte();
                }
                
                blnSuccess = true;
            }
            finally
            {
                if (ptrOutput != null)
                {
                    ptrOutput.Close();
                }
            }
        }
        finally
        {
            if (ptrInput != null)
            {
                ptrInput.Close();
            }
        }
        
        return blnSuccess;
    }

    static void Main(string[] strArgs_a)
    {
        int intResult = 1;
        RomData ptrRom = new RomData();
        bool blnEncodeOk = false;
        
        Console.WriteLine("ZOSCII Encoder v20260303");
        Console.WriteLine("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License");
        Console.WriteLine();

        if (strArgs_a.Length == 3)
        {
            ptrRom = LoadRom(strArgs_a[0]);
            if (ptrRom.ptrROMData != null)
            {
                blnEncodeOk = EncodeFile(ref ptrRom, strArgs_a[1], strArgs_a[2]);
                
                if (blnEncodeOk)
                {
                    intResult = 0;
                }
                else
                {
                    Console.Error.WriteLine("Encode failed");
                }
                
                UnloadRom(ref ptrRom);
            }
            else
            {
                Console.Error.WriteLine("Failed to load ROM");
            }
        }
        else
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} <romfile> <inputdatafile> <encodedoutput>");
        }
        
        Environment.Exit(intResult);
    }
}