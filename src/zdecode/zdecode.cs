// Cyborg ZOSCII v20260303
// (c) 2026 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

using System;
using System.IO;

class Program
{
    private const int ZOSCII_ROM_LOAD_MAX = 131072;

    private class RomData
    {
        public byte[] ptrROMData;
        public long lngROMSize;
    }

    private static RomData LoadRom(string strFilename_a)
    {
        RomData ptrRom = null;
        
        try
        {
            using (FileStream ptrStream = new FileStream(strFilename_a, FileMode.Open, FileAccess.Read))
            {
                ptrRom = new RomData();
                long lngLoad = Math.Min(ptrStream.Length, ZOSCII_ROM_LOAD_MAX);
                ptrRom.lngROMSize = lngLoad;
                ptrRom.ptrROMData = new byte[lngLoad];
                ptrStream.Read(ptrRom.ptrROMData, 0, (int)lngLoad);
            }
        }
        catch
        {
            ptrRom = null;
        }
        
        return ptrRom;
    }

    private static void FreeRom(RomData ptrRom_a)
    {
        // In C#, garbage collector handles this, but method kept for consistency
    }

    private static bool DecodeFile(RomData ptrRom_a, string strInputFile_a, string strOutputFile_a)
    {
        bool blnSuccess = false;
        
        try
        {
            using (FileStream ptrInput = new FileStream(strInputFile_a, FileMode.Open, FileAccess.Read))
            using (FileStream ptrOutput = new FileStream(strOutputFile_a, FileMode.Create, FileAccess.Write))
            {
                long lngInputSize = ptrInput.Length;
                byte[] arrBuf = new byte[2];
                long lngSlots = lngInputSize / 2;
                int intI = 0;
                
                if (lngSlots >= 0)
                {
                    // Decode each slot
                    for (intI = 0; intI < lngSlots; intI++)
                    {
                        if (ptrInput.Read(arrBuf, 0, 2) != 2)
                        {
                            break;
                        }
                        
                        ushort intAddr = BitConverter.ToUInt16(arrBuf, 0);
                        if (intAddr < ptrRom_a.lngROMSize)
                        {
                            ptrOutput.WriteByte(ptrRom_a.ptrROMData[intAddr]);
                        }
                    }
                    
                    if (intI == lngSlots)
                    {
                        blnSuccess = true;
                    }
                }
            }
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
        bool blnDecodeOk = false;
        
        Console.WriteLine("ZOSCII Decoder v20260303");
        Console.WriteLine("(c) 2026 Cyborg Unicorn Pty Ltd - MIT License");
        Console.WriteLine();

        if (strArgs_a.Length == 3)
        {
            ptrRom = LoadRom(strArgs_a[0]);
            if (ptrRom != null)
            {
                blnDecodeOk = DecodeFile(ptrRom, strArgs_a[1], strArgs_a[2]);
                FreeRom(ptrRom);
                
                if (blnDecodeOk)
                {
                    intResult = 0;
                }
                else
                {
                    Console.Error.WriteLine("Decode failed");
                }
            }
            else
            {
                Console.Error.WriteLine("Failed to load ROM");
            }
        }
        else
        {
            Console.Error.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} <romfile> <encoded> <output>");
        }
        
        Environment.Exit(intResult);
    }
}