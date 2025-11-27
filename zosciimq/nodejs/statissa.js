// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Statistics Assessor (statissa.js)
// 
// Scans all queues in QUEUE_ROOT to calculate storage used, file counts,
// and identifies the oldest/newest message in each queue.
// 
// Outputs a single HTML report.
// 
// Execution: node statissa.js (starts HTTP server on port 3001)
// or: node statissa.js --file (writes to statissa-report.html)

const fs = require('fs');
const path = require('path');
const http = require('http');
const { promisify } = require('util');

const readdir = promisify(fs.readdir);
const stat = promisify(fs.stat);

// Constants
const CONFIG = {
    PAGE_TITLE: 'ZOSCII MQ Storage Statistics (Statissa)',
    
    QUEUE_ROOT: './queues/',
    STORE_ROOT: './store/',
    TEMP_QUEUE: 'temp/',
    
    LOG_OUTPUT: true,
    FILE_ERRORLOG: './statissa.log',
    OUTPUT_FILE: './statissa-report.html'
};

function logError(str) {
    if (CONFIG.LOG_OUTPUT) {
        const timestamp = new Date().toISOString().replace('T', ' ').substring(0, 19);
        fs.appendFileSync(CONFIG.FILE_ERRORLOG, timestamp + ' - ' + str + '\n');
    }
}

function checkFolders() {
    if (!fs.existsSync(CONFIG.QUEUE_ROOT)) {
        logError('Fatal Error: Could not find root queue directory ' + CONFIG.QUEUE_ROOT);
        console.log('Fatal Error: Could not find root queue directory ' + CONFIG.QUEUE_ROOT);
        process.exit(1);
    }
    
    if (!fs.existsSync(CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE)) {
        logError('Fatal Error: Could not find temp queue directory ' + CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE);
        console.log('Fatal Error: Could not find temp queue directory ' + CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE);
        process.exit(1);
    }
    
    if (!fs.existsSync(CONFIG.STORE_ROOT)) {
        logError('Fatal Error: Could not find root store directory ' + CONFIG.STORE_ROOT);
        console.log('Fatal Error: Could not find root store directory ' + CONFIG.STORE_ROOT);
        process.exit(1);
    }
}

async function getQueueStats() {
    const arrQueueStats = {};
    let intTotalStorage = 0;
    
    if (!fs.existsSync(CONFIG.QUEUE_ROOT)) {
        return [arrQueueStats, intTotalStorage];
    }
    
    let arrQueuePaths = [];
    try {
        const items = await readdir(CONFIG.QUEUE_ROOT);
        for (const item of items) {
            const fullPath = path.join(CONFIG.QUEUE_ROOT, item);
            const stats = await stat(fullPath);
            if (stats.isDirectory()) {
                arrQueuePaths.push(fullPath);
            }
        }
    } catch (err) {
        return [arrQueueStats, intTotalStorage];
    }
    
    for (const strQueuePath of arrQueuePaths) {
        const strQueueName = path.basename(strQueuePath);
        let intQueueSize = 0;
        let intFileCount = 0;
        let strOldestFile = 'N/A';
        let strNewestFile = 'N/A';
        
        let arrAllFiles = [];
        try {
            const files = await readdir(strQueuePath);
            arrAllFiles = files.filter(f => f.endsWith('.bin')).map(f => path.join(strQueuePath, f));
        } catch (err) {
            continue;
        }
        
        arrAllFiles.sort();
        intFileCount = arrAllFiles.length;
        
        if (intFileCount > 0) {
            strOldestFile = path.basename(arrAllFiles[0]);
            strNewestFile = path.basename(arrAllFiles[intFileCount - 1]);
            
            for (const strFullPath of arrAllFiles) {
                try {
                    const stats = await stat(strFullPath);
                    intQueueSize += stats.size;
                } catch (err) {}
            }
        }
        
        let strOldestDisplay = 'N/A';
        let strNewestDisplay = 'N/A';
        if (intFileCount > 0) {
            strOldestDisplay = strOldestFile.length > 16 ? strOldestFile.substring(8, 16) + '...' : strOldestFile;
            strNewestDisplay = strNewestFile.length > 16 ? strNewestFile.substring(8, 16) + '...' : strNewestFile;
        }
        
        arrQueueStats[strQueueName] = {
            count: intFileCount,
            size_bytes: intQueueSize,
            oldest: strOldestFile,
            oldest_display: strOldestDisplay,
            newest: strNewestFile,
            newest_display: strNewestDisplay
        };
        
        intTotalStorage += intQueueSize;
    }
    
    return [arrQueueStats, intTotalStorage];
}

async function getDirectorySizeRecursive(strPath) {
    let intTotalSize = 0;
    let intFileCount = 0;
    let arrOldestFile = null; // [timestamp, filename]
    let arrNewestFile = null; // [timestamp, filename]
    
    try {
        const getAllFiles = async (dirPath) => {
            let files = [];
            const items = await readdir(dirPath);
            
            for (const item of items) {
                const fullPath = path.join(dirPath, item);
                try {
                    const stats = await stat(fullPath);
                    
                    if (stats.isDirectory()) {
                        files = files.concat(await getAllFiles(fullPath));
                    } else if (item.endsWith('.bin')) {
                        files.push({ path: fullPath, stats: stats });
                    }
                } catch (err) {}
            }
            
            return files;
        };
        
        const allFiles = await getAllFiles(strPath);
        
        for (const fileInfo of allFiles) {
            intTotalSize += fileInfo.stats.size;
            intFileCount++;
            
            const intMTime = fileInfo.stats.mtimeMs;
            const strFileName = path.basename(fileInfo.path);
            
            if (arrOldestFile === null || intMTime < arrOldestFile[0]) {
                arrOldestFile = [intMTime, strFileName];
            }
            
            if (arrNewestFile === null || intMTime > arrNewestFile[0]) {
                arrNewestFile = [intMTime, strFileName];
            }
        }
    } catch (objError) {
        logError('Error traversing directory ' + strPath + ': ' + objError.message);
        return [0, 0, 'N/A', 'N/A'];
    }
    
    const strOldestFileName = arrOldestFile ? arrOldestFile[1] : 'N/A';
    const strNewestFileName = arrNewestFile ? arrNewestFile[1] : 'N/A';
    
    return [intTotalSize, intFileCount, strOldestFileName, strNewestFileName];
}

async function getStoreStats() {
    const arrStoreStats = {};
    let intTotalStorage = 0;
    
    if (!fs.existsSync(CONFIG.STORE_ROOT)) {
        return [arrStoreStats, intTotalStorage];
    }
    
    let arrStorePaths = [];
    try {
        const items = await readdir(CONFIG.STORE_ROOT);
        for (const item of items) {
            const fullPath = path.join(CONFIG.STORE_ROOT, item);
            const stats = await stat(fullPath);
            if (stats.isDirectory()) {
                arrStorePaths.push(fullPath);
            }
        }
    } catch (err) {
        return [arrStoreStats, intTotalStorage];
    }
    
    for (const strStorePath of arrStorePaths) {
        const strStoreName = path.basename(strStorePath);
        
        const [intStoreSize, intFileCount, strOldestFile, strNewestFile] = await getDirectorySizeRecursive(strStorePath);
        
        let strOldestDisplay = 'N/A';
        let strNewestDisplay = 'N/A';
        if (intFileCount > 0) {
            strOldestDisplay = strOldestFile.length > 16 ? strOldestFile.substring(8, 16) + '...' : strOldestFile;
            strNewestDisplay = strNewestFile.length > 16 ? strNewestFile.substring(8, 16) + '...' : strNewestFile;
        }
        
        arrStoreStats[strStoreName] = {
            count: intFileCount,
            size_bytes: intStoreSize,
            oldest: strOldestFile,
            oldest_display: strOldestDisplay,
            newest: strNewestFile,
            newest_display: strNewestDisplay
        };
        
        intTotalStorage += intStoreSize;
    }
    
    return [arrStoreStats, intTotalStorage];
}

function formatBytes(intBytes, intPrecision = 2) {
    if (intBytes === 0) return '0 Bytes';
    
    const arrUnits = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const intBytes_abs = Math.abs(intBytes);
    const intPow = Math.floor(Math.log(intBytes_abs) / Math.log(1024));
    const intPow_clamped = Math.min(intPow, arrUnits.length - 1);
    
    const value = intBytes / Math.pow(1024, intPow_clamped);
    return value.toFixed(intPrecision) + ' ' + arrUnits[intPow_clamped];
}

function outputHtmlReport(arrQueueStats, intQueueTotalStorage, arrStoreStats, intStoreTotalStorage) {
    const strReportTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    let html = [];
    
    html.push('<!DOCTYPE html>');
    html.push('<html lang="en">');
    html.push('<head>');
    html.push(' <meta charset="UTF-8">');
    html.push(' <meta name="viewport" content="width=device-width, initial-scale=1.0">');
    html.push(` <title>${CONFIG.PAGE_TITLE}</title>`);
    html.push(' <style>');
    html.push(' body { font-family: Arial, sans-serif; background-color: #f8fafc; margin: 0 5% 5% 5%; }');
    html.push(' table { border-collapse: collapse; width: 100%; }');
    html.push(' th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }');
    html.push(' th { background-color: #f0f0f0; }');
    html.push(' </style>');
    html.push('</head>');
    html.push('<body>\n');
    html.push(`<h1>${CONFIG.PAGE_TITLE}</h1>`);
    html.push(`<p>Generated at: ${strReportTime}</p>`);
    
    // Queues
    const strFormattedQueueTotalStorage = formatBytes(intQueueTotalStorage);
    html.push('<h2>Queue Overall Summary</h2>');
    html.push(`<p>Total Overall Storage Used: ${intQueueTotalStorage} Bytes (${strFormattedQueueTotalStorage})</p>`);
    html.push('<h2>Queue Breakdown</h2>');
    
    if (Object.keys(arrQueueStats).length === 0) {
        html.push('<p>No Queues Found</p>');
        html.push('<p>The queue root directory (QUEUE_ROOT) is empty or does not contain any subdirectories.</p>');
    } else {
        html.push('<table>');
        html.push(' <tr>');
        html.push(' <th>Queue Name</th>');
        html.push(' <th>Messages</th>');
        html.push(' <th>Size</th>');
        html.push(' <th>Oldest Message (File)</th>');
        html.push(' <th>Newest Message (File)</th>');
        html.push(' <th>Storage % Share</th>');
        html.push(' </tr>');
        
        for (const [strQueueName, arrData] of Object.entries(arrQueueStats)) {
            let fltShare = 0;
            if (intQueueTotalStorage > 0) {
                fltShare = (arrData.size_bytes / intQueueTotalStorage) * 100;
            }
            
            html.push(' <tr>');
            html.push(` <td>${strQueueName}</td>`);
            html.push(` <td>${arrData.count}</td>`);
            html.push(` <td>${formatBytes(arrData.size_bytes)}</td>`);
            html.push(` <td>${arrData.oldest_display}</td>`);
            html.push(` <td>${arrData.newest_display}</td>`);
            html.push(' <td>');
            html.push(" <table style='width: 100px; border: none;'><tr>");
            if (fltShare > 0) {
                html.push(` <td style='width: ${fltShare}%; background-color: #3b82f6;'></td>`);
            }
            if (fltShare < 100) {
                html.push(` <td style='width: ${100 - fltShare}%;'></td>`);
            }
            html.push(' </tr></table>');
            html.push(` ${fltShare.toFixed(2)}%`);
            html.push(' </td>');
            html.push(' </tr>');
        }
        
        html.push('</table>');
    }
    
    // Store
    const strFormattedStoreTotalStorage = formatBytes(intStoreTotalStorage);
    html.push('<h2>Store Overall Summary</h2>');
    html.push(`<p>Total Overall Storage Used: ${intStoreTotalStorage} Bytes (${strFormattedStoreTotalStorage})</p>`);
    html.push('<h2>Store Breakdown</h2>');
    
    if (Object.keys(arrStoreStats).length === 0) {
        html.push('<p>No Stores Found</p>');
        html.push('<p>The store root directory (STORE_ROOT) is empty or does not contain any subdirectories.</p>');
    } else {
        html.push('<table>');
        html.push(' <tr>');
        html.push(' <th>Store Name</th>');
        html.push(' <th>Messages</th>');
        html.push(' <th>Size</th>');
        html.push(' <th>Oldest Message (File)</th>');
        html.push(' <th>Newest Message (File)</th>');
        html.push(' <th>Storage % Share</th>');
        html.push(' </tr>');
        
        for (const [strStoreName, arrData] of Object.entries(arrStoreStats)) {
            let fltShare = 0;
            if (intStoreTotalStorage > 0) {
                fltShare = (arrData.size_bytes / intStoreTotalStorage) * 100;
            }
            
            html.push(' <tr>');
            html.push(` <td>${strStoreName}</td>`);
            html.push(` <td>${arrData.count}</td>`);
            html.push(` <td>${formatBytes(arrData.size_bytes)}</td>`);
            html.push(` <td>${arrData.oldest_display}</td>`);
            html.push(` <td>${arrData.newest_display}</td>`);
            html.push(' <td>');
            html.push(" <table style='width: 100px; border: none;'><tr>");
            if (fltShare > 0) {
                html.push(` <td style='width: ${fltShare}%; background-color: #3b82f6;'></td>`);
            }
            if (fltShare < 100) {
                html.push(` <td style='width: ${100 - fltShare}%;'></td>`);
            }
            html.push(' </tr></table>');
            html.push(` ${fltShare.toFixed(2)}%`);
            html.push(' </td>');
            html.push(' </tr>');
        }
        
        html.push('</table>');
    }
    
    html.push('</body>');
    html.push('</html>');
    
    return html.join('\n');
}

async function generateReport() {
    checkFolders();
    
    const [arrQueueStats, intQueueTotalStorage] = await getQueueStats();
    const [arrStoreStats, intStoreTotalStorage] = await getStoreStats();
    
    return outputHtmlReport(arrQueueStats, intQueueTotalStorage, arrStoreStats, intStoreTotalStorage);
}

// Entry Point
async function main() {
    const args = process.argv.slice(2);
    
    if (args.includes('--file')) {
        // Write to file mode
        const html = await generateReport();
        fs.writeFileSync(CONFIG.OUTPUT_FILE, html);
        console.log(`Report generated: ${CONFIG.OUTPUT_FILE}`);
    } else {
        // HTTP server mode
        const PORT = process.env.PORT || 3001;
        
        const server = http.createServer(async (req, res) => {
            try {
                const html = await generateReport();
                res.writeHead(200, { 'Content-Type': 'text/html' });
                res.end(html);
            } catch (err) {
                console.error('Error generating report:', err);
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('Internal Server Error');
            }
        });
        
        server.listen(PORT, () => {
            console.log(`ZOSCII MQ Statissa running on http://localhost:${PORT}`);
        });
    }
}

if (require.main === module) {
    main().catch(err => {
        console.error('Error:', err);
        process.exit(1);
    });
}

module.exports = { generateReport, getQueueStats, getStoreStats, formatBytes };