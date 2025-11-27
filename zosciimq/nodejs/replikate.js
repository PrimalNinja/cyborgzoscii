// Cyborg ZOSCII MQ v20251030
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// ZOSCII MQ Server-to-Q Replikate (replikate.js)
// 
// Pulls messages from a URL and publishes them to a local Target Queue.
// State is managed using a unique file per target queue, making it concurrency-safe.
// 
// Execution Example:
// node replikate.js --url=http://other.server/index.php --sq=sourcequeue --tq=targetqueue

const fs = require('fs');
const path = require('path');
const http = require('http');
const https = require('https');
const url = require('url');
const querystring = require('querystring');
const { promisify } = require('util');

const readFile = promisify(fs.readFile);
const writeFile = promisify(fs.writeFile);
const mkdir = promisify(fs.mkdir);

// Constants
const CONFIG = {
    CLI_ONLY: false,
    FILE_ERRORLOG: './replikate.log',
    STATE_FILE_TEMPLATE: './states/replikate_state_%SOURCE%_%TARGET%.txt',
    STATE_KEY_NAME: 'last_processed_id',
    
    LOG_OUTPUT: false,
    
    LOCAL_URL: 'http://localhost/zosciimq/index.php',
    NONCE_ROOT: './nonce/',
    QUEUE_ROOT: './queues/',
    STORE_ROOT: './store/',
    TEMP_QUEUE: 'temp/',
    FOLDER_PERMISSIONS: 0o755
};

function logError(str) {
    if (CONFIG.LOG_OUTPUT) {
        const timestamp = new Date().toISOString().replace('T', ' ').substring(0, 19);
        fs.appendFileSync(CONFIG.FILE_ERRORLOG, timestamp + ' - ' + str + '\n');
    }
}

function convertNameToBase36(strName) {
    let varResult = false;
    
    const arrParts = strName.split('-');
    const strTimestamp = arrParts[0];
    
    if (strTimestamp.length === 18) {
        const SS = strTimestamp.substring(12, 14);
        const NN = strTimestamp.substring(10, 12);
        const HH = strTimestamp.substring(8, 10);
        const DD = strTimestamp.substring(6, 8);
        const MM = strTimestamp.substring(4, 6);
        const YYYY = strTimestamp.substring(0, 4);
        const CCCC = strTimestamp.substring(14, 18);
        
        const strReorderedDecimal = SS + NN + HH + DD + MM + YYYY + CCCC;
        varResult = BigInt(strReorderedDecimal).toString(36);
    }
    
    return varResult;
}

function getRetentionFromName(strName) {
    const arrParts = strName.split('-');
    
    if (arrParts.length >= 2) {
        return parseInt(arrParts[1], 10);
    }
    
    return 0;
}

function insertSuffixBeforeExtension(strName, strSuffix) {
    const strExtension = path.extname(strName);
    const strBaseName = path.basename(strName, strExtension);
    
    if (strExtension !== null && strExtension !== '') {
        return strBaseName + strSuffix + strExtension;
    } else {
        return strBaseName + strSuffix;
    }
}

async function initFolders() {
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
    
    if (!fs.existsSync(CONFIG.STORE_ROOT)) {
        try {
            await mkdir(CONFIG.STORE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create root store directory: ' + CONFIG.STORE_ROOT);
            process.exit(1);
        }
    }
    
    if (!fs.existsSync(CONFIG.NONCE_ROOT)) {
        try {
            await mkdir(CONFIG.NONCE_ROOT, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            logError('Fatal Could not create nonce directory: ' + CONFIG.NONCE_ROOT);
            process.exit(1);
        }
    }
}

function fetchNextMessage(strSourceURL, strSourceQueue, strAfterName) {
    return new Promise((resolve) => {
        const parsedUrl = url.parse(strSourceURL);
        const postData = querystring.stringify({
            action: 'fetch',
            q: strSourceQueue,
            after: strAfterName
        });
        
        const options = {
            hostname: parsedUrl.hostname,
            port: parsedUrl.port,
            path: parsedUrl.path,
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'Content-Length': Buffer.byteLength(postData)
            },
            timeout: 10000
        };
        
        const protocol = parsedUrl.protocol === 'https:' ? https : http;
        
        const req = protocol.request(options, (res) => {
            const chunks = [];
            
            res.on('data', (chunk) => {
                chunks.push(chunk);
            });
            
            res.on('end', () => {
                const binContent = Buffer.concat(chunks);
                
                if (res.statusCode !== 200) {
                    logError(`Received HTTP Status ${res.statusCode}. Server response: ${binContent.toString()}`);
                    resolve([null, null]);
                    return;
                }
                
                // Check Content-Disposition header for filename
                const contentDisposition = res.headers['content-disposition'];
                let strName = null;
                
                if (contentDisposition) {
                    const match = contentDisposition.match(/filename="([^"]+)"/i);
                    if (match) {
                        strName = path.basename(match[1]);
                    }
                }
                
                if (strName && binContent) {
                    resolve([strName, binContent]);
                } else {
                    // Try to parse as JSON
                    try {
                        const arrJSON = JSON.parse(binContent.toString());
                        if (arrJSON.system === 'ZOSCII MQ') {
                            if (arrJSON.error && arrJSON.error.length > 0) {
                                logError(`Source MQ reported JSON error: ${arrJSON.error} - Message: ${arrJSON.message}`);
                            }
                        } else {
                            logError(`Invalid JSON. Content: ${binContent.toString().substring(0, 100)}`);
                        }
                    } catch (err) {
                        logError(`Invalid response structure (missing name or content, and not valid JSON). Content: ${binContent.toString().substring(0, 100)}`);
                    }
                    resolve([null, null]);
                }
            });
        });
        
        req.on('error', (err) => {
            logError(`cURL ERROR: Failed to connect or execute request: ${err.message}`);
            resolve([null, null]);
        });
        
        req.on('timeout', () => {
            req.destroy();
            logError('Request timeout');
            resolve([null, null]);
        });
        
        req.write(postData);
        req.end();
    });
}

function publishToQueue(strTargetQueue, intRetention, binContent) {
    return new Promise((resolve) => {
        const strRetentionDays = intRetention.toString().padStart(4, '0');
        
        const postData = querystring.stringify({
            action: 'publish',
            q: strTargetQueue,
            r: strRetentionDays,
            msg: binContent.toString('base64')
        });
        
        const parsedUrl = url.parse(CONFIG.LOCAL_URL);
        
        const options = {
            hostname: parsedUrl.hostname,
            port: parsedUrl.port || 80,
            path: parsedUrl.path,
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'Content-Length': Buffer.byteLength(postData)
            },
            timeout: 30000
        };
        
        const protocol = parsedUrl.protocol === 'https:' ? https : http;
        
        const req = protocol.request(options, (res) => {
            const chunks = [];
            
            res.on('data', (chunk) => {
                chunks.push(chunk);
            });
            
            res.on('end', () => {
                const strResponse = Buffer.concat(chunks).toString();
                
                if (res.statusCode !== 200) {
                    logError(`Local POST to index failed with HTTP code: ${res.statusCode}`);
                    resolve(false);
                    return;
                }
                
                try {
                    const arrJSON = JSON.parse(strResponse);
                    if (arrJSON.error && arrJSON.error.length > 0) {
                        logError(`Local API Error: ${arrJSON.error} / System Error: ${arrJSON.system}`);
                        resolve(false);
                    } else {
                        resolve(true);
                    }
                } catch (err) {
                    logError(`Local POST returned invalid JSON: ${strResponse}`);
                    resolve(false);
                }
            });
        });
        
        req.on('error', (err) => {
            logError(`Exception during publish: ${err.message}`);
            resolve(false);
        });
        
        req.on('timeout', () => {
            req.destroy();
            logError('Publish timeout');
            resolve(false);
        });
        
        req.write(postData);
        req.end();
    });
}

async function saveToStore(strName, binMessage) {
    let blnResult = true;
    
    const strTempName = convertNameToBase36(strName);
    
    if (strTempName === false) {
        logError('Name conversion failed for: ' + strName);
        return false;
    }
    
    const strDir1 = strTempName.substring(0, 1);
    const strDir2 = strTempName.substring(1, 2);
    const strDir3 = strTempName.substring(2, 3);
    const strStorePath = CONFIG.STORE_ROOT + strDir1 + path.sep + strDir2 + path.sep + strDir3 + path.sep;
    
    if (!fs.existsSync(strStorePath)) {
        try {
            await mkdir(strStorePath, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
        } catch (err) {
            blnResult = false;
            logError('Could not create nested store directory: ' + strStorePath);
        }
    }
    
    if (blnResult) {
        strName = insertSuffixBeforeExtension(strName, '-u');
        const strFullPath = strStorePath + strName;
        
        try {
            await writeFile(strFullPath, binMessage);
        } catch (err) {
            blnResult = false;
            logError('Failed to write file to store: ' + strFullPath);
        }
    }
    
    return blnResult;
}

async function handleReplikate(strSourceURL, strSourceQueue, strTargetQueue) {
    console.log('--- Starting ZOSCII MQ Replikation ---');
    
    const strSafeSource = strSourceQueue.replace(/[^a-zA-Z0-9_-]/g, '_');
    let strSafeTarget = strTargetQueue.replace(/[^a-zA-Z0-9_-]/g, '_');
    if (strTargetQueue.length === 0) {
        strSafeTarget = 'store';
    }
    
    let strStateFilePath = CONFIG.STATE_FILE_TEMPLATE;
    strStateFilePath = strStateFilePath.replace('%SOURCE%', strSafeSource);
    strStateFilePath = strStateFilePath.replace('%TARGET%', strSafeTarget);
    
    // Ensure states directory exists
    const statesDir = path.dirname(strStateFilePath);
    if (!fs.existsSync(statesDir)) {
        await mkdir(statesDir, { recursive: true, mode: CONFIG.FOLDER_PERMISSIONS });
    }
    
    let strLastName = '';
    if (fs.existsSync(strStateFilePath)) {
        strLastName = (await readFile(strStateFilePath, 'utf8')).trim();
    }
    
    if (strLastName === '') {
        console.log('Last Processed Pointer: START');
    } else {
        console.log('Last Processed Pointer: ' + strLastName);
    }
    
    let intTotalReplikated = 0;
    
    while (true) {
        const [strName, binContent] = await fetchNextMessage(strSourceURL, strSourceQueue, strLastName);
        
        if (strName === null) {
            console.log('Source URL is caught up or returned no data. Halting this run.');
            break;
        }
        
        let blnSuccess = false;
        
        if (strTargetQueue.length > 0) {
            // Save to queue
            const intRetention = getRetentionFromName(strName);
            blnSuccess = await publishToQueue(strTargetQueue, intRetention, binContent);
            
            if (blnSuccess) {
                strLastName = strName;
                intTotalReplikated++;
                console.log('REPLIKATED: New pointer set to ' + strLastName);
            } else {
                logError('Failed to replicate message to ' + strTargetQueue + '. Halting.');
                console.log('Failed to replicate message to ' + strTargetQueue + '. Halting.');
                break;
            }
        } else {
            // Save to store
            blnSuccess = await saveToStore(strName, binContent);
            
            if (blnSuccess) {
                strLastName = strName;
                intTotalReplikated++;
            } else {
                logError('Failed to replicate message to store. Halting.');
                console.log('Failed to replicate message to store. Halting.');
                break;
            }
        }
    }
    
    await writeFile(strStateFilePath, strLastName);
    console.log('--- Replikation finished. Total messages replikated: ' + intTotalReplikated + ' ---');
}

// Entry Point
async function main() {
    // Parse command-line arguments
    let strSourceURL = '';
    let strSourceQueue = '';
    let strTargetQueue = '';
    
    process.argv.forEach(arg => {
        if (arg.startsWith('--url=')) {
            strSourceURL = arg.substring(6);
        } else if (arg.startsWith('--sq=')) {
            strSourceQueue = arg.substring(5);
        } else if (arg.startsWith('--tq=')) {
            strTargetQueue = arg.substring(5);
        }
    });
    
    if (!strSourceURL || !strSourceQueue) {
        logError('Missing required arguments.');
        console.log('Missing required arguments.');
        console.log('Usage: node replikate.js --url=<source_url> --sq=<source_queue> [--tq=<target_queue>]');
        process.exit(1);
    }
    
    await initFolders();
    await handleReplikate(strSourceURL, strSourceQueue, strTargetQueue);
}

if (require.main === module) {
    main().catch(err => {
        console.error('Error:', err);
        process.exit(1);
    });
}

module.exports = { handleReplikate, fetchNextMessage, publishToQueue, saveToStore };