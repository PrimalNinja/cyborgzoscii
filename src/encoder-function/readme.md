# ZOSCII Encoder Libraries

Cross-platform ZOSCII encoding libraries for C, Go, Rust, Python, JavaScript, and PHP.

## Installation

### C
```bash
# Copy zoscii-encoder.h and zoscii-encoder.c to your project
gcc -c zoscii-encoder.c -o zoscii-encoder.o
gcc your_program.c zoscii-encoder.o -o your_program
```

### Go
```bash
# Copy zoscii-encoder.go to your project or use as a module
go get github.com/yourusername/zoscii
```

### Rust
```bash
# Add to Cargo.toml:
# [dependencies]
# zoscii-encoder = "0.1"

# Or copy zoscii-encoder.rs and Cargo.toml to your project
```

### Python
```bash
# Copy zoscii-encoder.py to your project
# No external dependencies required
```

### JavaScript
```javascript
// Include zoscii-encode.js in your project
```

### PHP
```php
// Include zoscii-encode.php in your project
```

## Usage Examples

### C
```c
#include "zoscii-encoder.h"
#include <stdlib.h>

int main() {
    srand(time(NULL));
    
    unsigned char rom_data[1000];
    // ... load ROM data ...
    
    MemoryBlock blocks[] = {{0, 1000}};
    
    ZOSCIIResult result = toZOSCII(
        rom_data, 
        "Hello World", 
        blocks, 
        1, 
        NULL,  // No converter
        42     // Unmappable char
    );
    
    // Use result.addresses...
    
    freeZOSCIIResult(&result);
    return 0;
}
```

### Go
```go
package main

import (
    "https://github.com/PrimalNinja/cyborgzoscii"
)

func main() {
    romData := make([]byte, 1000)
    // ... load ROM data ...
    
    blocks := []zoscii.MemoryBlock{{Start: 0, Size: 1000}}
    
    result := zoscii.ToZOSCII(
        romData,
        "Hello World",
        blocks,
        nil,  // No converter
        42,   // Unmappable char
    )
    
    // Use result.Addresses...
}
```

### Rust
```rust
use zoscii::{to_zoscii, MemoryBlock};

fn main() {
    let rom_data = vec![0u8; 1000];
    // ... load ROM data ...
    
    let blocks = vec![MemoryBlock { start: 0, size: 1000 }];
    
    let result = to_zoscii(
        &rom_data,
        "Hello World",
        &blocks,
        None,  // No converter
        42,    // Unmappable char
    );
    
    // Use result.addresses...
}
```

### Python
```python
from zoscii-encoder import to_zoscii, MemoryBlock

rom_data = bytes([0] * 1000)
# ... load ROM data ...

blocks = [MemoryBlock(start=0, size=1000)]

result = to_zoscii(
    rom_data,
    "Hello World",
    blocks,
    converter=None,  # No converter
    unmappable_char=42
)

# Use result.addresses...
```

### JavaScript
```javascript
const rom = new Uint8Array(1000);
// ... load ROM data ...

const blocks = [{start: 0, size: 1000}];

const result = toZOSCII(
    rom,
    "Hello World",
    blocks,
    null,  // No converter
    42     // Unmappable char
);

// Use result.addresses...
```

### PHP
```php
<?php
require_once('zoscii-encode.php');

$romData = str_repeat("\x00", 1000);
// ... load ROM data ...

$blocks = [
    ['start' => 0, 'size' => 1000]
];

$addresses = toZOSCII(
    $romData,
    "Hello World",
    $blocks,
    null,  // No converter
    42     // Unmappable char
);

// Use $addresses...
?>
```

## Character Converters

All libraries include PETSCII and EBCDIC to ASCII converters:

```c
// C
result = toZOSCII(data, str, blocks, 1, petsciiToAscii, 42);
result = toZOSCII(data, str, blocks, 1, ebcdicToAscii, 42);

// Go
result := zoscii.ToZOSCII(data, str, blocks, zoscii.PetsciiToAscii, 42)

// Rust
result = to_zoscii(&data, str, &blocks, Some(petscii_to_ascii), 42);

// Python
result = to_zoscii(data, str, blocks, petscii_to_ascii, 42)
```

## API Reference

### Data Structures

**MemoryBlock** - Defines a valid region in ROM
- `start`: Starting address
- `size`: Size in bytes

**Result/ZOSCIIResult** - Encoding results
- `addresses`: Array of encoded addresses
- `input_counts`: Frequency of each input character
- `rom_counts`: Frequency of each byte in ROM

### Functions

**to_zoscii/ToZOSCII** - Main encoding function
- Parameters:
  - ROM/binary data
  - Input string
  - Memory blocks
  - Optional converter function
  - Unmappable character code
- Returns: Result structure with addresses and statistics

**petscii_to_ascii** - Convert PETSCII to ASCII
**ebcdic_to_ascii** - Convert EBCDIC to ASCII

## License

MIT License - (c) 2025 Cyborg Unicorn Pty Ltd