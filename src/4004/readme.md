# ZOSCII Encoder/Decoder for Intel 4004

**Status:** 🚧 **UNTESTED - Requires 4004 Expert Review**

A hardware-based cryptographic primitive implementation for the Intel 4004 microprocessor, providing one-time-pad-like security through ROM indirection.

## Overview

ZOSCII (Zero-sum One-time Stenographic Code for Integrated Indirection) is a cryptographic encoding scheme where:
- **Plaintext bytes** are encoded as **ROM addresses** containing those bytes
- **Multiple instances** of each byte exist in ROM at different addresses
- **Random instance selection** provides different encodings for the same plaintext
- **Decoding** requires physical access to the ROM chip (hardware indirection)

### Security Model

```
Plaintext:  "HELLO"
ROM:        [... H ... H ... E ... L ... L ... L ... O ...]
            addr: 0x42  0x91  0x15  0x03  0x7A  0xC5  0x88

Encoding:   H→0x91, E→0x15, L→0x7A, L→0x03, O→0x88
(Random instance selected each time)
```

**Adversary Model:** Without physical ROM access, determining `ROM[addr]` from `addr` alone is computationally infeasible (assumes ROM is not published/leaked).

## Files

- **`encoder.asm`** - Encodes plaintext bytes to ROM addresses using PRNG
- **`decoder.asm`** - Decodes ROM addresses back to plaintext via hardware indirection
- **`README.md`** - This file

## Known Issues & TODOs for 4004 Expert

### ⚠️ Critical Items Requiring Validation

1. **ISZ Instruction Semantics**
   - Code assumes: `ISZ Rn, label` skips to `label` if result is ZERO
   - 4004 manual states: "Increment and skip if zero"
   - **Needs verification:** Does it skip the NEXT instruction or jump to label?
   - **Impact:** All loop logic depends on this

2. **FIN Instruction Behavior**
   - Code assumes: `FIN P0` fetches 8-bit word from ROM into R0:R1
   - Format assumed: `R0 = high nibble`, `R1 = low nibble`
   - **Needs verification:** Is this correct for ROM data layout?
   - **Impact:** Character comparison may fail

3. **SRC Addressing Limitations**
   - Code uses 8-bit addresses (R2:R3 via `SRC P1`)
   - 4004 has 12-bit ROM space (4096 bytes)
   - **Current limitation:** Only addresses first 256 bytes
   - **TODO:** Implement bank switching for full 4K access

4. **Carry Propagation**
   - Manual carry handling implemented for all multi-nibble operations
   - **Needs verification:** Are all carry cases covered?
   - **Test case:** Pointer at 0xXF incrementing to 0xY0

5. **PRNG Quality**
   - 8-bit Galois LFSR with polynomial `x^8 + x^6 + x^5 + x^4 + 1`
   - Period: 255 (will repeat, but should be sufficient for short messages)
   - **TODO:** Verify tap configuration and behavior on 4004
   - **Enhancement:** Add external entropy source if available

### 🔧 Implementation Questions

6. **RAM Organization**
   - Code assumes:
     - `0x30-0x31`: PRNG state storage
     - `0x40-0x7F`: Input/output plaintext buffer
     - `0x80-0xFF`: Address buffer
   - **Needs verification:** Is this layout valid for target 4004 system?

7. **ROM Data Format**
   - Assumes ROM contains raw bytes where each character appears multiple times
   - **Needs specification:** How should ROM be populated?
   - **Question:** Should ROM be randomly shuffled or organized?

8. **Counter Initialization**
   - Code uses `FIM P6, 0x40` then `LD R12` to get counter value
   - Intent: Process 64 bytes
   - **Needs verification:** Does this work as intended with ISZ?

9. **JCN Flags**
   - Code uses: `JCN Z`, `JCN NZ`, `JCN C`, `JCN NC`
   - **Needs verification:** Are flag names correct for 4004 assembler syntax?
   - Some assemblers use: `JZ`, `JNZ`, `JC`, `JNC`

10. **Register Allocation Conflicts**
    - `FIN P0` overwrites R0:R1
    - `FIN` uses register pair as destination
    - **Needs review:** Ensure no register clobbers occur

### 🧪 Testing Requirements

**Before hardware deployment:**

1. **Assembler Compatibility**
   - Test with actual 4004 assembler (syntax may vary)
   - Verify all mnemonics are correct
   - Check immediate value formats

2. **Logic Verification**
   - Single-step through encoder with known input
   - Verify PRNG generates different values
   - Confirm search finds correct instance
   - Check address storage is correct

3. **Decoder Validation**
   - Feed encoder output directly to decoder
   - Verify plaintext matches original
   - Test with boundary cases (0x00, 0xFF)

4. **Edge Cases**
   - Message length = 1, 64, 128
   - Characters appearing 1x, 10x, 100x in ROM
   - Pointer wraparound scenarios
   - PRNG state after 255 cycles

### 📝 Documentation Gaps

- **Missing:** ROM generation script/procedure
- **Missing:** Example input/output traces
- **Missing:** Cycle count estimates
- **Missing:** Memory footprint calculation
- **Missing:** Error handling specification

## Architecture Notes

### Intel 4004 Constraints
- **4-bit ALU**: All operations are nibble-based
- **12-bit addressing**: ROM is 0x000-0xFFF (4096 bytes)
- **Register pairs**: P0-P7 = (R0:R1, R2:R3, ..., R14:R15)
- **No interrupts**: Purely sequential execution
- **No stack**: `BBL` returns with value in accumulator

### Instruction Peculiarities
```assembly
LDM n      ; Load immediate (4-bit only: 0-15)
FIM Pn, nn ; Load 8-bit immediate into register pair
ISZ Rn, L  ; Increment Rn, skip if zero (?)
JCN cc, L  ; Jump conditional
SRC Pn     ; Send register pair to address bus
RDM        ; Read RAM at selected address
WRM        ; Write RAM at selected address
FIN Pn     ; Fetch indirect (ROM[P1] → Pn)
BBL n      ; Branch back and load (return with value)
```

## Usage (Theoretical)

### Encoding
```
Input:  RAM 0x40-0x7F contains plaintext
Output: RAM 0x80-0xFF contains address pairs
```

### Decoding
```
Input:  RAM 0x80-0xFF contains address pairs
Output: RAM 0x40-0x7F contains plaintext
```

### Call Convention
```assembly
JMS ENCODE_SETUP
; Check accumulator on BBL return (0 = success)

JMS DECODE_SETUP  
; Check accumulator on BBL return (0 = success)
```

## Contributing

**Priority:** Need 4004 expert to:
1. Verify instruction semantics (especially ISZ, FIN)
2. Test on simulator or real hardware
3. Optimize for code size and cycle count
4. Implement full 12-bit addressing
5. Add error handling

**Contributions welcome for:**
- ROM generation utilities
- Test vectors and validation suite
- Cycle-accurate simulator traces
- Hardware implementation notes

## License

[Specify license - suggest MIT or public domain for educational code]

## References

- Intel 4004 Assembly Language Programming Manual
- MCS-4 Micro Computer Set Users Manual
- [Add relevant papers on hardware-based cryptography]

## Contact

[Your contact info or GitHub handle]

---

**⚠️ DISCLAIMER:** This code is UNTESTED and may contain critical bugs. Do not use in production without thorough validation by a 4004 expert. The security model assumes ROM secrecy - if ROM contents are leaked, all security is lost.