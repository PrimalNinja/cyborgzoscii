// Cyborg ZOSCII v20250805
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

import React, { useState, useCallback } from 'react';
import { toZOSCII, ebcdicToAscii } from './cyborgzoscii';

const ZOSCIIEncoder = () => {
    const [message, setMessage] = useState('');
    const [romFile, setRomFile] = useState(null);
    const [encoding, setEncoding] = useState('ascii');
    const [result, setResult] = useState(null);
    const [loading, setLoading] = useState(false);

    const handleFileUpload = useCallback((event) => {
        const file = event.target.files[0];
        if (file) {
            const reader = new FileReader();
            reader.onload = (e) => {
                setRomFile(new Uint8Array(e.target.result));
            };
            reader.readAsArrayBuffer(file);
        }
    }, []);

    const handleEncode = useCallback(async () => {
        if (!message || !romFile) return;
        
        setLoading(true);
        try {
            const memoryBlocks = [{ start: 0, size: romFile.length }];
            const converter = encoding === 'ebcdic' ? ebcdicToAscii : null;
            
            const addresses = toZOSCII(romFile, message, memoryBlocks, converter, 42);
            
            setResult({
                addresses,
                stats: {
                    originalSize: message.length,
                    encodedSize: addresses.length * 2,
                    ratio: (addresses.length * 2) / message.length
                }
            });
        } catch (error) {
            console.error('Encoding failed:', error);
            setResult({ error: error.message });
        } finally {
            setLoading(false);
        }
    }, [message, romFile, encoding]);

    const downloadResult = useCallback(() => {
        if (!result || !result.addresses) return;
        
        const binaryData = new Uint8Array(result.addresses.length * 4);
        for (let i = 0; i < result.addresses.length; i++) {
            const addr = result.addresses[i];
            binaryData[i * 4] = addr & 0xFF;
            binaryData[i * 4 + 1] = (addr >> 8) & 0xFF;
            binaryData[i * 4 + 2] = (addr >> 16) & 0xFF;
            binaryData[i * 4 + 3] = (addr >> 24) & 0xFF;
        }
        
        const blob = new Blob([binaryData], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `zoscii_${Date.now()}.bin`;
        a.click();
        URL.revokeObjectURL(url);
    }, [result]);

    return (
        <div className="zoscii-encoder">
            <h2>ZOSCII Encoder</h2>
            
            <div className="form-group">
                <label>ROM File:</label>
                <input type="file" onChange={handleFileUpload} accept=".bin,.rom" />
            </div>
            
            <div className="form-group">
                <label>Message:</label>
                <textarea 
                    value={message}
                    onChange={(e) => setMessage(e.target.value)}
                    rows={4}
                    placeholder="Enter your message..."
                />
            </div>
            
            <div className="form-group">
                <label>Encoding:</label>
                <select value={encoding} onChange={(e) => setEncoding(e.target.value)}>
                    <option value="ascii">ASCII</option>
                    <option value="ebcdic">EBCDIC</option>
                    <option value="petscii">PETSCII</option>
                </select>
            </div>
            
            <button onClick={handleEncode} disabled={!message || !romFile || loading}>
                {loading ? 'Encoding...' : 'Encode Message'}
            </button>
            
            {result && (
                <div className="result">
                    {result.error ? (
                        <div className="error">Error: {result.error}</div>
                    ) : (
                        <div className="success">
                            <h3>Encoding Results</h3>
                            <p>Original size: {result.stats.originalSize} bytes</p>
                            <p>Encoded size: {result.stats.encodedSize} bytes</p>
                            <p>Size ratio: {result.stats.ratio.toFixed(2)}x</p>
                            <p>Addresses generated: {result.addresses.length}</p>
                            
                            <button onClick={downloadResult}>
                                Download Address File
                            </button>
                            
                            <details>
                                <summary>Address List</summary>
                                <pre>
                                    {result.addresses.map((addr, i) => 
                                        `${message[i]} -> 0x${addr.toString(16).toUpperCase()}`
                                    ).join('\n')}
                                </pre>
                            </details>
                        </div>
                    )}
                </div>
            )}
        </div>
    );
};

export default ZOSCIIEncoder;