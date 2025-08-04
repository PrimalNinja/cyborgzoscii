// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

const fs = require('fs');
const express = require('express');
const app = express();

// Load ZOSCII library
const { toZOSCII, petsciiToAscii, ebcdicToAscii } = require('./cyborgzoscii');

app.post('/encode', express.json(), (req, res) => {
    try {
        const { message, romFile, encoding } = req.body;
        
        // Load ROM data
        const romData = fs.readFileSync(romFile);
        const memoryBlocks = [{ start: 0, size: romData.length }];
        
        // Select converter
        let converter = null;
        switch (encoding) {
            case 'petscii': converter = petsciiToAscii; break;
            case 'ebcdic': converter = ebcdicToAscii; break;
        }
        // Encode message
        const addresses = toZOSCII(romData, message, memoryBlocks, converter, 42);
        
        res.json({
            success: true,
            addresses: addresses,
            metadata: {
                originalLength: message.length,
                encodedLength: addresses.length,
                romFile: romFile,
                encoding: encoding || 'ascii'
            }
        });
        
    } catch (error) {
        res.status(400).json({
            success: false,
            error: error.message
        });
    }
});

app.get('/decode/:addressFile', (req, res) => {
    try {
        const addressFile = req.params.addressFile;
        const romFile = req.query.rom;
        
        // Load address data and ROM
        const addressData = fs.readFileSync(addressFile);
        const romData = fs.readFileSync(romFile);
        
        // Parse addresses from binary file
        const addresses = [];
        for (let i = 0; i < addressData.length; i += 4) {
            if (i + 3 < addressData.length) {
                const addr = addressData.readUInt32LE(i);
                addresses.push(addr);
            }
        }
        
        // Decode message
        let message = '';
        for (let addr of addresses) {
            if (addr < romData.length) {
                message += String.fromCharCode(romData[addr]);
            }
        }
        
        res.json({
            success: true,
            message: message,
            metadata: {
                addressCount: addresses.length,
                decodedLength: message.length,
                romFile: romFile
            }
        });
        
    } catch (error) {
        res.status(400).json({
            success: false,
            error: error.message
        });
    }
});

app.listen(3000, () => {
    console.log('ZOSCII server running on port 3000');
});