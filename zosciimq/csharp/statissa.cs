// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Statistics Assessor (Statissa.cs)
// 
// Scans all queues in QUEUE_ROOT to calculate storage used, file counts,
// and identifies the oldest/newest message in each queue.
// 
// Outputs a single HTML report.
// 
// Execution: dotnet run Statissa.cs (writes to statissa-report.html)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace CyborgUnicorn.ZosciiMQ
{
    public class StatissaConstants
    {
        public const string PAGE_TITLE = "ZOSCII MQ Storage Statistics (Statissa)";
        public const bool LOG_OUTPUT = true;
        public const string FILE_ERRORLOG = "./statissa.log";
        public const string OUTPUT_FILE = "./statissa-report.html";
    }
    
    public class QueueStats
    {
        public int Count { get; set; }
        public long SizeBytes { get; set; }
        public string Oldest { get; set; }
        public string OldestDisplay { get; set; }
        public string Newest { get; set; }
        public string NewestDisplay { get; set; }
    }
    
    public class Statissa
    {
        private static void LogError(string str)
        {
            if (StatissaConstants.LOG_OUTPUT)
            {
                File.AppendAllText(StatissaConstants.FILE_ERRORLOG,
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " - " + str + "\n");
            }
        }
        
        private static void CheckFolders()
        {
            if (!Directory.Exists(Constants.QUEUE_ROOT))
            {
                LogError("Fatal Error: Could not find root queue directory " + Constants.QUEUE_ROOT);
                Console.WriteLine("Fatal Error: Could not find root queue directory " + Constants.QUEUE_ROOT);
                Environment.Exit(1);
            }
            
            if (!Directory.Exists(Constants.QUEUE_ROOT + Constants.TEMP_QUEUE))
            {
                LogError("Fatal Error: Could not find temp queue directory " + Constants.QUEUE_ROOT + Constants.TEMP_QUEUE);
                Console.WriteLine("Fatal Error: Could not find temp queue directory " + Constants.QUEUE_ROOT + Constants.TEMP_QUEUE);
                Environment.Exit(1);
            }
            
            if (!Directory.Exists(Constants.STORE_ROOT))
            {
                LogError("Fatal Error: Could not find root store directory " + Constants.STORE_ROOT);
                Console.WriteLine("Fatal Error: Could not find root store directory " + Constants.STORE_ROOT);
                Environment.Exit(1);
            }
        }
        
        private static (Dictionary<string, QueueStats>, long) GetQueueStats()
        {
            var arrQueueStats = new Dictionary<string, QueueStats>();
            long intTotalStorage = 0;
            
            if (!Directory.Exists(Constants.QUEUE_ROOT))
            {
                return (arrQueueStats, intTotalStorage);
            }
            
            string[] arrQueuePaths = Directory.GetDirectories(Constants.QUEUE_ROOT);
            if (arrQueuePaths == null || arrQueuePaths.Length == 0)
            {
                return (arrQueueStats, intTotalStorage);
            }
            
            foreach (string strQueuePath in arrQueuePaths)
            {
                string strQueueName = Path.GetFileName(strQueuePath);
                long intQueueSize = 0;
                int intFileCount = 0;
                string strOldestFile = "N/A";
                string strNewestFile = "N/A";
                
                string[] arrAllFiles = Directory.GetFiles(strQueuePath, "*.bin");
                if (arrAllFiles == null)
                {
                    continue;
                }
                
                Array.Sort(arrAllFiles);
                intFileCount = arrAllFiles.Length;
                
                if (intFileCount > 0)
                {
                    strOldestFile = Path.GetFileName(arrAllFiles[0]);
                    strNewestFile = Path.GetFileName(arrAllFiles[intFileCount - 1]);
                    
                    foreach (string strFullPath in arrAllFiles)
                    {
                        intQueueSize += new FileInfo(strFullPath).Length;
                    }
                }
                
                string strOldestDisplay = "N/A";
                string strNewestDisplay = "N/A";
                if (intFileCount > 0)
                {
                    strOldestDisplay = strOldestFile.Length > 16 ? strOldestFile.Substring(8, 8) + "..." : strOldestFile;
                    strNewestDisplay = strNewestFile.Length > 16 ? strNewestFile.Substring(8, 8) + "..." : strNewestFile;
                }
                
                arrQueueStats[strQueueName] = new QueueStats
                {
                    Count = intFileCount,
                    SizeBytes = intQueueSize,
                    Oldest = strOldestFile,
                    OldestDisplay = strOldestDisplay,
                    Newest = strNewestFile,
                    NewestDisplay = strNewestDisplay
                };
                
                intTotalStorage += intQueueSize;
            }
            
            return (arrQueueStats, intTotalStorage);
        }
        
        private static (long totalSize, int fileCount, string oldestFile, string newestFile) GetDirectorySizeRecursive(string strPath)
        {
            long intTotalSize = 0;
            int intFileCount = 0;
            DateTime? oldestTime = null;
            string oldestFileName = "N/A";
            DateTime? newestTime = null;
            string newestFileName = "N/A";
            
            try
            {
                string[] arrAllFiles = Directory.GetFiles(strPath, "*.bin", SearchOption.AllDirectories);
                
                foreach (string strFullPath in arrAllFiles)
                {
                    FileInfo fileInfo = new FileInfo(strFullPath);
                    intTotalSize += fileInfo.Length;
                    intFileCount++;
                    
                    DateTime mTime = fileInfo.LastWriteTime;
                    string fileName = fileInfo.Name;
                    
                    if (oldestTime == null || mTime < oldestTime)
                    {
                        oldestTime = mTime;
                        oldestFileName = fileName;
                    }
                    
                    if (newestTime == null || mTime > newestTime)
                    {
                        newestTime = mTime;
                        newestFileName = fileName;
                    }
                }
            }
            catch (Exception objError)
            {
                LogError("Error traversing directory " + strPath + ": " + objError.Message);
                return (0, 0, "N/A", "N/A");
            }
            
            return (intTotalSize, intFileCount, oldestFileName, newestFileName);
        }
        
        private static (Dictionary<string, QueueStats>, long) GetStoreStats()
        {
            var arrStoreStats = new Dictionary<string, QueueStats>();
            long intTotalStorage = 0;
            
            if (!Directory.Exists(Constants.STORE_ROOT))
            {
                return (arrStoreStats, intTotalStorage);
            }
            
            string[] arrStorePaths = Directory.GetDirectories(Constants.STORE_ROOT);
            if (arrStorePaths == null || arrStorePaths.Length == 0)
            {
                return (arrStoreStats, intTotalStorage);
            }
            
            foreach (string strStorePath in arrStorePaths)
            {
                string strStoreName = Path.GetFileName(strStorePath);
                
                var (intStoreSize, intFileCount, strOldestFile, strNewestFile) = GetDirectorySizeRecursive(strStorePath);
                
                string strOldestDisplay = "N/A";
                string strNewestDisplay = "N/A";
                if (intFileCount > 0)
                {
                    strOldestDisplay = strOldestFile.Length > 16 ? strOldestFile.Substring(8, 8) + "..." : strOldestFile;
                    strNewestDisplay = strNewestFile.Length > 16 ? strNewestFile.Substring(8, 8) + "..." : strNewestFile;
                }
                
                arrStoreStats[strStoreName] = new QueueStats
                {
                    Count = intFileCount,
                    SizeBytes = intStoreSize,
                    Oldest = strOldestFile,
                    OldestDisplay = strOldestDisplay,
                    Newest = strNewestFile,
                    NewestDisplay = strNewestDisplay
                };
                
                intTotalStorage += intStoreSize;
            }
            
            return (arrStoreStats, intTotalStorage);
        }
        
        private static string FormatBytes(long intBytes, int intPrecision = 2)
        {
            if (intBytes == 0) return "0 Bytes";
            
            string[] arrUnits = { "Bytes", "KB", "MB", "GB", "TB" };
            int intPow = (int)Math.Floor(Math.Log(intBytes) / Math.Log(1024));
            intPow = Math.Min(intPow, arrUnits.Length - 1);
            
            double value = intBytes / Math.Pow(1024, intPow);
            return Math.Round(value, intPrecision) + " " + arrUnits[intPow];
        }
        
        private static void OutputHtmlReport(Dictionary<string, QueueStats> arrQueueStats, long intQueueTotalStorage,
                                            Dictionary<string, QueueStats> arrStoreStats, long intStoreTotalStorage)
        {
            StringBuilder sb = new StringBuilder();
            string strReportTime = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss zzz");
            
            sb.AppendLine("<!DOCTYPE html>");
            sb.AppendLine("<html lang=\"en\">");
            sb.AppendLine("<head>");
            sb.AppendLine(" <meta charset=\"UTF-8\">");
            sb.AppendLine(" <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
            sb.AppendLine($" <title>{StatissaConstants.PAGE_TITLE}</title>");
            sb.AppendLine(" <style>");
            sb.AppendLine(" body { font-family: Arial, sans-serif; background-color: #f8fafc; margin: 0 5% 5% 5%; }");
            sb.AppendLine(" table { border-collapse: collapse; width: 100%; }");
            sb.AppendLine(" th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
            sb.AppendLine(" th { background-color: #f0f0f0; }");
            sb.AppendLine(" </style>");
            sb.AppendLine("</head>");
            sb.AppendLine("<body>\n");
            sb.AppendLine($"<h1>{StatissaConstants.PAGE_TITLE}</h1>");
            sb.AppendLine($"<p>Generated at: {strReportTime}</p>");
            
            // Queues
            string strFormattedQueueTotalStorage = FormatBytes(intQueueTotalStorage);
            sb.AppendLine("<h2>Queue Overall Summary</h2>");
            sb.AppendLine($"<p>Total Overall Storage Used: {intQueueTotalStorage} Bytes ({strFormattedQueueTotalStorage})</p>");
            sb.AppendLine("<h2>Queue Breakdown</h2>");
            
            if (arrQueueStats.Count == 0)
            {
                sb.AppendLine("<p>No Queues Found</p>");
                sb.AppendLine("<p>The queue root directory (QUEUE_ROOT) is empty or does not contain any subdirectories.</p>");
            }
            else
            {
                sb.AppendLine("<table>");
                sb.AppendLine(" <tr>");
                sb.AppendLine(" <th>Queue Name</th>");
                sb.AppendLine(" <th>Messages</th>");
                sb.AppendLine(" <th>Size</th>");
                sb.AppendLine(" <th>Oldest Message (File)</th>");
                sb.AppendLine(" <th>Newest Message (File)</th>");
                sb.AppendLine(" <th>Storage % Share</th>");
                sb.AppendLine(" </tr>");
                
                foreach (var kvp in arrQueueStats)
                {
                    string strQueueName = kvp.Key;
                    QueueStats arrData = kvp.Value;
                    
                    double fltShare = 0;
                    if (intQueueTotalStorage > 0)
                    {
                        fltShare = ((double)arrData.SizeBytes / intQueueTotalStorage) * 100;
                    }
                    
                    sb.AppendLine(" <tr>");
                    sb.AppendLine($" <td>{strQueueName}</td>");
                    sb.AppendLine($" <td>{arrData.Count}</td>");
                    sb.AppendLine($" <td>{FormatBytes(arrData.SizeBytes)}</td>");
                    sb.AppendLine($" <td>{arrData.OldestDisplay}</td>");
                    sb.AppendLine($" <td>{arrData.NewestDisplay}</td>");
                    sb.AppendLine(" <td>");
                    sb.AppendLine(" <table style='width: 100px; border: none;'><tr>");
                    if (fltShare > 0)
                    {
                        sb.AppendLine($" <td style='width: {fltShare}%; background-color: #3b82f6;'></td>");
                    }
                    if (fltShare < 100)
                    {
                        sb.AppendLine($" <td style='width: {100 - fltShare}%;'></td>");
                    }
                    sb.AppendLine(" </tr></table>");
                    sb.AppendLine($" {fltShare:F2}%");
                    sb.AppendLine(" </td>");
                    sb.AppendLine(" </tr>");
                }
                
                sb.AppendLine("</table>");
            }
            
            // Store
            string strFormattedStoreTotalStorage = FormatBytes(intStoreTotalStorage);
            sb.AppendLine("<h2>Store Overall Summary</h2>");
            sb.AppendLine($"<p>Total Overall Storage Used: {intStoreTotalStorage} Bytes ({strFormattedStoreTotalStorage})</p>");
            sb.AppendLine("<h2>Store Breakdown</h2>");
            
            if (arrStoreStats.Count == 0)
            {
                sb.AppendLine("<p>No Stores Found</p>");
                sb.AppendLine("<p>The store root directory (STORE_ROOT) is empty or does not contain any subdirectories.</p>");
            }
            else
            {
                sb.AppendLine("<table>");
                sb.AppendLine(" <tr>");
                sb.AppendLine(" <th>Store Name</th>");
                sb.AppendLine(" <th>Messages</th>");
                sb.AppendLine(" <th>Size</th>");
                sb.AppendLine(" <th>Oldest Message (File)</th>");
                sb.AppendLine(" <th>Newest Message (File)</th>");
                sb.AppendLine(" <th>Storage % Share</th>");
                sb.AppendLine(" </tr>");
                
                foreach (var kvp in arrStoreStats)
                {
                    string strStoreName = kvp.Key;
                    QueueStats arrData = kvp.Value;
                    
                    double fltShare = 0;
                    if (intStoreTotalStorage > 0)
                    {
                        fltShare = ((double)arrData.SizeBytes / intStoreTotalStorage) * 100;
                    }
                    
                    sb.AppendLine(" <tr>");
                    sb.AppendLine($" <td>{strStoreName}</td>");
                    sb.AppendLine($" <td>{arrData.Count}</td>");
                    sb.AppendLine($" <td>{FormatBytes(arrData.SizeBytes)}</td>");
                    sb.AppendLine($" <td>{arrData.OldestDisplay}</td>");
                    sb.AppendLine($" <td>{arrData.NewestDisplay}</td>");
                    sb.AppendLine(" <td>");
                    sb.AppendLine(" <table style='width: 100px; border: none;'><tr>");
                    if (fltShare > 0)
                    {
                        sb.AppendLine($" <td style='width: {fltShare}%; background-color: #3b82f6;'></td>");
                    }
                    if (fltShare < 100)
                    {
                        sb.AppendLine($" <td style='width: {100 - fltShare}%;'></td>");
                    }
                    sb.AppendLine(" </tr></table>");
                    sb.AppendLine($" {fltShare:F2}%");
                    sb.AppendLine(" </td>");
                    sb.AppendLine(" </tr>");
                }
                
                sb.AppendLine("</table>");
            }
            
            sb.AppendLine("</body>");
            sb.AppendLine("</html>");
            
            // Write to file
            File.WriteAllText(StatissaConstants.OUTPUT_FILE, sb.ToString());
            Console.WriteLine($"Report generated: {StatissaConstants.OUTPUT_FILE}");
        }
        
        public static void Main(string[] args)
        {
            CheckFolders();
            
            var (arrQueueStats, intQueueTotalStorage) = GetQueueStats();
            var (arrStoreStats, intStoreTotalStorage) = GetStoreStats();
            
            OutputHtmlReport(arrQueueStats, intQueueTotalStorage, arrStoreStats, intStoreTotalStorage);
        }
    }
}