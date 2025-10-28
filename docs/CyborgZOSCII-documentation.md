# CyborgZOSCII Documentation

## Overview

**CyborgZOSCII** (Zero Overhead Secure Code Information Interchange) is an innovative character encoding system that replaces traditional ASCII/PETSCII lookup tables with direct ROM memory addressing. Instead of storing character codes, ZOSCII encodes text as sequences of memory addresses that point to locations in ROM containing the corresponding byte values.

## Key Features

- **Zero ROM Overhead**: Eliminates traditional character lookup tables
- **Performance Optimized**: Up to 30% faster text output on some 8-bit systems
- **Built-in Security**: Content obfuscation through address indirection
- **Memory Efficient**: Uses "sparse" ROM data as character encoding space
- **Randomized Output**: Same message generates different encodings each time

## How It Works

### 1. Address Mapping Phase
ZOSCII scans ROM memory and creates arrays of addresses for each possible byte value (0-255):
```
Byte Value 72 ('H') found at addresses: [0x4A2F, 0x8C15, 0xF003, ...]
Byte Value 69 ('E') found at addresses: [0x7B81, 0x9A43, 0xE127, ...]
```

### 2. Encoding Phase
For each character in the input string:
1. Get the character's byte value (e.g., 'H' = 72)
2. Randomly select one address from that byte value's address array
3. Store the selected address in the result sequence

### 3. Decoding Phase
To reconstruct the original message:
1. For each address in the sequence
2. Read the byte value at that ROM location
3. Convert byte value back to character

### Example
```
Input:  "HI"
ROM:    Address 0x4A2F contains byte 72 ('H')
        Address 0x7B81 contains byte 73 ('I')
Output: [0x4A2F, 0x7B81]
```

## Use Cases

### Performance Applications
- **8-bit System Optimization**: Eliminate lookup table overhead
- **Real-time Text Display**: Faster character output for games/demos
- **Embedded Systems**: Reduce ROM usage in memory-constrained devices

### Security Applications
- **Content Obfuscation**: Hide message content as memory addresses  
- **Steganographic Communication**: Conceal data in binary files
- **Invite-only Websites**: Content invisible without ROM key
- **Digital Rights Management**: Protect content from unauthorized access

### Development Applications
- **Retro Computing**: Optimize performance on vintage systems
- **Demo Scene**: Create novel effects and hidden messages
- **Educational**: Demonstrate alternative encoding approaches

## JavaScript Library Reference

### Core Function: toZOSCII()

```javascript
function toZOSCII(arrBinaryData, strInputString, arrMemoryBlocks, cbConverter, intUnmappableChar)
```

**Parameters:**
- `arrBinaryData` - `Uint8Array` containing ROM/binary data
- `strInputString` - String message to encode
- `arrMemoryBlocks` - Array of memory block objects: `{start: startAddress, size: blockSize}`
- `cbConverter` - Character conversion function (optional, can be `null`)
- `intUnmappableChar` - Fallback character code for unmappable characters (suggest: 42 for '*')

**Returns:** Array of memory addresses representing the encoded message

**Example Usage:**
```javascript
// Define valid memory regions
var memoryBlocks = [
    {start: 0xC000, size: 0x1000},  // ROM at C000-CFFF
    {start: 0xE000, size: 0x0800}   // Additional ROM at E000-E7FF
];

// Load ROM data
var romData = new Uint8Array(65536); // Your ROM data here

// Encode message
var addresses = toZOSCII(romData, "Hello, World!", memoryBlocks, null, 42);
console.log(addresses); // [0x4A2F, 0x8C15, 0x7B81, ...]
```

### Character Conversion Functions

#### petsciiToAscii()
```javascript
function petsciiToAscii(intPetsciiChar, intUnmappableChar)
```
Converts PETSCII character codes to ASCII equivalents.

#### ebcdicToAscii()  
```javascript
function ebcdicToAscii(intEbcdicChar, intUnmappableChar)
```
Converts EBCDIC character codes to ASCII equivalents.

**Usage with Converter:**
```javascript
// Encode EBCDIC text to ZOSCII addresses
var addresses = toZOSCII(romData, inputText, memoryBlocks, ebcdicToAscii, 42);
```

## PHP API Reference

### Address File Decoder

The PHP component reads binary address files and converts them to JSON arrays:

```php
// Usage: addresses.php?file=filename.bin
<?php
$filename = $_GET['file'];

if (file_exists($filename)) {
    $binaryData = file_get_contents($filename);
    $addresses = array();
    
    // Convert 4-byte little-endian chunks to integers
    for ($i = 0; $i < strlen($binaryData); $i += 4) {
        if ($i + 3 < strlen($binaryData)) {
            $byte1 = ord($binaryData[$i]);
            $byte2 = ord($binaryData[$i + 1]);
            $byte3 = ord($binaryData[$i + 2]);
            $byte4 = ord($binaryData[$i + 3]);
            
            $address = $byte1 | ($byte2 << 8) | ($byte3 << 16) | ($byte4 << 24);
            $addresses[] = $address;
        }
    }
    
    header('Content-Type: application/json');
    echo json_encode($addresses);
} else {
    header('Content-Type: application/json');
    echo json_encode(array('error' => 'File not found'));
}
?>
```

## Complete Workflow Example

### 1. Encoding (JavaScript)
```javascript
// Step 1: Load ROM data
var romFile = new Uint8Array(/* your ROM file data */);

// Step 2: Define memory blocks
var blocks = [{start: 0x0000, size: romFile.length}];

// Step 3: Encode message
var message = "Secret message!";
var addresses = toZOSCII(romFile, message, blocks, null, 42);

// Step 4: Save as binary file
var binaryData = new Uint8Array(addresses.length * 4);
for (var i = 0; i < addresses.length; i++) {
    var addr = addresses[i];
    binaryData[i * 4] = addr & 0xFF;
    binaryData[i * 4 + 1] = (addr >> 8) & 0xFF;
    binaryData[i * 4 + 2] = (addr >> 16) & 0xFF;
    binaryData[i * 4 + 3] = (addr >> 24) & 0xFF;
}
// Download binaryData as .bin file
```

### 2. Serving Addresses (PHP)
```php
// Save PHP decoder as addresses.php
// URL: https://yoursite.com/addresses.php?file=message.bin
```

### 3. Decoding (JavaScript)
```javascript
// Load addresses from server
$.ajax({
    url: 'addresses.php?file=message.bin',
    success: function(addresses) {
        // Decode message using ROM data
        var decoded = "";
        for (var i = 0; i < addresses.length; i++) {
            var addr = addresses[i];
            if (addr < romData.length) {
                decoded += String.fromCharCode(romData[addr]);
            }
        }
        console.log(decoded); // "Secret message!"
    }
});
```

## Performance Considerations

- **Memory Usage**: ZOSCII uses 2 bytes per character (16-bit addresses) vs 1 byte for ASCII
- **Setup Time**: Initial ROM scanning creates address lookup arrays  
- **Execution Speed**: up to 30% faster character output than traditional ASCII loops
- **ROM Requirements**: Sufficient byte value diversity needed for all required characters
- **Random Access Cost**: Each character lookup requires ROM memory access

### Performance Optimization Tips

**For Web Applications:**
```javascript
// Cache ROM analysis results
var cachedAddressArrays = null;
function initializeZOSCII(romData, memoryBlocks) {
    if (!cachedAddressArrays) {
        // Perform expensive ROM scanning once
        cachedAddressArrays = buildAddressArrays(romData, memoryBlocks);
    }
    return cachedAddressArrays;
}
```

**For 8-bit Systems:**
```z80
; Pre-load common characters into fast access tables
common_chars:   db "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!? "
common_addrs:   dw addr_A, addr_B, addr_C, ...  ; Pre-calculated addresses
```

## Security Properties

### Content Obfuscation
- **Address Sequences**: Messages appear as memory address lists
- **Random Selection**: Same message generates different encodings each time
- **ROM Dependency**: Decoding requires the specific ROM file used for encoding

### Security Limitations
- **Not Cryptographically Secure**: Addresses can be analyzed statistically
- **ROM Availability**: If ROM becomes public, all messages can be decoded
- **Pattern Analysis**: Frequency analysis may reveal common characters
- **No Authentication**: No built-in integrity checking

### Recommended Security Practices
```javascript
// Use multiple ROM sources for enhanced security
var multiRomBlocks = [
    {start: 0x0000, size: 0x4000, source: 'rom1.bin'},
    {start: 0x4000, size: 0x4000, source: 'rom2.bin'},
    {start: 0x8000, size: 0x4000, source: 'rom3.bin'}
];

// Add checksums for integrity verification
function addChecksum(addresses) {
    var checksum = 0;
    for (var i = 0; i < addresses.length; i++) {
        checksum ^= addresses[i];
    }
    addresses.push(checksum);
    return addresses;
}
```

## Error Handling

### Common Issues and Solutions

**Missing Character Bytes:**
```javascript
// Check for unmappable characters before encoding
function validateMessage(message, romData, memoryBlocks) {
    var missing = [];
    for (var i = 0; i < message.length; i++) {
        var byteValue = message.charCodeAt(i);
        if (!findByteInROM(byteValue, romData, memoryBlocks)) {
            missing.push({char: message[i], value: byteValue, position: i});
        }
    }
    return missing;
}
```

**Invalid Address Ranges:**
```javascript
// Validate memory blocks before processing
function validateMemoryBlocks(blocks, romSize) {
    for (var i = 0; i < blocks.length; i++) {
        var block = blocks[i];
        if (block.start < 0 || block.start + block.size > romSize) {
            throw new Error(`Invalid memory block: ${block.start}-${block.start + block.size}`);
        }
    }
}
```

**ROM Loading Errors:**
```javascript
// Handle ROM file loading failures gracefully
function loadROMFile(file) {
    return new Promise((resolve, reject) => {
        var reader = new FileReader();
        reader.onload = function(e) {
            try {
                var romData = new Uint8Array(e.target.result);
                if (romData.length === 0) {
                    throw new Error('Empty ROM file');
                }
                resolve(romData);
            } catch (error) {
                reject(new Error('Failed to parse ROM file: ' + error.message));
            }
        };
        reader.onerror = () => reject(new Error('Failed to read ROM file'));
        reader.readAsArrayBuffer(file);
    });
}
```

## Browser Compatibility

### JavaScript Requirements
- **ES5 Compatible**: Works in all modern browsers
- **Typed Arrays**: Requires Uint8Array support (IE10+)
- **File API**: For ROM file loading (IE10+)
- **AJAX**: For PHP integration (All browsers)

### Compatibility Table
| Feature | Chrome | Firefox | Safari | Edge | IE |
|---------|--------|---------|--------|------|----| 
| Core ZOSCII | ✓ | ✓ | ✓ | ✓ | 9+ |
| File Upload | ✓ | ✓ | ✓ | ✓ | 10+ |
| Drag & Drop | ✓ | ✓ | ✓ | ✓ | 10+ |

## Deployment Guidelines

### Web Server Setup
```apache
# .htaccess for Apache
RewriteEngine On
RewriteCond %{REQUEST_FILENAME} !-f
RewriteRule ^api/addresses/(.+)$ addresses.php?file=$1 [L,QSA]

# Enable CORS for cross-domain requests
Header set Access-Control-Allow-Origin "*"
Header set Access-Control-Allow-Methods "GET, POST, OPTIONS"
```

### Production Considerations
- **File Size Limits**: Configure PHP `upload_max_filesize` for large ROM files
- **Security**: Validate file uploads and sanitize filenames
- **Caching**: Implement ROM analysis result caching for performance
- **Rate Limiting**: Prevent abuse of encoding/decoding services

## License and Attribution

CyborgZOSCII is released under the **MIT License**.

```
Copyright (c) 2025 Julian

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality  
4. Submit a pull request with detailed description

### Areas for Contribution
- **Additional Character Sets**: Support for more encoding systems
- **Performance Optimizations**: Faster algorithms and caching strategies
- **Security Enhancements**: Improved obfuscation techniques
- **Platform Ports**: C, Python, Rust implementations
- **Documentation**: Examples, tutorials, use case studies
- **Web Tools**: Browser extensions for ZOSCII encoding/decoding
- **Mobile Apps**: iOS/Android implementations
- **IDE Plugins**: Integration with development environments

## Frequently Asked Questions

### General Questions

**Q: Is ZOSCII backward compatible with ASCII?**
A: No, ZOSCII uses a completely different addressing scheme. However, the library includes converters for ASCII, PETSCII, and EBCDIC input.

**Q: How much ROM space is needed for ZOSCII?**
A: Minimum 256 bytes to guarantee all byte values exist, but larger ROMs (64KB+) provide better randomization and security through more address options.

**Q: Can the same ROM be used multiple times?**
A: Yes, but using the same ROM repeatedly may create patterns. For security applications, consider using different ROMs or ROM sections.

**Q: What happens if a character can't be encoded?**
A: The system uses the `intUnmappableChar` parameter (typically 42 for '*') as a fallback character.

### Technical Questions

**Q: Why are addresses stored as little-endian in the binary format?**
A: Little-endian format is consistent with x86 architecture and many 8-bit systems, making integration easier.

**Q: Can ZOSCII handle Unicode characters?**
A: ZOSCII operates on byte values (0-255), so Unicode characters must be converted to a supported encoding first (UTF-8, etc.).

**Q: Is there a maximum message length?**
A: No hard limit, but practical limits depend on available memory and ROM address diversity.

**Q: How random is the address selection?**
A: Uses JavaScript's `Math.random()`, which is pseudorandom. If you requrie a cryptographic solution, this is not that, however it can be combined with cryptographically secure techniques.

### Performance Questions

**Q: How does ZOSCII compare to compression algorithms?**
A: ZOSCII typically increases data size (2x for addresses vs 1x for characters) but provides obfuscation benefits that compression doesn't offer.

**Q: Can ZOSCII be combined with compression?**
A: Yes, you can compress the address sequences after ZOSCII encoding, though this may reduce obfuscation benefits.

**Q: What's the optimal ROM size for performance?**
A: Larger ROMs provide more address options but slower initial scanning. 64KB-256KB typically offers good balance.

## Advanced Usage Examples

### Multi-ROM Encoding

```javascript
// Use different ROM sections for enhanced security
function multiRomEncode(message, romData) {
    var sections = [
        {start: 0x0000, size: 0x4000},  // Section 1
        {start: 0x4000, size: 0x4000},  // Section 2
        {start: 0x8000, size: 0x4000}   // Section 3
    ];
    
    var results = [];
    for (var i = 0; i < sections.length; i++) {
        var addresses = toZOSCII(romData, message, [sections[i]], null, 42);
        results.push({
            section: i,
            addresses: addresses,
            checksum: calculateChecksum(addresses)
        });
    }
    
    return results;
}
```

### Streaming Encoding

```javascript
// Encode large texts in chunks to manage memory
function streamingEncode(text, romData, memoryBlocks, chunkSize = 1000) {
    var results = [];
    
    for (var i = 0; i < text.length; i += chunkSize) {
        var chunk = text.substring(i, i + chunkSize);
        var addresses = toZOSCII(romData, chunk, memoryBlocks, null, 42);
        
        results.push({
            chunk: Math.floor(i / chunkSize),
            start: i,
            length: chunk.length,
            addresses: addresses
        });
    }
    
    return results;
}
```

### Custom Character Set Support

```javascript
// Add support for custom character encodings
function customToAscii(customChar, unmappableChar) {
    var customToAsciiMap = {
        // Define your custom character mappings
        0x80: 65,  // Custom char 0x80 -> 'A'
        0x81: 66,  // Custom char 0x81 -> 'B'
        // ... more mappings
    };
    
    return customToAsciiMap[customChar] || unmappableChar;
}

// Usage
var addresses = toZOSCII(romData, customText, memoryBlocks, customToAscii, 42);
```

### Error Recovery and Validation

```javascript
// Comprehensive validation and error recovery
function robustEncode(message, romData, memoryBlocks) {
    try {
        // Pre-validation
        validateInputs(message, romData, memoryBlocks);
        
        // Attempt encoding
        var addresses = toZOSCII(romData, message, memoryBlocks, null, 42);
        
        // Post-validation
        var decoded = decodeZOSCII(addresses, romData);
        if (decoded !== message) {
            throw new Error('Encoding validation failed');
        }
        
        return {
            success: true,
            addresses: addresses,
            stats: {
                originalLength: message.length,
                encodedLength: addresses.length,
                compressionRatio: (addresses.length * 2) / message.length
            }
        };
        
    } catch (error) {
        return {
            success: false,
            error: error.message,
            partialResult: null
        };
    }
}

function validateInputs(message, romData, memoryBlocks) {
    if (!message || message.length === 0) {
        throw new Error('Empty message');
    }
    
    if (!romData || romData.length === 0) {
        throw new Error('Empty ROM data');
    }
    
    if (!memoryBlocks || memoryBlocks.length === 0) {
        throw new Error('No memory blocks defined');
    }
    
    // Check if ROM contains necessary byte values
    var requiredBytes = new Set();
    for (var i = 0; i < message.length; i++) {
        requiredBytes.add(message.charCodeAt(i));
    }
    
    var availableBytes = new Set();
    for (var addr = 0; addr < romData.length; addr++) {
        if (isValidAddress(addr, memoryBlocks)) {
            availableBytes.add(romData[addr]);
        }
    }
    
    for (var byte of requiredBytes) {
        if (!availableBytes.has(byte)) {
            throw new Error(`Required byte value ${byte} not found in ROM`);
        }
    }
}
```

## Integration Examples

### Usage Examples

```bash
# Encode a message using ASCII
python zoscii_cli.py encode rom_file.bin "Hello, World!" -o message.bin

# Encode with EBCDIC conversion and verbose output
python zoscii_cli.py encode rom_file.bin "Hello, World!" -e ebcdic -v -o encoded.bin

# Decode a message
python zoscii_cli.py decode rom_file.bin message.bin -o decoded.txt

# Analyze ROM compatibility
python zoscii_cli.py info rom_file.bin

# Use custom memory blocks
python zoscii_cli.py encode rom_file.bin "Secret" -m memory_blocks.json
```

## Best Practices

### Security Considerations

1. **ROM Selection**: Use ROMs with high entropy and diverse byte values
2. **Key Management**: Treat ROM files as encryption keys - protect accordingly
3. **Address Randomization**: Ensure sufficient address options for each character
4. **Validation**: Always validate decoded messages when possible

### Performance Optimization

1. **Caching**: Cache ROM analysis results for repeated encoding operations
2. **Memory Blocks**: Use targeted memory blocks instead of entire ROM when possible
3. **Batch Processing**: Process multiple messages together to amortize setup costs
4. **Streaming**: For large texts, use streaming approaches to manage memory

### Error Handling

```javascript
// Robust error handling example
function safeZOSCIIEncode(romData, message, memoryBlocks) {
    try {
        // Validate inputs
        if (!romData || romData.length === 0) {
            throw new Error('Invalid ROM data');
        }
        
        if (!message || message.length === 0) {
            throw new Error('Empty message');
        }
        
        // Perform encoding with error recovery
        var addresses = toZOSCII(romData, message, memoryBlocks, null, 42);
        
        // Validate result
        if (addresses.length !== message.length) {
            console.warn('Address count mismatch - some characters may be missing');
        }
        
        return {
            success: true,
            addresses: addresses,
            warnings: []
        };
        
    } catch (error) {
        return {
            success: false,
            error: error.message,
            addresses: null
        };
    }
}
```

## Conclusion

CyborgZOSCII represents a novel approach to character encoding that trades traditional lookup table efficiency for direct memory addressing benefits. While not suitable for all applications, it excels in scenarios requiring:

- **Performance optimization** on resource-constrained systems
- **Content obfuscation** for security or steganographic applications  
- **Novel encoding approaches** for research and educational purposes

The system's open-source nature and comprehensive documentation make it accessible to developers while providing sufficient depth for advanced applications. Whether you're optimizing text output for an 8-bit demo, hiding messages in binary files, or exploring alternative encoding paradigms, CyborgZOSCII offers a unique solution that bridges historical computing constraints with modern security needs.

## Changelog

### Version 1.0.0 (2025-01-04)
- Initial release
- Core JavaScript implementation
- PHP decoder service
- Web-based encoder interface
- Character conversion functions (ASCII (default), PETSCII, EBCDIC)
- Documentation and examples

### Contributing Guidelines

We welcome contributions from the community! Here's how to get involved:

#### Code Contributions
1. **Fork** the repository
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Write tests** for new functionality
4. **Follow coding standards**: Use consistent naming and documentation
5. **Submit a pull request** with detailed description

#### Documentation Contributions
- **Examples**: Add real-world usage examples
- **Tutorials**: Create step-by-step guides
- **Translations**: Help translate documentation
- **API Documentation**: Improve function/parameter descriptions

#### Community Contributions
- **Use Case Studies**: Document interesting applications
- **Educational Content**: Create learning materials

### Code of Conduct

CyborgZOSCII is committed to fostering a welcoming and inclusive community. All participants are expected to:

- **Be nice**
- **Be lawful**

### Commercial Support

For commercial implementations, consulting, or custom development:

**Enterprise Licensing**: Contact me for commercial license options.

