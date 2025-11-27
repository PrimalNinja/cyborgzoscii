// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (C# Version)
// Message queue and storage system with ZOSCII addressing

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading;
using System.Web;

namespace CyborgUnicorn.ZosciiMQ
{
    public class Constants
    {
        public const bool DEBUG = false;
        public const bool LOG_OUTPUT = false;
        
        public const string LOCAL_URL = "http://localhost/zosciimq/index.php";
        public const int FOLDER_PERMISSIONS = 755;
        
        public const string NONCE_ROOT = "./nonce/";
        public const string QUEUE_ROOT = "./queues/";
        public const string STORE_ROOT = "./store/";
        
        public const string TEMP_QUEUE = "temp/";
        
        public const string LOCK_FOLDER = "locks/";
        public const int LOCK_WAIT = 10000; // in microseconds
        public const int LOCK_TIMEFRAME = 5; // in seconds to keep the lock
        
        public const int NONCE_TIMEFRAME = 5; // minimum minutes to keep NONCE
    }
    
    public class Config
    {
        public const bool ALLOW_FETCH = true;
        public const bool ALLOW_GET = false;
        public const bool ALLOW_IDENTIFY = false;
        public const bool ALLOW_PUBLISH = false;
        public const bool ALLOW_RETRIEVE = false;
        public const bool ALLOW_SCAN = false;
        public const bool ALLOW_STORE = false;
        
        public const string FILE_ERRORLOG = "./zosciimq.log";
    }
    
    public class Utils
    {
        // Converts the name timestamp part (YYYYMMDDHHNNSSCCCC) to a reordered 
        // decimal number (SSNNHHDDMMYYYYCCCC) and encodes it in BASE36.
        public static string ConvertNameToBase36(string strName)
        {
            string varResult = null;
            
            // Extract the timestamp part from YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
            string[] arrParts = strName.Split('-');
            
            // We only care about the first part (timestamp)
            string strTimestamp = arrParts[0];
            
            if (strTimestamp.Length == 18)
            {
                // Reorder the parts: SS NN HH DD MM YYYY CCCC
                string SS = strTimestamp.Substring(12, 2);
                string NN = strTimestamp.Substring(10, 2);
                string HH = strTimestamp.Substring(8, 2);
                string DD = strTimestamp.Substring(6, 2);
                string MM = strTimestamp.Substring(4, 2);
                string YYYY = strTimestamp.Substring(0, 4);
                string CCCC = strTimestamp.Substring(14, 4);
                
                // Concatenate to form the large decimal number
                string strReorderedDecimal = SS + NN + HH + DD + MM + YYYY + CCCC;
                
                // Convert to BASE36 (0-9 and a-z)
                long decimalValue = long.Parse(strReorderedDecimal);
                varResult = ToBase36(decimalValue);
            }
            
            return varResult;
        }
        
        private static string ToBase36(long value)
        {
            const string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
            if (value == 0) return "0";
            
            StringBuilder result = new StringBuilder();
            while (value > 0)
            {
                result.Insert(0, chars[(int)(value % 36)]);
                value /= 36;
            }
            return result.ToString();
        }
        
        public static string GetGUID()
        {
            Guid guid = Guid.NewGuid();
            return guid.ToString();
        }
        
        public static void InitFolders()
        {
            // Ensure the root queue directory exists
            if (!Directory.Exists(Constants.QUEUE_ROOT))
            {
                try
                {
                    Directory.CreateDirectory(Constants.QUEUE_ROOT);
                }
                catch (Exception ex)
                {
                    LogError("Fatal Could not create root queue directory: " + Constants.QUEUE_ROOT);
                    Environment.Exit(1);
                }
            }
            
            if (!Directory.Exists(Constants.QUEUE_ROOT + Constants.TEMP_QUEUE))
            {
                try
                {
                    Directory.CreateDirectory(Constants.QUEUE_ROOT + Constants.TEMP_QUEUE);
                }
                catch (Exception ex)
                {
                    LogError("Fatal Could not create temp queue directory: " + Constants.QUEUE_ROOT + Constants.TEMP_QUEUE);
                    Environment.Exit(1);
                }
            }
            
            // Ensure the root store directory exists
            if (!Directory.Exists(Constants.STORE_ROOT))
            {
                try
                {
                    Directory.CreateDirectory(Constants.STORE_ROOT);
                }
                catch (Exception ex)
                {
                    LogError("Fatal Could not create root store directory: " + Constants.STORE_ROOT);
                    Environment.Exit(1);
                }
            }
            
            // Ensure the nonce directory exists
            if (!Directory.Exists(Constants.NONCE_ROOT))
            {
                try
                {
                    Directory.CreateDirectory(Constants.NONCE_ROOT);
                }
                catch (Exception ex)
                {
                    LogError("Fatal Could not create nonce directory: " + Constants.NONCE_ROOT);
                    Environment.Exit(1);
                }
            }
        }
        
        public static int GetRetentionFromName(string strName)
        {
            // Name format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
            // The retention value (RRRR) is the second segment.
            string[] arrParts = strName.Split('-');
            
            // Check if the parts exist (we need at least two segments for retention)
            if (arrParts.Length >= 2)
            {
                // Cast to integer to get the numeric value from the RRRR segment (index 1)
                if (int.TryParse(arrParts[1], out int result))
                {
                    return result;
                }
            }
            
            // Default to 0 days retention if the format is invalid
            return 0;
        }
        
        public static string InsertSuffixBeforeExtension(string strName, string strSuffix)
        {
            string strResult = "";
            
            // Get extension
            string strExtension = Path.GetExtension(strName);
            string strBaseName = Path.GetFileNameWithoutExtension(strName);
            
            // Recombine the new base name and the extension
            if (!string.IsNullOrEmpty(strExtension))
            {
                strResult = strBaseName + strSuffix + strExtension;
            }
            else
            {
                // Insert the suffix into the base name
                strResult = strBaseName + strSuffix;
            }
            
            return strResult;
        }
        
        public static void LogDebug(string str)
        {
            if (Constants.DEBUG)
            {
                Console.WriteLine(str);
            }
        }
        
        public static void LogError(string str)
        {
            if (Constants.LOG_OUTPUT)
            {
                File.AppendAllText(Config.FILE_ERRORLOG, DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " - " + str + "\n");
            }
        }
        
        public static void SendJSONResponse(HttpListenerResponse response, string strSystemError, string strError, string strMessage, object arrResult)
        {
            var arrJSON = new
            {
                system = "ZOSCII MQ",
                version = "1.0",
                error = strError,
                message = strMessage,
                result = arrResult
            };
            
            if (!string.IsNullOrEmpty(strSystemError))
            {
                LogError(strSystemError);
            }
            
            response.ContentType = "application/json";
            string strJSON = JsonSerializer.Serialize(arrJSON);
            byte[] buffer = Encoding.UTF8.GetBytes(strJSON);
            response.ContentLength64 = buffer.Length;
            response.OutputStream.Write(buffer, 0, buffer.Length);
            response.OutputStream.Close();
        }
    }
    
    public class Handlers
    {
        public static int CleanUpLocks(string strLocksPath)
        {
            int intResult = 0;
            
            DateTime intCutoffTime = DateTime.Now.AddSeconds(-Constants.LOCK_TIMEFRAME);
            
            // Find all .lock files
            string[] arrLockFiles = Directory.Exists(strLocksPath) ? Directory.GetFiles(strLocksPath, "*.lock") : new string[0];
            
            if (arrLockFiles != null && arrLockFiles.Length > 0)
            {
                Utils.LogError($"cleanUpLocks: Found {arrLockFiles.Length} lock files");
                
                foreach (string strLockFile in arrLockFiles)
                {
                    try
                    {
                        DateTime intFileMTime = File.GetLastWriteTime(strLockFile);
                        
                        if (intFileMTime < intCutoffTime)
                        {
                            try
                            {
                                File.Delete(strLockFile);
                            }
                            catch
                            {
                                Utils.LogError("Failed to delete stale lock file: " + strLockFile);
                            }
                        }
                    }
                    catch { }
                }
            }
            else
            {
                Utils.LogError("cleanUpLocks: glob did NOT return an array!");
            }
            
            // Re-scan
            arrLockFiles = Directory.Exists(strLocksPath) ? Directory.GetFiles(strLocksPath, "*.lock") : new string[0];
            if (arrLockFiles != null && arrLockFiles.Length > 0)
            {
                intResult = arrLockFiles.Length;
            }
            
            return intResult;
        }
        
        public static List<string> FindUnidentifiedFilesRecursive(string strPath)
        {
            List<string> arrResult = new List<string>();
            
            try
            {
                string[] arrItems = Directory.GetFileSystemEntries(strPath);
                
                foreach (string strItem in arrItems)
                {
                    string strItemName = Path.GetFileName(strItem);
                    
                    if (strItemName == "." || strItemName == "..")
                    {
                        continue;
                    }
                    
                    if (Directory.Exists(strItem))
                    {
                        // Recursively search subdirectories
                        arrResult.AddRange(FindUnidentifiedFilesRecursive(strItem + Path.DirectorySeparatorChar));
                    }
                    else if (File.Exists(strItem))
                    {
                        // Check specifically for '-u' before the file extension (e.g., blah-u.bin)
                        if (Regex.IsMatch(strItemName, @"-u\.[^.]+$", RegexOptions.IgnoreCase))
                        {
                            arrResult.Add(strItemName);
                        }
                    }
                }
            }
            catch { }
            
            return arrResult;
        }
        
        public static void HandleFetch(HttpListenerResponse response, string strQueueName, string strAfterName, int intOffset, int intLength)
        {
            string strQueuePath = Constants.QUEUE_ROOT + strQueueName + Path.DirectorySeparatorChar;
            string strLockPath = Constants.QUEUE_ROOT + strQueueName + Path.DirectorySeparatorChar + Constants.LOCK_FOLDER;
            
            if (Directory.Exists(strQueuePath))
            {
                // wait for lock to become free
                while (CleanUpLocks(strLockPath) > 0)
                {
                    Thread.Sleep(Constants.LOCK_WAIT / 1000); // Convert microseconds to milliseconds
                }
                
                // Get all message files, sorted chronologically by name
                string[] arrAllFiles = Directory.GetFiles(strQueuePath, "*.bin");
                if (arrAllFiles == null || arrAllFiles.Length == 0)
                {
                    Utils.SendJSONResponse(response, "", "", "Queue is empty.", new object[0]);
                    return;
                }
                else
                {
                    // Sort files alphabetically (chronologically by name)
                    Array.Sort(arrAllFiles);
                    string strNextMessagePath = null;
                    
                    bool blnFoundAfter = false;
                    // Check if client is requesting from the start (empty 'after')
                    if (string.IsNullOrEmpty(strAfterName))
                    {
                        blnFoundAfter = true;
                    }
                    
                    // Find the next message after the 'after' pointer
                    foreach (string strFullPath in arrAllFiles)
                    {
                        string strName = Path.GetFileName(strFullPath);
                        if (blnFoundAfter)
                        {
                            // This is the first file after the 'after' pointer
                            strNextMessagePath = strFullPath;
                            break;
                        }
                        if (strName == strAfterName)
                        {
                            // Found the pointer; the next iteration's file will be the message to return.
                            blnFoundAfter = true;
                        }
                    }
                    
                    if (strNextMessagePath != null)
                    {
                        string strName = Path.GetFileName(strNextMessagePath);
                        
                        if (intOffset == 0 && intLength == 0)
                        {
                            response.ContentType = "application/octet-stream";
                            response.AddHeader("Content-Disposition", $"attachment; filename=\"{strName}\"");
                            
                            byte[] fileBytes = File.ReadAllBytes(strNextMessagePath);
                            response.ContentLength64 = fileBytes.Length;
                            response.OutputStream.Write(fileBytes, 0, fileBytes.Length);
                            response.OutputStream.Close();
                            return;
                        }
                        
                        long intFileSize = new FileInfo(strNextMessagePath).Length;
                        
                        if (intOffset < 0 || intOffset >= intFileSize)
                        {
                            Utils.SendJSONResponse(response, "", "Invalid offset.", "", new object[0]);
                            return;
                        }
                        
                        if (intLength <= 0 || (intOffset + intLength) > intFileSize)
                        {
                            intLength = (int)(intFileSize - intOffset);
                        }
                        
                        response.ContentType = "application/octet-stream";
                        response.AddHeader("Content-Disposition", $"attachment; filename=\"{strName}\"");
                        response.ContentLength64 = intLength;
                        response.AddHeader("X-ZOSCII-Offset", intOffset.ToString());
                        response.AddHeader("X-ZOSCII-Total-Length", intFileSize.ToString());
                        
                        using (FileStream objFile = File.OpenRead(strNextMessagePath))
                        {
                            objFile.Seek(intOffset, SeekOrigin.Begin);
                            byte[] binChunk = new byte[intLength];
                            objFile.Read(binChunk, 0, intLength);
                            response.OutputStream.Write(binChunk, 0, intLength);
                        }
                        response.OutputStream.Close();
                        return;
                    }
                    else
                    {
                        Utils.SendJSONResponse(response, "", "", $"No new messages found after '{strAfterName}' (or queue is empty).", new object[0]);
                        return;
                    }
                }
            }
            else
            {
                Utils.SendJSONResponse(response, "", $"Queue '{strQueueName}' does not exist.", "", new object[0]);
                return;
            }
        }
        
        public static void HandleIdentify(HttpListenerResponse response, List<string> arrNames)
        {
            List<string> arrResult = new List<string>();
            
            foreach (string strName in arrNames)
            {
                string strBaseName = Path.GetFileName(strName);
                
                string strExtension = Path.GetExtension(strBaseName);
                int intExtensionLength = strExtension.Length;
                int intSuffixLength = intExtensionLength + 2; // "-u"
                
                if (strBaseName.Length >= intSuffixLength && 
                    strBaseName.Substring(strBaseName.Length - intSuffixLength, 2) == "-u")
                {
                    string strTempName = Utils.ConvertNameToBase36(strBaseName);
                    
                    if (strTempName != null)
                    {
                        string strDir1 = strTempName.Substring(0, 1);
                        string strDir2 = strTempName.Substring(1, 1);
                        string strDir3 = strTempName.Substring(2, 1);
                        string strStorePath = Constants.STORE_ROOT + strDir1 + Path.DirectorySeparatorChar + 
                                            strDir2 + Path.DirectorySeparatorChar + strDir3 + Path.DirectorySeparatorChar;
                        
                        string strFullCurrentPath = strStorePath + strBaseName;
                        
                        if (File.Exists(strFullCurrentPath))
                        {
                            // Removes the '-u' suffix
                            string strNewName = strBaseName.Remove(strBaseName.Length - intSuffixLength, 2);
                            string strFullNewPath = strStorePath + strNewName;
                            
                            try
                            {
                                File.Move(strFullCurrentPath, strFullNewPath);
                                arrResult.Add(strNewName);
                            }
                            catch { }
                        }
                    }
                }
            }
            
            Utils.SendJSONResponse(response, "", "", "Returned messages identified.", arrResult);
        }
        
        public static void HandleNonce(HttpListenerResponse response, string strNonce)
        {
            string strNonceFile = Constants.NONCE_ROOT + strNonce;
            if (File.Exists(strNonceFile))
            {
                Utils.SendJSONResponse(response, "", "", "Nonce already used.", new object[0]);
            }
        }
        
        public static void HandlePublish(HttpListenerResponse response, string strQueueName, string strNonce, 
                                        int intRetentionDays, byte[] binMessage)
        {
            // Format RRRR, e.g., 3 becomes 0003
            string strRetentionDays = intRetentionDays.ToString("D4");
            
            if (binMessage == null || binMessage.Length == 0)
            {
                Utils.SendJSONResponse(response, "", "Message required.", "", new object[0]);
                return;
            }
            else
            {
                string strQueuePath = Constants.QUEUE_ROOT + strQueueName + Path.DirectorySeparatorChar;
                string strLockPath = strQueuePath + Constants.LOCK_FOLDER;
                
                if (!Directory.Exists(strQueuePath))
                {
                    try
                    {
                        Directory.CreateDirectory(strQueuePath);
                    }
                    catch
                    {
                        Utils.SendJSONResponse(response, "index.cs: Could not create queue directory: " + strQueuePath, 
                                             "Could not create queue.", "", new object[0]);
                        return;
                    }
                }
                
                if (!Directory.Exists(strLockPath))
                {
                    try
                    {
                        Directory.CreateDirectory(strLockPath);
                    }
                    catch
                    {
                        Utils.SendJSONResponse(response, "index.cs: Could not create lock directory: " + strLockPath, 
                                             "Could not create queue.", "", new object[0]);
                        return;
                    }
                }
                
                // wait for lock to become free
                while (CleanUpLocks(strLockPath) > 0)
                {
                    Thread.Sleep(Constants.LOCK_WAIT / 1000);
                }
                
                string strLockFile = Utils.GetGUID() + ".lock";
                string strLockFilePath = strLockPath + strLockFile;
                
                try
                {
                    File.WriteAllText(strLockFilePath, "");
                    
                    string strBaseTime = DateTime.Now.ToString("yyyyMMddHHmmss");
                    int intCollisionCounter = 0;
                    string strName = "";
                    string strFullPath = "";
                    string strFullTempPath = Constants.QUEUE_ROOT + Constants.TEMP_QUEUE + Utils.GetGUID() + ".bin";
                    
                    // Write to temp location first
                    File.WriteAllBytes(strFullTempPath, binMessage);
                    
                    // Generate unique name with collision handling
                    while (true)
                    {
                        string strCollisionCounter = intCollisionCounter.ToString("D4");
                        strName = strBaseTime + strCollisionCounter + "-" + strRetentionDays + "-" + Utils.GetGUID() + ".bin";
                        strFullPath = strQueuePath + strName;
                        
                        // Check for existence. If unique, break the loop.
                        if (!File.Exists(strFullPath))
                        {
                            break;
                        }
                        
                        // If file exists, we had a collision. Try the next sequential number.
                        intCollisionCounter++;
                        
                        // Safety break: Prevents an infinite loop.
                        if (intCollisionCounter > 9999)
                        {
                            try { File.Delete(strLockFilePath); } catch { }
                            Utils.SendJSONResponse(response, "index.cs: Queue exceeded 9,999 attempted messages in one second.", 
                                                 "Queue overload, try again.", "", new object[0]);
                            return;
                        }
                    }
                    
                    try
                    {
                        File.Move(strFullTempPath, strFullPath);
                    }
                    catch
                    {
                        try { File.Delete(strFullTempPath); } catch { }
                        try { File.Delete(strLockFilePath); } catch { }
                        Utils.SendJSONResponse(response, "index.cs: Failed to create message.", "Failed to create message.", "", new object[0]);
                        return;
                    }
                    
                    if (!string.IsNullOrEmpty(strNonce))
                    {
                        string strNonceFile = Constants.NONCE_ROOT + strNonce;
                        try
                        {
                            File.WriteAllText(strNonceFile, "");
                        }
                        catch
                        {
                            try { File.Delete(strFullPath); } catch { }
                            try { File.Delete(strLockFilePath); } catch { }
                            Utils.SendJSONResponse(response, "index.cs: Failed to create nonce.", "Failed to create nonce.", "", new object[0]);
                            return;
                        }
                    }
                    
                    try { File.Delete(strLockFilePath); } catch { }
                    Utils.SendJSONResponse(response, "", "", "Message published.", new object[0]);
                }
                finally
                {
                    try { File.Delete(strLockFilePath); } catch { }
                }
            }
        }
        
        public static void HandleRetrieve(HttpListenerResponse response, string strName)
        {
            if (string.IsNullOrEmpty(strName))
            {
                Utils.SendJSONResponse(response, "", "Missing 'name' argument for retrieve action.", "", new object[0]);
                return;
            }
            else
            {
                string strTempName = Utils.ConvertNameToBase36(strName);
                
                if (strTempName == null)
                {
                    Utils.SendJSONResponse(response, "", $"Invalid name '{strName}'.", "", new object[0]);
                    return;
                }
                else
                {
                    // Extract first 3 chars for nested path
                    string strDir1 = strTempName.Substring(0, 1);
                    string strDir2 = strTempName.Substring(1, 1);
                    string strDir3 = strTempName.Substring(2, 1);
                    string strStorePath = Constants.STORE_ROOT + strDir1 + Path.DirectorySeparatorChar + 
                                        strDir2 + Path.DirectorySeparatorChar + strDir3 + Path.DirectorySeparatorChar;
                    
                    string strFullPath = strStorePath + strName;
                    
                    if (File.Exists(strFullPath))
                    {
                        response.ContentType = "application/octet-stream";
                        response.AddHeader("Content-Disposition", $"attachment; filename=\"{strName}\"");
                        
                        byte[] fileBytes = File.ReadAllBytes(strFullPath);
                        response.ContentLength64 = fileBytes.Length;
                        response.OutputStream.Write(fileBytes, 0, fileBytes.Length);
                        response.OutputStream.Close();
                        return;
                    }
                    else
                    {
                        Utils.SendJSONResponse(response, "", "Message not found.", "", new object[0]);
                        return;
                    }
                }
            }
        }
        
        public static void HandleScan(HttpListenerResponse response)
        {
            List<string> arrResult = FindUnidentifiedFilesRecursive(Constants.STORE_ROOT);
            Utils.SendJSONResponse(response, "", "", "", arrResult);
        }
        
        public static void HandleStore(HttpListenerResponse response, string strNonce, int intRetentionDays, byte[] binMessage)
        {
            // Format RRRR, e.g., 3 becomes 0003
            string strRetentionDays = intRetentionDays.ToString("D4");
            
            if (binMessage == null || binMessage.Length == 0)
            {
                Utils.SendJSONResponse(response, "", "Message required.", "", new object[0]);
                return;
            }
            else
            {
                string strBaseTime = DateTime.Now.ToString("yyyyMMddHHmmss");
                string strName = "";
                string strFullPath = "";
                string strGetGUID = Utils.GetGUID();
                
                strName = strBaseTime + "0000-" + strRetentionDays + "-" + strGetGUID + ".bin";
                
                string strTempName = Utils.ConvertNameToBase36(strName);
                
                // Extract first 3 chars for nested path
                string strDir1 = strTempName.Substring(0, 1);
                string strDir2 = strTempName.Substring(1, 1);
                string strDir3 = strTempName.Substring(2, 1);
                string strStorePath = Constants.STORE_ROOT + strDir1 + Path.DirectorySeparatorChar + 
                                    strDir2 + Path.DirectorySeparatorChar + strDir3 + Path.DirectorySeparatorChar;
                
                strFullPath = strStorePath + strName;
                
                if (!Directory.Exists(strStorePath))
                {
                    try
                    {
                        Directory.CreateDirectory(strStorePath);
                    }
                    catch
                    {
                        Utils.SendJSONResponse(response, "index.cs: Could not create store directory: " + strStorePath, 
                                             "Could not create store.", "", new object[0]);
                        return;
                    }
                }
                
                try
                {
                    File.WriteAllBytes(strFullPath, binMessage);
                }
                catch
                {
                    Utils.SendJSONResponse(response, "index.cs: Failed to create message.", "Failed to create message.", "", new object[0]);
                    return;
                }
                
                if (!string.IsNullOrEmpty(strNonce))
                {
                    string strNonceFile = Constants.NONCE_ROOT + strNonce;
                    try
                    {
                        File.WriteAllText(strNonceFile, "");
                    }
                    catch
                    {
                        try { File.Delete(strFullPath); } catch { }
                        Utils.SendJSONResponse(response, "index.cs: Failed to create nonce.", "Failed to create nonce.", "", new object[0]);
                        return;
                    }
                }
                
                Utils.SendJSONResponse(response, "", "", "Message stored.", strName);
            }
        }
    }
    
    // HTTP Server Entry Point (simplified - would need proper HTTP listener in production)
    public class Program
    {
        public static void Main(string[] args)
        {
            Utils.InitFolders();
            
            // This is a simplified example - in production, you'd use ASP.NET Core or similar
            Console.WriteLine("ZOSCII MQ C# Server");
            Console.WriteLine("To implement: Use ASP.NET Core WebAPI with proper routing");
        }
    }
}