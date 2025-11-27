// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Server-to-Q Replikate (Replikate.cs)
// 
// Pulls messages from a URL and publishes them to a local Target Queue.
// State is managed using a unique file per target queue, making it concurrency-safe.
// 
// Execution Example:
// Replikate.exe --url=http://other.server/index.php --sq=sourcequeue --tq=targetqueue

using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace CyborgUnicorn.ZosciiMQ
{
    public class ReplikateConstants
    {
        public const bool CLI_ONLY = false;
        public const string FILE_ERRORLOG = "./replikate.log";
        public const string STATE_FILE_TEMPLATE = "./states/replikate_state_%SOURCE%_%TARGET%.txt";
        public const string STATE_KEY_NAME = "last_processed_id";
    }
    
    public class Replikate
    {
        private static readonly HttpClient httpClient = new HttpClient();
        
        private static void LogError(string str)
        {
            if (Constants.LOG_OUTPUT)
            {
                File.AppendAllText(ReplikateConstants.FILE_ERRORLOG,
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " - " + str + "\n");
            }
        }
        
        private static async Task<(string name, byte[] content)> FetchNextMessage(
            string strSourceURL, string strSourceQueue, string strAfterName)
        {
            string name = null;
            byte[] content = null;
            
            try
            {
                var postData = new Dictionary<string, string>
                {
                    { "action", "fetch" },
                    { "q", strSourceQueue },
                    { "after", strAfterName }
                };
                
                var formContent = new FormUrlEncodedContent(postData);
                var response = await httpClient.PostAsync(strSourceURL, formContent);
                
                if (!response.IsSuccessStatusCode)
                {
                    LogError($"Received HTTP Status {response.StatusCode}. Server response: {await response.Content.ReadAsStringAsync()}");
                }
                else
                {
                    // Check if this is a binary response (message) or JSON (error/status)
                    var contentDisposition = response.Content.Headers.ContentDisposition;
                    
                    if (contentDisposition != null && !string.IsNullOrEmpty(contentDisposition.FileName))
                    {
                        name = contentDisposition.FileName.Trim('"');
                        name = Path.GetFileName(name);
                        content = await response.Content.ReadAsByteArrayAsync();
                    }
                    else
                    {
                        // Try to parse as JSON
                        string responseText = await response.Content.ReadAsStringAsync();
                        try
                        {
                            using (JsonDocument doc = JsonDocument.Parse(responseText))
                            {
                                JsonElement root = doc.RootElement;
                                if (root.TryGetProperty("system", out JsonElement system) && 
                                    system.GetString() == "ZOSCII MQ")
                                {
                                    if (root.TryGetProperty("error", out JsonElement error) && 
                                        !string.IsNullOrEmpty(error.GetString()))
                                    {
                                        string message = root.TryGetProperty("message", out JsonElement msg) ? 
                                            msg.GetString() : "";
                                        LogError($"Source MQ reported JSON error: {error.GetString()} - Message: {message}");
                                    }
                                }
                                else
                                {
                                    LogError($"Invalid JSON. Content: {responseText.Substring(0, Math.Min(100, responseText.Length))}");
                                }
                            }
                        }
                        catch
                        {
                            LogError($"Invalid response structure (missing name or content, and not valid JSON). Content: {responseText.Substring(0, Math.Min(100, responseText.Length))}");
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                LogError($"cURL ERROR: Failed to connect or execute request: {ex.Message}");
            }
            
            return (name, content);
        }
        
        private static async Task<bool> PublishToQueue(string strTargetQueue, int intRetention, byte[] binContent)
        {
            bool blnResult = true;
            
            // Format retention days (r) as a 4-digit string
            string strRetentionDays = intRetention.ToString("D4");
            
            try
            {
                var postData = new Dictionary<string, string>
                {
                    { "action", "publish" },
                    { "q", strTargetQueue },
                    { "r", strRetentionDays },
                    { "msg", Convert.ToBase64String(binContent) } // Base64 encode binary data
                };
                
                var formContent = new FormUrlEncodedContent(postData);
                var response = await httpClient.PostAsync(Constants.LOCAL_URL, formContent);
                
                if (!response.IsSuccessStatusCode)
                {
                    LogError($"Local POST to index failed with HTTP code: {response.StatusCode}");
                    blnResult = false;
                }
                else
                {
                    // Decode and check API response for success
                    string strResponse = await response.Content.ReadAsStringAsync();
                    
                    try
                    {
                        using (JsonDocument doc = JsonDocument.Parse(strResponse))
                        {
                            JsonElement root = doc.RootElement;
                            if (root.TryGetProperty("error", out JsonElement error))
                            {
                                string errorStr = error.GetString();
                                if (!string.IsNullOrEmpty(errorStr))
                                {
                                    string systemError = root.TryGetProperty("system", out JsonElement sys) ? 
                                        sys.GetString() : "";
                                    LogError($"Local API Error: {errorStr} / System Error: {systemError}");
                                    blnResult = false;
                                }
                            }
                        }
                    }
                    catch
                    {
                        LogError($"Local POST returned invalid JSON: {strResponse}");
                        blnResult = false;
                    }
                }
            }
            catch (Exception ex)
            {
                LogError($"Exception during publish: {ex.Message}");
                blnResult = false;
            }
            
            return blnResult;
        }
        
        private static bool SaveToStore(string strName, byte[] binMessage)
        {
            bool blnResult = true;
            
            string strTempName = Utils.ConvertNameToBase36(strName);
            
            if (strTempName == null)
            {
                LogError("Name conversion failed for: " + strName);
                return false;
            }
            
            string strDir1 = strTempName.Substring(0, 1);
            string strDir2 = strTempName.Substring(1, 1);
            string strDir3 = strTempName.Substring(2, 1);
            string strStorePath = Constants.STORE_ROOT + strDir1 + Path.DirectorySeparatorChar + 
                                strDir2 + Path.DirectorySeparatorChar + strDir3 + Path.DirectorySeparatorChar;
            
            if (!Directory.Exists(strStorePath))
            {
                try
                {
                    Directory.CreateDirectory(strStorePath);
                }
                catch
                {
                    blnResult = false;
                    LogError("Could not create nested store directory: " + strStorePath);
                }
            }
            
            if (blnResult)
            {
                strName = Utils.InsertSuffixBeforeExtension(strName, "-u");
                string strFullPath = strStorePath + strName;
                
                try
                {
                    File.WriteAllBytes(strFullPath, binMessage);
                }
                catch
                {
                    blnResult = false;
                    LogError("Failed to write file to store: " + strFullPath);
                }
            }
            
            return blnResult;
        }
        
        private static async Task HandleReplikate(string strSourceURL, string strSourceQueue, string strTargetQueue)
        {
            Console.WriteLine("--- Starting ZOSCII MQ Replikation ---");
            
            string strSafeSource = Regex.Replace(strSourceQueue, @"[^a-zA-Z0-9_-]", "_");
            string strSafeTarget = Regex.Replace(strTargetQueue, @"[^a-zA-Z0-9_-]", "_");
            if (string.IsNullOrEmpty(strTargetQueue))
            {
                strSafeTarget = "store";
            }
            
            string strStateFilePath = ReplikateConstants.STATE_FILE_TEMPLATE;
            strStateFilePath = strStateFilePath.Replace("%SOURCE%", strSafeSource);
            strStateFilePath = strStateFilePath.Replace("%TARGET%", strSafeTarget);
            
            // Ensure states directory exists
            string statesDir = Path.GetDirectoryName(strStateFilePath);
            if (!Directory.Exists(statesDir))
            {
                Directory.CreateDirectory(statesDir);
            }
            
            string strLastName = "";
            if (File.Exists(strStateFilePath))
            {
                strLastName = File.ReadAllText(strStateFilePath).Trim();
            }
            
            if (string.IsNullOrEmpty(strLastName))
            {
                Console.WriteLine("Last Processed Pointer: START");
            }
            else
            {
                Console.WriteLine("Last Processed Pointer: " + strLastName);
            }
            
            int intTotalReplikated = 0;
            
            while (true)
            {
                var (strName, binContent) = await FetchNextMessage(strSourceURL, strSourceQueue, strLastName);
                
                if (strName == null)
                {
                    Console.WriteLine("Source URL is caught up or returned no data. Halting this run.");
                    break;
                }
                
                bool blnSuccess = false;
                
                if (!string.IsNullOrEmpty(strTargetQueue))
                {
                    // Save to queue
                    int intRetention = Utils.GetRetentionFromName(strName);
                    blnSuccess = await PublishToQueue(strTargetQueue, intRetention, binContent);
                    
                    if (blnSuccess)
                    {
                        strLastName = strName;
                        intTotalReplikated++;
                        Console.WriteLine("REPLIKATED: New pointer set to " + strLastName);
                    }
                    else
                    {
                        LogError("Failed to replicate message to " + strTargetQueue + ". Halting.");
                        Console.WriteLine("Failed to replicate message to " + strTargetQueue + ". Halting.");
                        break;
                    }
                }
                else
                {
                    // Save to store
                    blnSuccess = SaveToStore(strName, binContent);
                    
                    if (blnSuccess)
                    {
                        strLastName = strName;
                        intTotalReplikated++;
                    }
                    else
                    {
                        LogError("Failed to replicate message to store. Halting.");
                        Console.WriteLine("Failed to replicate message to store. Halting.");
                        break;
                    }
                }
            }
            
            File.WriteAllText(strStateFilePath, strLastName);
            Console.WriteLine("--- Replikation finished. Total messages replikated: " + intTotalReplikated + " ---");
        }
        
        public static async Task Main(string[] args)
        {
            // Parse command-line arguments
            string strSourceURL = "";
            string strSourceQueue = "";
            string strTargetQueue = "";
            
            foreach (string arg in args)
            {
                if (arg.StartsWith("--url="))
                {
                    strSourceURL = arg.Substring(6);
                }
                else if (arg.StartsWith("--sq="))
                {
                    strSourceQueue = arg.Substring(5);
                }
                else if (arg.StartsWith("--tq="))
                {
                    strTargetQueue = arg.Substring(5);
                }
            }
            
            if (string.IsNullOrEmpty(strSourceURL) || string.IsNullOrEmpty(strSourceQueue))
            {
                LogError("Missing required arguments.");
                Console.WriteLine("Missing required arguments.");
                Console.WriteLine("Usage: Replikate --url=<source_url> --sq=<source_queue> [--tq=<target_queue>]");
                return;
            }
            
            Utils.InitFolders();
            
            await HandleReplikate(strSourceURL, strSourceQueue, strTargetQueue);
        }
    }
}