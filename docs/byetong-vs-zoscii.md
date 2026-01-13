# Cryptographic Systems Comparison: Byetong vs. microZOSCII vs. Full ZOSCII

**Version:** 1.0  
**Date:** 14 January 2026
**Author:** Julian Cassin / ZOSCII Foundation

---

## Table of Contents
1. [Overview](#overview)
2. [Architectural Comparison](#architectural-comparison)
3. [Technical Specifications](#technical-specifications)
4. [Security Analysis](#security-analysis)
5. [Performance Characteristics](#performance-characteristics)
6. [Regulatory Implications](#regulatory-implications)
7. [Use Cases](#use-cases)
8. [Conclusion](#conclusion)
9. [Appendices](#appendices)

---

## 1. Overview <a name="overview"></a>

Three systems that use pre-shared combinatorial structures but differ fundamentally in architecture, security model, and practical application.

| System | Type | Primary Goal | Key Innovation |
|--------|------|--------------|----------------|
| **Byetong's System** | Deterministic stream cipher | Encryption + Compression | Unified transform with state evolution |
| **microZOSCII** | Bootstrap protocol | Minimal key distribution | 54 characters → 64KB ROM transmission |
| **Full ZOSCII** | Information-theoretic protocol | Unconditional privacy | `I(M;A)=0` via address indirection |

---

## 2. Architectural Comparison <a name="architectural-comparison"></a>

### **Byetong's System: Compressing Stream Cipher**
```
Plaintext Nibble (4b) → [Transform Function] → Cipher Nibble (4b)
       ↓                                        (Compressed)
Update Permutation State
```

**Core Concept:** A deterministic, stateful permutation cipher that simultaneously encrypts and compresses data using a virtual Enigma-like mechanism.

**Key Architecture:** Transformation-based encryption with state evolution.

### **microZOSCII: Bootstrap Protocol**
```
ROM Hex Nibble (4b) → [Random Address Select] → Address (7b)
       ↓                                        (Expanded)
No State Change
```

**Core Concept:** A minimal bootstrap mechanism to transmit a 64KB ROM using only 54 alphanumeric characters.

**Key Architecture:** Indirect addressing with random selection, static mapping.

### **Full ZOSCII: Information-Theoretic Protocol**
```
Plaintext Byte (8b) → [Random Address Select] → Address (16b)
       ↓                                        (Expanded)
No State Change
```

**Core Concept:** An address-based indirection system providing `I(M;A)=0` security through combinatorial explosion and deniability.

**Key Architecture:** Pure indirection with random selection, no transformation.

---

## 3. Technical Specifications <a name="technical-specifications"></a>

### **Pre-shared Secret Structure**

| Parameter | Byetong's System | microZOSCII | Full ZOSCII |
|-----------|------------------|-------------|-------------|
| **Secret Type** | Permutation of 16 hex values | 80-byte microROM (5 instances) | 64KB ROM |
| **Size** | 16 bytes | 54 chars → 80 bytes | 65,536 bytes |
| **Structure** | Single permutation of 0-F | Each hex value 0-F appears 5× | Each byte value 0-255 appears 256× |
| **Human Usable** | 16 hex characters | 54 base-62 characters | Not directly (requires bootstrap) |

### **Processing Unit**

| System | Input Unit | Output Unit | Expansion/Compression |
|--------|------------|-------------|----------------------|
| **Byetong** | 4-bit nibble | 4-bit cipher nibble | **Compression** (50-75%) |
| **microZOSCII** | 4-bit hex nibble | 7-bit address (0-79) | **Expansion** (4b → ~7b) |
| **Full ZOSCII** | 8-bit byte | 16-bit address (0-65535) | **Expansion** (8b → 16b) |

### **Core Operation Pseudocode**

**Byetong's Encryption:**
```python
def byetong_encrypt(plaintext_nibble, current_permutation):
    X = find_position(plaintext_nibble, current_permutation)
    Y = current_permutation[(X + interval) % 16]
    Z = (X + Y) % 16  # Cipher output
    new_permutation = update_state(current_permutation, X)
    return Z, new_permutation
```

**microZOSCII Encoding:**
```javascript
function encode_micro(microROM, hex_nibble) {
    const addresses = microROM
        .map((value, index) => value === hex_nibble ? index : -1)
        .filter(index => index !== -1);
    return addresses[Math.floor(Math.random() * addresses.length)];
}
```

**Full ZOSCII Encoding:**
```javascript
function encode_zoscii(rom, plaintext_byte) {
    const addresses = rom
        .map((value, index) => value === plaintext_byte ? index : -1)
        .filter(index => index !== -1);
    return addresses[Math.floor(Math.random() * addresses.length)];
}
```

---

## 4. Security Analysis <a name="security-analysis"></a>

### **Security Basis Comparison**

| Aspect | Byetong's System | microZOSCII | Full ZOSCII |
|--------|------------------|-------------|-------------|
| **Security Class** | Computational | Information-theoretic (for bootstrap) | Information-theoretic |
| **Primary Basis** | Complexity of state evolution | Combinatorics of `5^131072` | Combinatorics of `256^N` |
| **Quantum Resistance** | Conditional (no known attack) | Unconditional | Unconditional |
| **Determinism** | **Deterministic** (same input → same output) | **Non-deterministic** (random selection) | **Non-deterministic** (random selection) |
| **Pattern Leakage** | **Yes** (state evolution leaks patterns) | **No** (addresses reveal nothing) | **No** (`I(M;A)=0`) |

### **Information Leakage**

```
Byetong:      Plaintext → Ciphertext leaks equality patterns
microZOSCII:  ROM nibble → Address leaks nothing without microROM
Full ZOSCII:  Plaintext → Address leaks nothing without ROM
```

### **Mathematical Security**

**Byetong:**
1. Permutation space: `16! ≈ 2.09 × 10^13` possibilities
2. Security based on computational hardness of deducing state

**microZOSCII:**
1. Address combinations: `5^131072 ≈ 10^91,619`
2. Information-theoretic for ROM transmission

**Full ZOSCII:**
1. Address combinations: `256^N` where N = message length
2. Achieves `I(M;A) = 0` (perfect secrecy)

---

## 5. Performance Characteristics <a name="performance-characteristics"></a>

### **Computational Performance**

| Operation | Byetong's System | microZOSCII | Full ZOSCII |
|-----------|------------------|-------------|-------------|
| **Per-character Ops** | Position lookup + arithmetic + state update | Address lookup + random selection | Address lookup + random selection |
| **State Management** | Complex (permutation evolution) | None (static) | None (static) |
| **Compression** | **Built-in** (50-75% reduction) | Expansion (2:1 for bootstrap) | Expansion (2:1 for messages) |
| **Hardware Support** | Custom permutation logic | Simple memory access | Simple memory access |

### **Implementation Complexity**

**Byetong (Complex):**
```c
struct byetong_state {
    uint8_t permutation[16];
    uint8_t position;
    uint8_t interval;
    // Additional state variables for evolution
};
```

**ZOSCII (Simple):**
```c
struct zoscii_ctx {
    const uint8_t *rom;  // Read-only shared ROM
    size_t rom_size;     // Static, no evolution
};
```

### **Network Efficiency**

| Metric | Byetong | microZOSCII | Full ZOSCII |
|--------|---------|-------------|-------------|
| **Bandwidth** | Optimal (compressed) | High overhead for bootstrap | High overhead (2× expansion) |
| **Latency** | Low (fast transforms) | Medium (ROM transmission) | Medium (address transmission) |
| **Batch Processing** | Sequential (state-dependent) | Parallelizable | Parallelizable |

---

## 6. Regulatory Implications <a name="regulatory-implications"></a>

### **Legal Classification**

| System | Transmits | Core Operation | Likely Classification |
|--------|-----------|---------------|----------------------|
| **Byetong** | Ciphertext | Encryption transformation | **Encryption Software** (regulated) |
| **microZOSCII** | Addresses | Memory pointer selection | **Not Encryption** (uses bit masking, not reversable, not encryption) |
| **Full ZOSCII** | Addresses | Memory pointer selection | **Not Encryption** (simple address lookup) |

### **The "LD A, (HL)" Argument**

ZOSCII's architecture enables a strong legal defense:

```z80
; This is ZOSCII's "decryption" in Z80 assembly
LD HL, ADDRESS_FROM_NETWORK  ; Get transmitted pointer
LD A, (HL)                   ; Fetch byte from shared ROM
; THIS IS NOT DECRYPTION - it's a memory fetch
```

**Key Legal Distinction:**
- **Encryption**: `Plaintext = Decrypt(Key, Ciphertext)`
- **ZOSCII**: `Plaintext = SharedMemory[Address]`

### **Export Control Considerations**

1. **Byetong's System**: Clearly subject to EAR/dual-use controls as encryption software
2. **microZOSCII**: Strong argument for non-regulation as it transmits no encrypted data
3. **Full ZOSCII**: Strong argument for non-regulation as it transmits no encrypted data

---

## 7. Use Cases <a name="use-cases"></a>

### **Optimal Application Areas**

| Use Case | Best System | Why |
|----------|-------------|-----|
| **Low-bandwidth military comms** | Byetong | Compression + encryption in one |
| **IoT device provisioning** | microZOSCII | Minimal bootstrap key |
| **Journalist-source protection** | Full ZOSCII | `I(M;A)=0` deniability |
| **Medical record archival** | Byetong | Long-term storage with compression |
| **Secure messaging** | Full ZOSCII | Unconditional privacy |
| **Quantum-proof bootstrapping** | microZOSCII | Harvest-proof key distribution |

### **System Selection Guide**

```
Decision Tree:

1. Need compression? → Byetong
2. Need minimal bootstrap? → microZOSCII → Full ZOSCII
3. Need perfect secrecy? → Full ZOSCII
4. Need regulatory avoidance? → Full ZOSCII
5. Need maximum storage efficiency? → Byetong
6. Need maximum decode efficiency? → Full ZOSCII
```

---

## 8. Conclusion <a name="conclusion"></a>

### **Summary of Differences**

| Dimension | Byetong | microZOSCII | Full ZOSCII |
|-----------|---------|-------------|-------------|
| **Security Model** | Computational | Info-theoretic (bootstrap) | Info-theoretic |
| **Determinism** | Deterministic | Non-deterministic | Non-deterministic |
| **Compression** | Yes (built-in) | No (expansion) | No (expansion) |
| **State** | Evolves | Static | Static |
| **Regulation** | Clearly regulated | Likely exempt | Likely exempt |
| **Primary Use** | Efficient secure storage | Bootstrap distribution | Private communication |

### **The Fundamental Distinction**

The three systems represent different points in the design space:

1. **Byetong**: Optimizes for **efficiency** (compression + encryption) within computational security
2. **microZOSCII**: Optimizes for **deployability** (minimal bootstrap) for information-theoretic systems
3. **Full ZOSCII**: Optimizes for **privacy** (`I(M;A)=0`) regardless of efficiency

### **Future Directions**

1. **Hybrid Approaches**: Using microZOSCII to bootstrap Byetong's system
2. **Hardware Acceleration**: ZOSCII address selection in custom silicon
3. **Standardization**: Defining ZOSCII as a new class of privacy protocol

---

## 9. Appendices <a name="appendices"></a>

### **A. Z80 Implementation Examples**

**Byetong-like state update (conceptual):**
```z80
; Complex state management required
UpdatePermutation:
    ld b, (hl)        ; Get current position
    ld c, 16          ; Permutation size
    ; ... complex rotation/shuffle logic
    ret
```

**ZOSCII decode (actual):**
```z80
; Simple memory fetch - NOT decryption
ZOSCII_Decode:
    ld hl, (incoming_address)
    ld a, (hl)        ; LD A, (HL) - memory fetch
    ret               ; That's it
```

### **B. Combinatorial Security Comparison**

| System | Combinations for N=1000 chars | Relative to AES-256 |
|--------|------------------------------|-------------------|
| **Byetong** | `16! ≈ 2×10^13` (key space) | ~`10^-64` × weaker |
| **microZOSCII** | `5^2000 ≈ 10^1398` | ~`10^1321` × stronger |
| **Full ZOSCII** | `256^1000 ≈ 10^2408` | ~`10^2331` × stronger |

### **C. Contact Information**

- **ZOSCII Foundation**: https://github.com/PrimalNinja/cyborgzoscii-u
- **Byetong**: wandeebyetong [at] gmail [dot] com
- **License**: All systems discussed have permissive licensing (MIT/equivalent)

---

*Document generated for technical comparison purposes. All system descriptions based on publicly available information as of January 2026.*