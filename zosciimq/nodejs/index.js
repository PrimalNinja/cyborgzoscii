// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ (Node.js Version)
// Message queue and storage system with ZOSCII addressing

const fs = require('fs');
const path = require('path');
const { promisify } = require('util');
const crypto = require('crypto');

// Promisify filesystem operations
const readdir = promisify(fs.readdir);
const stat = promisify(fs.stat);
const readFile = promisify(fs.readFile);
const writeFile = promisify(fs.writeFile);
const unlink = promisify(fs.unlink);
const mkdir = promisify(fs.mkdir);
const rename = promisify(fs.rename);

// Constants
const CONFIG = {
    DEBUG: false,
    LOG_OUTPUT: false,
    
    LOCAL_URL: 'http://localhost/zosciimq/index.php',
    FOLDER_PERMISSIONS: 0o755,
    
    NONCE_ROOT: './nonce/',
    QUEUE_ROOT: './queues/',
    STORE_ROOT: './store/',
    
    TEMP_QUEUE: 'temp/',
    
    LOCK_FOLDER: 'locks/',
    LOCK_WAIT: 10000, // in microseconds
    LOCK_TIMEFRAME: 5, // in seconds to keep the lock
    
    NONCE_TIMEFRAME: 5, // minimum minutes to keep NONCE
    
    ALLOW_FETCH: true,
    ALLOW_GET: false,
    ALLOW_IDENTIFY: false,
    ALLOW_PUBLISH: false,
    ALLOW_RETRIEVE: false,
    ALLOW_SCAN: false,
    ALLOW_STORE: false,
    
    FILE_ERRORLOG: './zosciimq.log'
};

// Utilities
class Utils {
    // Converts the name timestamp part (YYYYMMDDHHNNSSCCCC) to a reordered 
    // decimal number (SSNNHHDDMMYYYYCCCC) and encodes it in BASE36.
    static convertNameToBase36(strName) {
        let varResult = false;
        
        // Extract the timestamp part from YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
        const arrParts = strName.split('-');
        
        // We only care about the first part (timestamp)
        const strTimestamp = arrParts[0];
        
        if (strTimestamp.length === 18) {
            // Reorder the parts: SS NN HH DD MM YYYY CCCC
            const SS = strTimestamp.substring(12, 14);
            const NN = strTimestamp.substring(10, 12);
            const HH = strTimestamp.substring(8, 10);
            const DD = strTimestamp.substring(6, 8);
            const MM = strTimestamp.substring(4, 6);
            const YYYY = strTimestamp.substring(0, 4);
            const CCCC = strTimestamp.substring(14, 18);
            
            // Concatenate to form the large decimal number
            const strReorderedDecimal = SS + NN + HH + DD + MM + YYYY + CCCC;
            
            // Convert to BASE36 (0-9 and a-z)
            varResult = BigInt(strReorderedDecimal).toString(36);
        }
        
        return varResult;
    }
    
    static getGUID() {
        return crypto.randomUUID();
    }
    
    static async initFolders() {
        // Ensure the root queue directory exists
        if (!fs.existsSync(CONFIG.QUEUE_ROOT)) {
            try {
                await mkdir(CONFIG.QUEUE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
            } catch (err) {
                Utils.logError('Fatal Could not create root queue directory: ' + CONFIG.QUEUE_ROOT);
                process.exit(1);
            }
        }
        
        if (!fs.existsSync(CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE)) {
            try {
                await mkdir(CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
            } catch (err) {
                Utils.logError('Fatal Could not create temp queue directory: ' + CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE);
                process.exit(1);
            }
        }
        
        // Ensure the root store directory exists
        if (!fs.existsSync(CONFIG.STORE_ROOT)) {
            try {
                await mkdir(CONFIG.STORE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
            } catch (err) {
                Utils.logError('Fatal Could not create root store directory: ' + CONFIG.STORE_ROOT);
                process.exit(1);
            }
        }
        
        // Ensure the nonce directory exists
        if (!fs.existsSync(CONFIG.NONCE_ROOT)) {
            try {
                await mkdir(CONFIG.NONCE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
            } catch (err) {
                Utils.logError('Fatal Could not create nonce directory: ' + CONFIG.NONCE_ROOT);
                process.exit(1);
            }
        }
    }
    
    static getRetentionFromName(strName) {
        // Name format: YYYYMMDDHHNNSSCCCC-RRRR-GUID.bin
        // The retention value (RRRR) is the second segment.
        const arrParts = strName.split('-');
        
        // Check if the parts exist (we need at least two segments for retention)
        if (arrParts.length >= 2) {
            // Cast to integer to get the numeric value from the RRRR segment (index 1)
            return parseInt(arrParts[1], 10);
        }
        
        // Default to 0 days retention if the format is invalid
        return 0;
    }
    
    static insertSuffixBeforeExtension(strName, strSuffix) {
        let strResult = '';
        
        const strExtension = path.extname(strName);
        const strBaseName = path.basename(strName, strExtension);
        
        // Recombine the new base name and the extension
        if (strExtension !== null && strExtension !== '') {
            strResult = strBaseName + strSuffix + strExtension;
        } else {
            // Insert the suffix into the base name
            strResult = strBaseName + strSuffix;
        }
        
        return strResult;
    }
    
    static logDebug(str) {
        if (CONFIG.DEBUG) {
            console.log(str);
        }
    }
    
    static logError(str) {
        if (CONFIG.LOG_OUTPUT) {
            const timestamp = new Date().toISOString().replace('T', ' ').substring(0, 19);
            fs.appendFileSync(CONFIG.FILE_ERRORLOG, timestamp + ' - ' + str + '\n');
        }
    }
    
    static sendJSONResponse(res, strSystemError, strError, strMessage, arrResult) {
        const arrJSON = {
            system: 'ZOSCII MQ',
            version: '1.0',
            error: strError,
            message: strMessage,
            result: arrResult
        };
        
        if (strSystemError && strSystemError.length > 0) {
            Utils.logError(strSystemError);
        }
        
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify(arrJSON));
    }
}

// Handlers
class Handlers {
    static async cleanUpLocks(strLocksPath) {
        let intResult = 0;
        
        const intCutoffTime = Date.now() / 1000 - CONFIG.LOCK_TIMEFRAME;
        
        // Find all .lock files
        let arrLockFiles = [];
        if (fs.existsSync(strLocksPath)) {
            try {
                const files = await readdir(strLocksPath);
                arrLockFiles = files.filter(f => f.endsWith('.lock')).map(f => path.join(strLocksPath, f));
            } catch (err) {
                arrLockFiles = [];
            }
        }
        
        if (arrLockFiles && arrLockFiles.length > 0) {
            Utils.logError(`cleanUpLocks: Found ${arrLockFiles.length} lock files`);
            
            for (const strLockFile of arrLockFiles) {
                try {
                    const stats = await stat(strLockFile);
                    const intFileMTime = stats.mtimeMs / 1000;
                    
                    if (intFileMTime < intCutoffTime) {
                        try {
                            await unlink(strLockFile);
                        } catch (err) {
                            Utils.logError('Failed to delete stale lock file: ' + strLockFile);
                        }
                    }
                } catch (err) {
                    // File might have been deleted
                }
            }
        } else {
            Utils.logError('cleanUpLocks: glob did NOT return an array!');
        }
        
        // Re-scan
        if (fs.existsSync(strLocksPath)) {
            try {
                const files = await readdir(strLocksPath);
                arrLockFiles = files.filter(f => f.endsWith('.lock'));
                intResult = arrLockFiles.length;
            } catch (err) {
                intResult = 0;
            }
        }
        
        return intResult;
    }
    
    static async findUnidentifiedFilesRecursive(strPath) {
        let arrResult = [];
        
        try {
            const arrItems = await readdir(strPath);
            
            for (const strItem of arrItems) {
                if (strItem === '.' || strItem === '..') {
                    continue;
                }
                
                const strFullPath = path.join(strPath, strItem);
                
                try {
                    const stats = await stat(strFullPath);
                    
                    if (stats.isDirectory()) {
                        // Recursively search subdirectories
                        const subResults = await Handlers.findUnidentifiedFilesRecursive(strFullPath);
                        arrResult = arrResult.concat(subResults);
                    } else if (stats.isFile()) {
                        // Check specifically for '-u' before the file extension (e.g., blah-u.bin)
                        if (/-u\.[^.]+$/i.test(strItem)) {
                            arrResult.push(strItem);
                        }
                    }
                } catch (err) {
                    // Skip files we can't access
                }
            }
        } catch (err) {
            // Directory not accessible
        }
        
        return arrResult;
    }
    
    static async handleFetch(res, strQueueName, strAfterName, intOffset, intLength) {
        const strQueuePath = CONFIG.QUEUE_ROOT + strQueueName + path.sep;
        const strLockPath = CONFIG.QUEUE_ROOT + strQueueName + path.sep + CONFIG.LOCK_FOLDER;
        
        if (fs.existsSync(strQueuePath)) {
            // wait for lock to become free
            while (await Handlers.cleanUpLocks(strLockPath) > 0) {
                await new Promise(resolve => setTimeout(resolve, CONFIG.LOCK_WAIT / 1000));
            }
            
            // Get all message files, sorted chronologically by name
            let arrAllFiles = [];
            try {
                const files = await readdir(strQueuePath);
                arrAllFiles = files.filter(f => f.endsWith('.bin')).map(f => path.join(strQueuePath, f));
            } catch (err) {
                arrAllFiles = [];
            }
            
            if (!arrAllFiles || arrAllFiles.length === 0) {
                Utils.sendJSONResponse(res, '', '', 'Queue is empty.', []);
                return;
            } else {
                // Sort files alphabetically (chronologically by name)
                arrAllFiles.sort();
                let strNextMessagePath = null;
                
                let blnFoundAfter = false;
                // Check if client is requesting from the start (empty 'after')
                if (!strAfterName || strAfterName === '') {
                    blnFoundAfter = true;
                }
                
                // Find the next message after the 'after' pointer
                for (const strFullPath of arrAllFiles) {
                    const strName = path.basename(strFullPath);
                    if (blnFoundAfter) {
                        // This is the first file after the 'after' pointer
                        strNextMessagePath = strFullPath;
                        break;
                    }
                    if (strName === strAfterName) {
                        // Found the pointer; the next iteration's file will be the message to return.
                        blnFoundAfter = true;
                    }
                }
                
                if (strNextMessagePath) {
                    const strName = path.basename(strNextMessagePath);
                    
                    if (intOffset === 0 && intLength === 0) {
                        res.setHeader('Content-Type', 'application/octet-stream');
                        res.setHeader('Content-Disposition', `attachment; filename="${strName}"`);
                        
                        const fileBuffer = await readFile(strNextMessagePath);
                        res.end(fileBuffer);
                        return;
                    }
                    
                    const stats = await stat(strNextMessagePath);
                    const intFileSize = stats.size;
                    
                    if (intOffset < 0 || intOffset >= intFileSize) {
                        Utils.sendJSONResponse(res, '', 'Invalid offset.', '', []);
                        return;
                    }
                    
                    if (intLength <= 0 || (intOffset + intLength) > intFileSize) {
                        intLength = intFileSize - intOffset;
                    }
                    
                    res.setHeader('Content-Type', 'application/octet-stream');
                    res.setHeader('Content-Disposition', `attachment; filename="${strName}"`);
                    res.setHeader('Content-Length', intLength);
                    res.setHeader('X-ZOSCII-Offset', intOffset);
                    res.setHeader('X-ZOSCII-Total-Length', intFileSize);
                    
                    const fileHandle = fs.createReadStream(strNextMessagePath, {
                        start: intOffset,
                        end: intOffset + intLength - 1
                    });
                    fileHandle.pipe(res);
                    return;
                } else {
                    Utils.sendJSONResponse(res, '', '', `No new messages found after '${strAfterName}' (or queue is empty).`, []);
                    return;
                }
            }
        } else {
            Utils.sendJSONResponse(res, '', `Queue '${strQueueName}' does not exist.`, '', []);
            return;
        }
    }
    
    static async handleIdentify(res, arrNames) {
        const arrResult = [];
        
        for (const strName of arrNames) {
            const strBaseName = path.basename(strName);
            
            const strExtension = path.extname(strBaseName);
            const intExtensionLength = strExtension.length;
            const intSuffixLength = intExtensionLength + 2; // "-u"
            
            if (strBaseName.length >= intSuffixLength && 
                strBaseName.substring(strBaseName.length - intSuffixLength, strBaseName.length - intExtensionLength) === '-u') {
                const strTempName = Utils.convertNameToBase36(strBaseName);
                
                if (strTempName !== false) {
                    const strDir1 = strTempName.substring(0, 1);
                    const strDir2 = strTempName.substring(1, 2);
                    const strDir3 = strTempName.substring(2, 3);
                    const strStorePath = CONFIG.STORE_ROOT + strDir1 + path.sep + strDir2 + path.sep + strDir3 + path.sep;
                    
                    const strFullCurrentPath = strStorePath + strBaseName;
                    
                    if (fs.existsSync(strFullCurrentPath)) {
                        // Removes the '-u' suffix
                        const strNewName = strBaseName.substring(0, strBaseName.length - intSuffixLength) + strExtension;
                        const strFullNewPath = strStorePath + strNewName;
                        
                        try {
                            await rename(strFullCurrentPath, strFullNewPath);
                            arrResult.push(strNewName);
                        } catch (err) {
                            // Failed to rename
                        }
                    }
                }
            }
        }
        
        Utils.sendJSONResponse(res, '', '', 'Returned messages identified.', arrResult);
    }
    
    static handleNonce(res, strNonce) {
        const strNonceFile = CONFIG.NONCE_ROOT + strNonce;
        if (fs.existsSync(strNonceFile)) {
            Utils.sendJSONResponse(res, '', '', 'Nonce already used.', []);
        }
    }
    
    static async handlePublish(res, strQueueName, strNonce, intRetentionDays, binMessage) {
        // Format RRRR, e.g., 3 becomes 0003
        const strRetentionDays = intRetentionDays.toString().padStart(4, '0');
        
        if (!binMessage || binMessage.length === 0) {
            Utils.sendJSONResponse(res, '', 'Message required.', '', []);
            return;
        } else {
            const strQueuePath = CONFIG.QUEUE_ROOT + strQueueName + path.sep;
            const strLockPath = strQueuePath + CONFIG.LOCK_FOLDER;
            
            if (!fs.existsSync(strQueuePath)) {
                try {
                    await mkdir(strQueuePath, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
                } catch (err) {
                    Utils.sendJSONResponse(res, 'index.js: Could not create queue directory: ' + strQueuePath,
                                         'Could not create queue.', '', []);
                    return;
                }
            }
            
            if (!fs.existsSync(strLockPath)) {
                try {
                    await mkdir(strLockPath, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
                } catch (err) {
                    Utils.sendJSONResponse(res, 'index.js: Could not create lock directory: ' + strLockPath,
                                         'Could not create queue.', '', []);
                    return;
                }
            }
            
            // wait for lock to become free
            while (await Handlers.cleanUpLocks(strLockPath) > 0) {
                await new Promise(resolve => setTimeout(resolve, CONFIG.LOCK_WAIT / 1000));
            }
            
            const strLockFile = Utils.getGUID() + '.lock';
            const strLockFilePath = strLockPath + strLockFile;
            
            try {
                await writeFile(strLockFilePath, '');
                
                const now = new Date();
                const strBaseTime = now.getFullYear().toString() +
                                  (now.getMonth() + 1).toString().padStart(2, '0') +
                                  now.getDate().toString().padStart(2, '0') +
                                  now.getHours().toString().padStart(2, '0') +
                                  now.getMinutes().toString().padStart(2, '0') +
                                  now.getSeconds().toString().padStart(2, '0');
                
                let intCollisionCounter = 0;
                let strName = '';
                let strFullPath = '';
                const strFullTempPath = CONFIG.QUEUE_ROOT + CONFIG.TEMP_QUEUE + Utils.getGUID() + '.bin';
                
                // Write to temp location first
                await writeFile(strFullTempPath, binMessage);
                
                // Generate unique name with collision handling
                while (true) {
                    const strCollisionCounter = intCollisionCounter.toString().padStart(4, '0');
                    strName = strBaseTime + strCollisionCounter + '-' + strRetentionDays + '-' + Utils.getGUID() + '.bin';
                    strFullPath = strQueuePath + strName;
                    
                    // Check for existence. If unique, break the loop.
                    if (!fs.existsSync(strFullPath)) {
                        break;
                    }
                    
                    // If file exists, we had a collision. Try the next sequential number.
                    intCollisionCounter++;
                    
                    // Safety break: Prevents an infinite loop.
                    if (intCollisionCounter > 9999) {
                        try { await unlink(strLockFilePath); } catch (err) {}
                        Utils.sendJSONResponse(res, 'index.js: Queue exceeded 9,999 attempted messages in one second.',
                                             'Queue overload, try again.', '', []);
                        return;
                    }
                }
                
                try {
                    await rename(strFullTempPath, strFullPath);
                } catch (err) {
                    try { await unlink(strFullTempPath); } catch (e) {}
                    try { await unlink(strLockFilePath); } catch (e) {}
                    Utils.sendJSONResponse(res, 'index.js: Failed to create message.', 'Failed to create message.', '', []);
                    return;
                }
                
                if (strNonce && strNonce.length > 0) {
                    const strNonceFile = CONFIG.NONCE_ROOT + strNonce;
                    try {
                        await writeFile(strNonceFile, '');
                    } catch (err) {
                        try { await unlink(strFullPath); } catch (e) {}
                        try { await unlink(strLockFilePath); } catch (e) {}
                        Utils.sendJSONResponse(res, 'index.js: Failed to create nonce.', 'Failed to create nonce.', '', []);
                        return;
                    }
                }
                
                try { await unlink(strLockFilePath); } catch (err) {}
                Utils.sendJSONResponse(res, '', '', 'Message published.', []);
            } finally {
                try { await unlink(strLockFilePath); } catch (err) {}
            }
        }
    }
    
    static async handleRetrieve(res, strName) {
        if (!strName || strName === '') {
            Utils.sendJSONResponse(res, '', "Missing 'name' argument for retrieve action.", '', []);
            return;
        } else {
            const strTempName = Utils.convertNameToBase36(strName);
            
            if (strTempName === false) {
                Utils.sendJSONResponse(res, '', `Invalid name '${strName}'.`, '', []);
                return;
            } else {
                // Extract first 3 chars for nested path
                const strDir1 = strTempName.substring(0, 1);
                const strDir2 = strTempName.substring(1, 2);
                const strDir3 = strTempName.substring(2, 3);
                const strStorePath = CONFIG.STORE_ROOT + strDir1 + path.sep + strDir2 + path.sep + strDir3 + path.sep;
                
                const strFullPath = strStorePath + strName;
                
                if (fs.existsSync(strFullPath)) {
                    res.setHeader('Content-Type', 'application/octet-stream');
                    res.setHeader('Content-Disposition', `attachment; filename="${strName}"`);
                    
                    const fileBuffer = await readFile(strFullPath);
                    res.end(fileBuffer);
                    return;
                } else {
                    Utils.sendJSONResponse(res, '', 'Message not found.', '', []);
                    return;
                }
            }
        }
    }
    
    static async handleScan(res) {
        const arrResult = await Handlers.findUnidentifiedFilesRecursive(CONFIG.STORE_ROOT);
        Utils.sendJSONResponse(res, '', '', '', arrResult);
    }
    
    static async handleStore(res, strNonce, intRetentionDays, binMessage) {
        // Format RRRR, e.g., 3 becomes 0003
        const strRetentionDays = intRetentionDays.toString().padStart(4, '0');
        
        if (!binMessage || binMessage.length === 0) {
            Utils.sendJSONResponse(res, '', 'Message required.', '', []);
            return;
        } else {
            const now = new Date();
            const strBaseTime = now.getFullYear().toString() +
                              (now.getMonth() + 1).toString().padStart(2, '0') +
                              now.getDate().toString().padStart(2, '0') +
                              now.getHours().toString().padStart(2, '0') +
                              now.getMinutes().toString().padStart(2, '0') +
                              now.getSeconds().toString().padStart(2, '0');
            
            let strName = '';
            let strFullPath = '';
            const strGetGUID = Utils.getGUID();
            
            strName = strBaseTime + '0000-' + strRetentionDays + '-' + strGetGUID + '.bin';
            
            const strTempName = Utils.convertNameToBase36(strName);
            
            // Extract first 3 chars for nested path
            const strDir1 = strTempName.substring(0, 1);
            const strDir2 = strTempName.substring(1, 2);
            const strDir3 = strTempName.substring(2, 3);
            const strStorePath = CONFIG.STORE_ROOT + strDir1 + path.sep + strDir2 + path.sep + strDir3 + path.sep;
            
            strFullPath = strStorePath + strName;
            
            if (!fs.existsSync(strStorePath)) {
                try {
                    await mkdir(strStorePath, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
                } catch (err) {
                    Utils.sendJSONResponse(res, 'index.js: Could not create store directory: ' + strStorePath,
                                         'Could not create store.', '', []);
                    return;
                }
            }
            
            try {
                await writeFile(strFullPath, binMessage);
            } catch (err) {
                Utils.sendJSONResponse(res, 'index.js: Failed to create message.', 'Failed to create message.', '', []);
                return;
            }
            
            if (strNonce && strNonce.length > 0) {
                const strNonceFile = CONFIG.NONCE_ROOT + strNonce;
                try {
                    await writeFile(strNonceFile, '');
                } catch (err) {
                    try { await unlink(strFullPath); } catch (e) {}
                    Utils.sendJSONResponse(res, 'index.js: Failed to create nonce.', 'Failed to create nonce.', '', []);
                    return;
                }
            }
            
            Utils.sendJSONResponse(res, '', '', 'Message stored.', strName);
        }
    }
}

// HTTP Server Setup
const http = require('http');
const url = require('url');
const querystring = require('querystring');

async function handleRequest(req, res) {
    const parsedUrl = url.parse(req.url);
    const query = querystring.parse(parsedUrl.query);
    
    // Get parameters
    let binMessage = Buffer.alloc(0);
    let strAction = '';
    let strAfterName = '';
    let strLength = '';
    let strName = '';
    let strNames = '';
    let strNonce = '';
    let strOffset = '';
    let strQueueName = '';
    let strRetentionDays = '';
    
    if (CONFIG.ALLOW_GET) {
        if (query.action) strAction = query.action;
        if (query.after) strAfterName = query.after;
        if (query.length) strLength = query.length;
        if (query.name) strName = query.name;
        if (query.names) strNames = query.names;
        if (query.msg) binMessage = Buffer.from(query.msg);
        if (query.n) strNonce = query.n;
        if (query.offset) strOffset = query.offset;
        if (query.q) strQueueName = query.q;
        if (query.r) strRetentionDays = query.r;
    }
    
    if (req.method === 'POST') {
        const chunks = [];
        for await (const chunk of req) {
            chunks.push(chunk);
        }
        const body = Buffer.concat(chunks);
        const postData = querystring.parse(body.toString());
        
        if (!strAction && postData.action) strAction = postData.action;
        if (binMessage.length === 0 && postData.msg) binMessage = Buffer.from(postData.msg);
        if (!strAfterName && postData.after) strAfterName = postData.after;
        if (!strLength && postData.length) strLength = postData.length;
        if (!strName && postData.name) strName = postData.name;
        if (!strNames && postData.names) strNames = postData.names;
        if (!strNonce && postData.n) strNonce = postData.n;
        if (!strOffset && postData.offset) strOffset = postData.offset;
        if (!strQueueName && postData.q) strQueueName = postData.q;
        if (!strRetentionDays && postData.r) strRetentionDays = postData.r;
    }
    
    let arrNames = [];
    if (strNames.length > 0) {
        try {
            arrNames = JSON.parse(strNames);
        } catch (err) {
            arrNames = [];
        }
    }
    
    strAfterName = path.basename(strAfterName);
    strName = path.basename(strName);
    strNonce = strNonce.replace(/[^a-zA-Z0-9_-]/g, '');
    strQueueName = strQueueName.replace(/[^a-zA-Z0-9_-]/g, '').toLowerCase();
    const intRetentionDays = parseInt(strRetentionDays, 10) || 0;
    const intOffset = parseInt(strOffset, 10) || 0;
    const intLength = parseInt(strLength, 10) || 0;
    
    if (!strAction || strAction === '') {
        res.end('Welcome to the ZOSCII MQ.');
        return;
    }
    
    if (strNonce.length > 0) {
        Handlers.handleNonce(res, strNonce);
    }
    
    // Router
    switch (strAction) {
        case 'fetch':
            if (CONFIG.ALLOW_FETCH) {
                await Handlers.handleFetch(res, strQueueName, strAfterName, intOffset, intLength);
            }
            break;
            
        case 'identify':
            if (CONFIG.ALLOW_IDENTIFY) {
                await Handlers.handleIdentify(res, arrNames);
            }
            break;
            
        case 'publish':
            if (CONFIG.ALLOW_PUBLISH) {
                await Handlers.handlePublish(res, strQueueName, strNonce, intRetentionDays, binMessage);
            }
            break;
            
        case 'retrieve':
            if (CONFIG.ALLOW_RETRIEVE) {
                await Handlers.handleRetrieve(res, strName);
            }
            break;
            
        case 'scan':
            if (CONFIG.ALLOW_SCAN) {
                await Handlers.handleScan(res);
            }
            break;
            
        case 'store':
            if (CONFIG.ALLOW_STORE) {
                await Handlers.handleStore(res, strNonce, intRetentionDays, binMessage);
            }
            break;
            
        default:
            Utils.sendJSONResponse(res, '', `Unknown action '${strAction}'.`, '', []);
    }
}

// Entry Point
async function main() {
    await Utils.initFolders();
    
    const server = http.createServer((req, res) => {
        handleRequest(req, res).catch(err => {
            console.error('Request error:', err);
            res.statusCode = 500;
            res.end('Internal Server Error');
        });
    });
    
    const PORT = process.env.PORT || 3000;
    server.listen(PORT, () => {
        console.log(`ZOSCII MQ Node.js Server running on port ${PORT}`);
    });
}

if (require.main === module) {
    main();
}

module.exports = { Utils, Handlers, CONFIG };