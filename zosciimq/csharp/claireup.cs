// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Retention Cron Job (Claireup.cs)
// Deletes messages based on the RRRR (Retention Days) value in the filename.
//  
// Execution: dotnet run Claireup.cs

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace CyborgUnicorn.ZosciiMQ
{
    public class ClaireupConstants
    {
        public const bool CLI_ONLY = true;
        public const string FILE_ERRORLOG = "./claireup.log";
    }
    
    public class Claireup
    {
        private static void LogError(string str)
        {
            if (Constants.LOG_OUTPUT)
            {
                File.AppendAllText(ClaireupConstants.FILE_ERRORLOG, 
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " - " + str + "\n");
            }
        }
        
        private static void LogDebug(string str)
        {
            if (Constants.DEBUG)
            {
                Console.WriteLine(str);
            }
        }
        
        private static int CleanUpNonces(long intCurrentTimestamp)
        {
            int intDeletedCount = 0;
            
            if (Constants.NONCE_TIMEFRAME > 0)
            {
                // --- NONCE Cleanup Logic ---
                Console.WriteLine("Processing NONCE marker files...");
                
                // Calculate the cutoff time for deletion: Current Time - (NONCE_TIMEFRAME minutes * 60 seconds)
                long intCutoffTime = intCurrentTimestamp - (Constants.NONCE_TIMEFRAME * 60);
                
                // Find all files in the nonce directory
                string[] arrNonceFiles = Directory.Exists(Constants.NONCE_ROOT) ? 
                    Directory.GetFiles(Constants.NONCE_ROOT, "*") : new string[0];
                
                if (arrNonceFiles == null || arrNonceFiles.Length == 0)
                {
                    Console.WriteLine("INFO: No NONCE files found.");
                }
                else
                {
                    foreach (string strFullPath in arrNonceFiles)
                    {
                        try
                        {
                            // Get the last modification time of the file (when it was created by index)
                            long intFileModTime = new DateTimeOffset(File.GetLastWriteTime(strFullPath)).ToUnixTimeSeconds();
                            
                            // If the modification time is older than the calculated cutoff time, delete it.
                            if (intFileModTime < intCutoffTime)
                            {
                                try
                                {
                                    File.Delete(strFullPath);
                                    intDeletedCount++;
                                    Console.WriteLine("DELETED NONCE: " + Path.GetFileName(strFullPath) + 
                                        " (Aged out: " + File.GetLastWriteTime(strFullPath).ToString("yyyy-MM-dd HH:mm:ss") + ")");
                                }
                                catch
                                {
                                    LogError("Could not delete NONCE file: " + Path.GetFileName(strFullPath) + " (Permission denied?)");
                                }
                            }
                        }
                        catch { }
                    }
                }
            }
            
            return intDeletedCount;
        }
        
        private static int CleanUpQueues(long intCurrentTimestamp)
        {
            int intDeletedCount = 0;
            
            // Find all queue directories
            string[] arrQueuePaths = Directory.Exists(Constants.QUEUE_ROOT) ?
                Directory.GetDirectories(Constants.QUEUE_ROOT) : new string[0];
            
            foreach (string strQueuePath in arrQueuePaths)
            {
                string strQueueName = Path.GetFileName(strQueuePath);
                Console.WriteLine("Processing queue: " + strQueueName);
                
                // Find all message files in the queue
                string[] arrAllFiles = Directory.GetFiles(strQueuePath, "*.bin");
                
                if (arrAllFiles == null || arrAllFiles.Length == 0)
                {
                    Console.WriteLine("INFO: Queue is empty.");
                    continue;
                }
                
                // Process each file
                foreach (string strFullPath in arrAllFiles)
                {
                    string strFilename = Path.GetFileName(strFullPath);
                    
                    // Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
                    // Split the filename into parts based on the hyphen delimiter
                    string[] arrParts = strFilename.Split('-');
                    
                    // --- PARTS CHECK ---
                    // Ensure we have at least 3 parts for a valid filename structure
                    if (arrParts.Length < 3)
                    {
                        Console.WriteLine("WARNING: Skipping non-standard file: " + strFilename + ".");
                        continue;
                    }
                    
                    // Assign the parts based on their index
                    string strMessageTimestamp = arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
                    string strRetentionString = arrParts[1];  // RRRR (4 characters)
                    
                    // Convert to integers
                    int intRetentionDays = int.Parse(strRetentionString);
                    
                    // Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
                    // *** FIX: Explicitly use the first 14 characters to guarantee a clean time
                    string strTimestampPart = strMessageTimestamp.Substring(0, 14);
                    
                    DateTime dtMessageTime;
                    if (!DateTime.TryParseExact(strTimestampPart, "yyyyMMddHHmmss", 
                        CultureInfo.InvariantCulture, DateTimeStyles.None, out dtMessageTime))
                    {
                        Console.WriteLine("WARNING: Skipping file with invalid timestamp/format: " + strFilename + 
                            " (Retention: " + strRetentionString + ")");
                        continue;
                    }
                    
                    long intMessageTime = new DateTimeOffset(dtMessageTime).ToUnixTimeSeconds();
                    
                    // Calculate the expiration time (Message Time + Retention Days in seconds)
                    // 86400 seconds = 1 day
                    long intExpirationTime = intMessageTime + (intRetentionDays * 86400);
                    
                    LogDebug("DEBUG: " + strFilename + " | Retention: " + intRetentionDays + 
                        " days | Expires: " + DateTimeOffset.FromUnixTimeSeconds(intExpirationTime)
                        .ToString("yyyy-MM-dd HH:mm:ss"));
                    
                    // Check for expiration and delete
                    if (intCurrentTimestamp >= intExpirationTime)
                    {
                        // Time to delete!
                        try
                        {
                            File.Delete(strFullPath);
                            intDeletedCount++;
                            Console.WriteLine("DELETED: " + strFilename + " (Expired: " + 
                                DateTimeOffset.FromUnixTimeSeconds(intExpirationTime).ToString("yyyy-MM-dd HH:mm:ss") + ")");
                        }
                        catch
                        {
                            LogError("Could not delete file: " + strFilename + " (Permission denied?)");
                        }
                    }
                }
            }
            
            return intDeletedCount;
        }
        
        private static int CleanUpRecursive(string strPath, long intCurrentTimestamp)
        {
            int intDeletedCount = 0;
            
            try
            {
                // Recursively get all files
                string[] arrAllFiles = Directory.GetFiles(strPath, "*.bin", SearchOption.AllDirectories);
                
                foreach (string strFullPath in arrAllFiles)
                {
                    string strFilename = Path.GetFileName(strFullPath);
                    
                    // Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
                    string[] arrParts = strFilename.Split('-');
                    
                    // Ensure we have at least 3 parts for a valid filename structure
                    if (arrParts.Length < 3)
                    {
                        Console.WriteLine("WARNING: Skipping non-standard file: " + strFilename);
                        continue;
                    }
                    
                    string strMessageTimestamp = arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
                    string strRetentionString = arrParts[1];  // RRRR (4 characters)
                    
                    // Convert to integers
                    int intRetentionDays = int.Parse(strRetentionString);
                    
                    // Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
                    string strTimestampPart = strMessageTimestamp.Substring(0, 14);
                    
                    DateTime dtMessageTime;
                    if (!DateTime.TryParseExact(strTimestampPart, "yyyyMMddHHmmss",
                        CultureInfo.InvariantCulture, DateTimeStyles.None, out dtMessageTime))
                    {
                        Console.WriteLine("WARNING: Skipping file with invalid timestamp/format: " + strFilename +
                            " (Retention: " + strRetentionString + ", Timestamp: " + strMessageTimestamp + ")");
                        continue;
                    }
                    
                    long intMessageTime = new DateTimeOffset(dtMessageTime).ToUnixTimeSeconds();
                    
                    // Calculate the expiration time (Message Time + Retention Days in seconds)
                    long intExpirationTime = intMessageTime + (intRetentionDays * 86400);
                    
                    LogDebug("DEBUG: " + strFilename + " | Retention: " + intRetentionDays +
                        " days | Expires: " + DateTimeOffset.FromUnixTimeSeconds(intExpirationTime)
                        .ToString("yyyy-MM-dd HH:mm:ss"));
                    
                    // Check for expiration and delete
                    if (intCurrentTimestamp >= intExpirationTime)
                    {
                        // Time to delete!
                        try
                        {
                            File.Delete(strFullPath);
                            intDeletedCount++;
                            // Use relative path for cleaner output in recursive mode
                            string strRelativePath = strFullPath.Replace(Constants.STORE_ROOT, "");
                            Console.WriteLine("DELETED: " + strRelativePath + " (Expired: " +
                                DateTimeOffset.FromUnixTimeSeconds(intExpirationTime).ToString("yyyy-MM-dd HH:mm:ss") + ")");
                        }
                        catch
                        {
                            LogError("Could not delete file: " + strFullPath + " (Permission denied?)");
                        }
                    }
                }
            }
            catch (Exception objError)
            {
                // Non-fatal error during traversal
                LogError("Failed to traverse directory " + strPath + " recursively: " + objError.Message);
                return 0;
            }
            
            return intDeletedCount;
        }
        
        private static int CleanUpStore(long intCurrentTimestamp)
        {
            int intDeletedCount = 0;
            
            // Find all top-level store directories
            string[] arrStorePaths = Directory.Exists(Constants.STORE_ROOT) ?
                Directory.GetDirectories(Constants.STORE_ROOT) : new string[0];
            
            foreach (string strStorePath in arrStorePaths)
            {
                string strStoreName = Path.GetFileName(strStorePath);
                Console.WriteLine("Processing store (recursively): " + strStoreName);
                
                // Use the recursive helper function to clean up the store and its subdirectories
                intDeletedCount += CleanUpRecursive(strStorePath, intCurrentTimestamp);
            }
            
            return intDeletedCount;
        }
        
        private static void HandleClaireup()
        {
            long intCurrentTimestamp = DateTimeOffset.Now.ToUnixTimeSeconds();
            int intDeletedCount = 0;
            
            Console.WriteLine("--- Starting ZOSCII MQ Claireup at " + 
                DateTimeOffset.FromUnixTimeSeconds(intCurrentTimestamp).ToString("yyyy-MM-dd HH:mm:ss") + " ---");
            
            intDeletedCount += CleanUpNonces(intCurrentTimestamp);
            intDeletedCount += CleanUpQueues(intCurrentTimestamp);
            intDeletedCount += CleanUpStore(intCurrentTimestamp);
            
            Console.WriteLine("--- Claireup complete. Total deleted: " + intDeletedCount + " ---");
        }
        
        public static void Main(string[] args)
        {
            // CLI_ONLY check - C# console apps are inherently CLI
            // No additional check needed as this is a console application
            
            Utils.InitFolders();
            
            HandleClaireup();
        }
    }
}