# CyborgZOSCII v20250805

ZOSCII Zero Overhead Secure Coding Information Interchange.

An innovative character encoding system that eliminates lookup table overhead while providing built-in security properties through direct memory addressing.

## Overview

CyborgZOSCII is an alternative to ASCII/PETSCII that uses direct ROM addressing instead of traditional character-to-value mapping. This approach provides significant advantages for resource-constrained systems while offering unique security properties.

## Key Benefits

### Performance & Memory
1. **Zero ROM overhead** - No lookup tables required (ASCII/PETSCII tables eliminated)
2. **Faster execution** - Direct addressing eliminates base+offset calculations and even open to stack abuse for fast text printing
3. **Perfect for 8-bit systems** - Optimized for systems where every CPU cycle matters

### Security Properties
3. **Zero cryptography, maximum security** - 100% secure when reference ROM unknown
4. **Fault tolerance** - ROM corruption causes typos, not total failure
5. **No traditional cipher vulnerabilities** - Uses addressing, not mathematical transformations

## Technical Approach

- Uses 16-bit addresses per character (vs 8-bit character codes)
- Characters reference direct ROM memory locations
- Same message generates different encodings each time (random address selection)
- No mathematical operations required for decoding

## Use Cases

- Visit https://cyborgunicorn.com.au/zoscii/zosciibb for a realworld demonstration

- **Embedded systems** with severe ROM constraints
- **IoT devices** requiring lightweight secure messaging  
- **Retro computing** (Z80, 6502, etc.) applications
- **Corporate communications** requiring server-side content protection
- **Invite-only websites** with content invisible to hosting providers and unwanted users

- Julian


