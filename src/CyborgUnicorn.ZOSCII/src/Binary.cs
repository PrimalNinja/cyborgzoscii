// CyborgUnicorn.ZOSCII - Binary
// BVerify, BSplit, BJoin
// Format-agnostic binary comparison and PENTAGONE 3-of-5 redundancy
// Compatible with uverify.c / usplit.c / ujoin.c
// (c) 2026 Cyborg Unicorn Pty Ltd - MIT License

using System;
using System.Collections.Generic;
using System.IO;

namespace CyborgUnicorn.ZOSCII
{
// -------------------------------------------------------------------------

    /// <summary>
    /// Plain binary file and byte array comparison.
    /// Equivalent to the binary compare mode of uverify.c.
    /// </summary>
    public static class BVerify
    {
        /// <summary>
        /// Compare two files byte-for-byte. Returns true if identical.
        /// </summary>
        public static bool File(string strFile1_a, string strFile2_a)
        {
            bool blnResult = false;

            try
            {
                using (FileStream ptrFile1 = new FileStream(strFile1_a, FileMode.Open, FileAccess.Read))
                using (FileStream ptrFile2 = new FileStream(strFile2_a, FileMode.Open, FileAccess.Read))
                {
                    bool blnMatch = true;
                    bool blnDone = false;

                    while (!blnDone)
                    {
                        int intByte1 = ptrFile1.ReadByte();
                        int intByte2 = ptrFile2.ReadByte();

                        if (intByte1 != intByte2)
                        {
                            blnMatch = false;
                            blnDone = true;
                        }
                        else if (intByte1 == -1)
                        {
                            blnDone = true;
                        }
                    }

                    blnResult = blnMatch;
                }
            }
            catch { }

            return blnResult;
        }

        /// <summary>
        /// Compare two byte arrays. Returns true if identical.
        /// </summary>
        public static bool Bytes(byte[] arrData1_a, byte[] arrData2_a)
        {
            bool blnResult = false;

            try
            {
                if (arrData1_a.Length == arrData2_a.Length)
                {
                    bool blnMatch = true;

                    for (int intI = 0; intI < arrData1_a.Length && blnMatch; intI++)
                    {
                        if (arrData1_a[intI] != arrData2_a[intI])
                        {
                            blnMatch = false;
                        }
                    }

                    blnResult = blnMatch;
                }
            }
            catch { }

            return blnResult;
        }
    }
}
