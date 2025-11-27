// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Retention Cron Job (claireup.js)
// Deletes messages based on the RRRR (Retention Days) value in the filename.
//  
// Execution: node claireup.js

const fs = require('fs');
const path = require('path');
const { promisify } = require('util');

const readdir = promisify(fs.readdir);
const stat = promisify(fs.stat);
const unlink = promisify(fs.unlink);
const mkdir = promisify(fs.mkdir);

// Constants
const CONFIG = {
    CLI_ONLY: true,
    FILE_ERRORLOG: './claireup.log',
    
    DEBUG: false,
    LOG_OUTPUT: false,
    
    NONCE_ROOT: './nonce/',
    QUEUE_ROOT: './queues/',
    STORE_ROOT: './store/',
    
    TEMP_QUEUE: 'temp/',
    
    NONCE_TIMEFRAME: 5, // minimum minutes to keep NONCE
    FOLDER_PERMISSIONS: 0o755
};

function logError(str) {
    if (CONFIG.LOG_OUTPUT) {
        const timestamp = new Date().toISOString().replace('T', ' ').substring(0, 19);
        fs.appendFileSync(CONFIG.ERRORLOG, timestamp + ' - ' + str + '\n');
    }
}

function logDebug(str) {
    if (CONFIG.DEBUG) {
        console.log(str);
    }
}

async function initFolders() {
    // Ensure the root queue directory exists
    if (!fs.existsSync(CONFIG.QUEUE_ROOT)) {
        try {
            await mkdir(CONFIG.QUEUE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create root queue directory: ' + CONFIG.QUEUE_ROOT);
            process.exit(1);
        }
    }
    
    if (!fs.existsSync(CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE)) {
        try {
            await mkdir(CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create temp queue directory: ' + CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE);
            process.exit(1);
        }
    }
    
    // Ensure the root store directory exists
    if (!fs.existsSync(CONFIG.STORE_ROOT)) {
        try {
            await mkdir(CONFIG.STORE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create root store directory: ' + CONFIG.STORE_ROOT);
            process.exit(1);
        }
    }
    
    // Ensure the nonce directory exists
    if (!fs.existsSync(CONFIG.NONCE_ROOT)) {
        try {
            await mkdir(CONFIG.NONCE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create nonce directory: ' + CONFIG.NONCE_ROOT);
            process.exit(1);
        }
    }
}

async function cleanUpNonces(intCurrentTimestamp) {
    let intDeletedCount = 0;
    
    if (CONFIG.NONCE_TIMEFRAME > 0) {
        // --- NONCE Cleanup Logic ---
        console.log('Processing NONCE marker files...');
        
        // Calculate the cutoff time for deletion: Current Time - (NONCE_TIMEFRAME minutes * 60 seconds)
        const intCutoffTime = intCurrentTimestamp - (CONFIG.NONCE_TIMEFRAME * 60);
        
        // Find all files in the nonce directory
        let arrNonceFiles = [];
        if (fs.existsSync(CONFIG.NONCE_ROOT)) {
            try {
                arrNonceFiles = await readdir(CONFIG.NONCE_ROOT);
                arrNonceFiles = arrNonceFiles.map(f => path.join(CONFIG.NONCE_ROOT, f));
            } catch (err) {
                arrNonceFiles = [];
            }
        }
        
        if (!arrNonceFiles || arrNonceFiles.length === 0) {
            console.log('INFO: No NONCE files found.');
        } else {
            for (const strFullPath of arrNonceFiles) {
                try {
                    // Get the last modification time of the file (when it was created by index)
                    const stats = await stat(strFullPath);
                    const intFileModTime = Math.floor(stats.mtimeMs / 1000);
                    
                    // If the modification time is older than the calculated cutoff time, delete it.
                    if (intFileModTime < intCutoffTime) {
                        try {
                            await unlink(strFullPath);
                            intDeletedCount++;
                            const fileDate = new Date(stats.mtime).toISOString().replace('T', ' ').substring(0, 19);
                            console.log('DELETED NONCE: ' + path.basename(strFullPath) + 
                                ' (Aged out: ' + fileDate + ')');
                        } catch (err) {
                            logError('Could not delete NONCE file: ' + path.basename(strFullPath) + ' (Permission denied?)');
                        }
                    }
                } catch (err) {
                    // File might have been deleted
                }
            }
        }
    }
    
    return intDeletedCount;
}

async function cleanUpQueues(intCurrentTimestamp) {
    let intDeletedCount = 0;
    
    // Find all queue directories
    let arrQueuePaths = [];
    if (fs.existsSync(CONFIG.QUEUE_ROOT)) {
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
            arrQueuePaths = [];
        }
    }
    
    for (const strQueuePath of arrQueuePaths) {
        const strQueueName = path.basename(strQueuePath);
        console.log('Processing queue: ' + strQueueName);
        
        // Find all message files in the queue
        let arrAllFiles = [];
        try {
            const files = await readdir(strQueuePath);
            arrAllFiles = files.filter(f => f.endsWith('.bin')).map(f => path.join(strQueuePath, f));
        } catch (err) {
            arrAllFiles = [];
        }
        
        if (!arrAllFiles || arrAllFiles.length === 0) {
            console.log('INFO: Queue is empty.');
            continue;
        }
        
        // Process each file
        for (const strFullPath of arrAllFiles) {
            const strFilename = path.basename(strFullPath);
            
            // Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
            // Split the filename into parts based on the hyphen delimiter
            const arrParts = strFilename.split('-');
            
            // --- PARTS CHECK ---
            // Ensure we have at least 3 parts for a valid filename structure
            if (arrParts.length < 3) {
                console.log('WARNING: Skipping non-standard file: ' + strFilename + '.');
                continue;
            }
            
            // Assign the parts based on their index
            const strMessageTimestamp = arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
            const strRetentionString = arrParts[1];  // RRRR (4 characters)
            
            // Convert to integers
            const intRetentionDays = parseInt(strRetentionString, 10);
            
            // Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
            // *** FIX: Explicitly use the first 14 characters to guarantee a clean time
            const strTimestampPart = strMessageTimestamp.substring(0, 14);
            
            // Parse YYYYMMDDHHMMSS
            const year = parseInt(strTimestampPart.substring(0, 4), 10);
            const month = parseInt(strTimestampPart.substring(4, 6), 10) - 1; // JS months are 0-indexed
            const day = parseInt(strTimestampPart.substring(6, 8), 10);
            const hour = parseInt(strTimestampPart.substring(8, 10), 10);
            const minute = parseInt(strTimestampPart.substring(10, 12), 10);
            const second = parseInt(strTimestampPart.substring(12, 14), 10);
            
            const dtMessageTime = new Date(year, month, day, hour, minute, second);
            
            if (isNaN(dtMessageTime.getTime())) {
                console.log('WARNING: Skipping file with invalid timestamp/format: ' + strFilename +
                    ' (Retention: ' + strRetentionString + ')');
                continue;
            }
            
            const intMessageTime = Math.floor(dtMessageTime.getTime() / 1000);
            
            // Calculate the expiration time (Message Time + Retention Days in seconds)
            // 86400 seconds = 1 day
            const intExpirationTime = intMessageTime + (intRetentionDays * 86400);
            
            const expiryDate = new Date(intExpirationTime * 1000).toISOString().replace('T', ' ').substring(0, 19);
            logDebug('DEBUG: ' + strFilename + ' | Retention: ' + intRetentionDays +
                ' days | Expires: ' + expiryDate);
            
            // Check for expiration and delete
            if (intCurrentTimestamp >= intExpirationTime) {
                // Time to delete!
                try {
                    await unlink(strFullPath);
                    intDeletedCount++;
                    console.log('DELETED: ' + strFilename + ' (Expired: ' + expiryDate + ')');
                } catch (err) {
                    logError('Could not delete file: ' + strFilename + ' (Permission denied?)');
                }
            }
        }
    }
    
    return intDeletedCount;
}

async function cleanUpRecursive(strPath, intCurrentTimestamp) {
    let intDeletedCount = 0;
    
    try {
        // Recursively get all files
        const getAllFiles = async (dirPath) => {
            let files = [];
            const items = await readdir(dirPath);
            
            for (const item of items) {
                const fullPath = path.join(dirPath, item);
                const stats = await stat(fullPath);
                
                if (stats.isDirectory()) {
                    files = files.concat(await getAllFiles(fullPath));
                } else if (item.endsWith('.bin')) {
                    files.push(fullPath);
                }
            }
            
            return files;
        };
        
        const arrAllFiles = await getAllFiles(strPath);
        
        for (const strFullPath of arrAllFiles) {
            const strFilename = path.basename(strFullPath);
            
            // Expected Filename Format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
            const arrParts = strFilename.split('-');
            
            // Ensure we have at least 3 parts for a valid filename structure
            if (arrParts.length < 3) {
                console.log('WARNING: Skipping non-standard file: ' + strFilename);
                continue;
            }
            
            const strMessageTimestamp = arrParts[0]; // YYYYMMDDHHNNSSCCCC (18 characters)
            const strRetentionString = arrParts[1];  // RRRR (4 characters)
            
            // Convert to integers
            const intRetentionDays = parseInt(strRetentionString, 10);
            
            // Convert message timestamp (YYYYMMDDHHNNSS) to a Unix timestamp
            const strTimestampPart = strMessageTimestamp.substring(0, 14);
            
            // Parse YYYYMMDDHHMMSS
            const year = parseInt(strTimestampPart.substring(0, 4), 10);
            const month = parseInt(strTimestampPart.substring(4, 6), 10) - 1;
            const day = parseInt(strTimestampPart.substring(6, 8), 10);
            const hour = parseInt(strTimestampPart.substring(8, 10), 10);
            const minute = parseInt(strTimestampPart.substring(10, 12), 10);
            const second = parseInt(strTimestampPart.substring(12, 14), 10);
            
            const dtMessageTime = new Date(year, month, day, hour, minute, second);
            
            if (isNaN(dtMessageTime.getTime())) {
                console.log('WARNING: Skipping file with invalid timestamp/format: ' + strFilename +
                    ' (Retention: ' + strRetentionString + ', Timestamp: ' + strMessageTimestamp + ')');
                continue;
            }
            
            const intMessageTime = Math.floor(dtMessageTime.getTime() / 1000);
            
            // Calculate the expiration time (Message Time + Retention Days in seconds)
            const intExpirationTime = intMessageTime + (intRetentionDays * 86400);
            
            const expiryDate = new Date(intExpirationTime * 1000).toISOString().replace('T', ' ').substring(0, 19);
            logDebug('DEBUG: ' + strFilename + ' | Retention: ' + intRetentionDays +
                ' days | Expires: ' + expiryDate);
            
            // Check for expiration and delete
            if (intCurrentTimestamp >= intExpirationTime) {
                // Time to delete!
                try {
                    await unlink(strFullPath);
                    intDeletedCount++;
                    // Use relative path for cleaner output in recursive mode
                    const strRelativePath = strFullPath.replace(CONFIG.STORE_ROOT, '');
                    console.log('DELETED: ' + strRelativePath + ' (Expired: ' + expiryDate + ')');
                } catch (err) {
                    logError('Could not delete file: ' + strFullPath + ' (Permission denied?)');
                }
            }
        }
    } catch (objError) {
        // Non-fatal error during traversal
        logError('Failed to traverse directory ' + strPath + ' recursively: ' + objError.message);
        return 0;
    }
    
    return intDeletedCount;
}

async function cleanUpStore(intCurrentTimestamp) {
    let intDeletedCount = 0;
    
    // Find all top-level store directories
    let arrStorePaths = [];
    if (fs.existsSync(CONFIG.STORE_ROOT)) {
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
            arrStorePaths = [];
        }
    }
    
    for (const strStorePath of arrStorePaths) {
        const strStoreName = path.basename(strStorePath);
        console.log('Processing store (recursively): ' + strStoreName);
        
        // Use the recursive helper function to clean up the store and its subdirectories
        intDeletedCount += await cleanUpRecursive(strStorePath, intCurrentTimestamp);
    }
    
    return intDeletedCount;
}

async function handleClaireup() {
    const intCurrentTimestamp = Math.floor(Date.now() / 1000);
    let intDeletedCount = 0;
    
    const currentDate = new Date(intCurrentTimestamp * 1000).toISOString().replace('T', ' ').substring(0, 19);
    console.log('--- Starting ZOSCII MQ Claireup at ' + currentDate + ' ---');
    
    intDeletedCount += await cleanUpNonces(intCurrentTimestamp);
    intDeletedCount += await cleanUpQueues(intCurrentTimestamp);
    intDeletedCount += await cleanUpStore(intCurrentTimestamp);
    
    console.log('--- Claireup complete. Total deleted: ' + intDeletedCount + ' ---');
}

// Entry Point
async function main() {
    await initFolders();
    await handleClaireup();
}

if (require.main === module) {
    main().catch(err => {
        console.error('Error:', err);
        process.exit(1);
    });
}

module.exports = { handleClaireup, cleanUpNonces, cleanUpQueues, cleanUpStore };