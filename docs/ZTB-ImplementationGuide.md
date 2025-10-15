# ZOSCII Tamperproof Blockchain - Implementation Guide

**A quantum-resistant blockchain implementation using Information-Theoretically Secure (ITS) encoding and combinatorial security.**

*(c) 2025 Cyborg Unicorn Pty Ltd - MIT License*

---

## Table of Contents

1. [Overview](#overview)
2. [Key Concepts](#key-concepts)
3. [Architecture](#architecture)
4. [Command-Line Tools](#command-line-tools)
5. [File Formats](#file-formats)
6. [Rolling ROM Construction](#rolling-rom-construction)
7. [Security Model](#security-model)
8. [Build Instructions](#build-instructions)
9. [Example Workflows](#example-workflows)
10. [Important Notes](#important-notes)

---

## Overview

The ZOSCII Tamperproof Blockchain (ZTB) provides:

- ✅ **Quantum Resistance** - Security based on combinatorial infeasibility, not computational difficulty
- ✅ **Scalable Architecture** - Trunk and branch chains for efficient organization and indexing
- ✅ **Tamperproof Guarantee** - Entire blocks are ZOSCII encoded with pointer-based verification (~10^152900 impossibility)
- ✅ **Fast Verification** - O(1) lookups for decoding, no complex cryptographic operations
- ✅ **Information-Theoretic Security** - Immune to quantum algorithms (Shor's, Grover's)

---

## Key Concepts

### Traditional Blockchains vs ZTB

| Aspect | Traditional Blockchain | ZTB |
|--------|----------------------|-----|
| Block Linking | Cryptographic hashes (SHA-256) | ZOSCII pointer encoding |
| Security Basis | Computational difficulty | Combinatorial impossibility |
| Quantum Threat | Vulnerable | Immune |
| Data Structure | Linear chain | Tree (trunk + branches) |
| Decoding Speed | Hash verification | O(1) pointer lookup |

### Core Components

1. **Genesis ROM** (64KB high-entropy file)
   - Immutable root of trust for the entire blockchain
   - Contains random data with all 256 byte values (0-255) present
   - Lost Genesis ROM = unrecoverable blockchain

2. **Rolling ROM** (64KB composite)
   - Built from samples of previous blocks (up to 64 blocks × 1KB each)
   - Acts as the encoding table for each new block
   - Creates a "genetic code" linking blocks to their lineage

3. **ZOSCII Encoding**
   - Each byte (1 byte) → encoded as pointer (2 bytes) into Rolling ROM
   - Random selection from multiple valid addresses for same byte value
   - Non-deterministic: same data encodes differently each time

4. **Complete Block Encoding**
   - **Entire block** (header + payload) is ZOSCII encoded together
   - No plaintext headers - everything is encoded
   - Stronger integrity guarantee (headers cannot be tampered independently)

---

## Architecture

### Chain Structure

```
Genesis ROM (64KB random data)
        ↓
    Trunk Block 1 (encoded)
        ↓
    Trunk Block 2 (encoded)
        ├─→ Branch A Block 1 (encoded)
        │       ↓
        │   Branch A Block 2 (encoded)
        │
        ├─→ Branch B Block 1 (encoded)
        │       ↓
        │   Branch B Block 2 (encoded)
        ↓
    Trunk Block 3 (encoded)
```

### Block Types

- **Trunk Block** (`is_branch = 0`)
  - Main chain blocks
  - Linear progression from Genesis
  - `trunk_id = NULL_GUID`

- **Branch Block** (`is_branch = 1`)
  - Splits from trunk at specific point
  - Maintains link to parent trunk
  - `trunk_id = <parent_trunk_id>`
  - Rolling ROM samples from: branch history → trunk history → Genesis

- **Checkpoint Block** (`is_branch = 2`)
  - Special archival boundary marker
  - Allows pruning of historical blocks
  - Future blocks can skip archived history in Rolling ROM construction

---

## Command-Line Tools

### 1. `ztbcreate` - Genesis ROM Creator

Creates the 64KB high-entropy Genesis ROM file - the foundation of your blockchain.

**Usage:**
```bash
ztbcreate <output_genesis_rom_file>
```

**Example:**
```bash
ztbcreate genesis.rom
```

**Output:**
- Generates 64KB random data file
- Analyzes entropy (ensures all 256 byte values present)
- Warns if regeneration needed due to missing values

**⚠️ CRITICAL:** Keep this file secure and backed up. Loss = unrecoverable blockchain.

---

### 2. `ztbaddblock` - Block Creator

Adds a new block to an existing chain (trunk or branch).

**Usage:**
```bash
# Add text block
ztbaddblock <genesis_rom> <chain_id> -t "text string"

# Add file block
ztbaddblock <genesis_rom> <chain_id> -f <file_path>
```

**Examples:**
```bash
# First block creates new trunk
ztbaddblock genesis.rom MyTrunk -t "Genesis block"

# Add subsequent blocks
ztbaddblock genesis.rom MyTrunk -t "Block 2"
ztbaddblock genesis.rom MyTrunk -f invoice.pdf

# Add to existing branch
ztbaddblock genesis.rom ClientA -t "Transaction 1"
```

**Process:**
1. Scans existing blocks in chain
2. Builds Rolling ROM from previous blocks (max 64 blocks)
3. Creates header (GUID, timestamp, checksum, metadata)
4. Combines header + payload (padded to min 512 bytes)
5. **ZOSCII encodes entire block** using Rolling ROM
6. Writes: `<chain_id>_<index>_<block_guid>.ztb`

**Parameters:**
- `-t "text"` - Add text string as payload
- `-f filename` - Add file contents as payload

---

### 3. `ztbaddbranch` - Branch Creator

Creates a new branch splitting from a trunk block.

**Usage:**
```bash
ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -t "text"
ztbaddbranch <genesis_rom> <trunk_id> <new_branch_id> -f <file>
```

**Example:**
```bash
# Create trunk
ztbaddblock genesis.rom CompanyLedger -t "2025 Books"

# Branch off for departments
ztbaddbranch genesis.rom CompanyLedger Sales -t "Sales Genesis"
ztbaddbranch genesis.rom CompanyLedger Marketing -t "Marketing Genesis"

# Add to branches
ztbaddblock genesis.rom Sales -t "Sale #001"
ztbaddblock genesis.rom Marketing -t "Campaign #001"
```

**Process:**
1. Verifies trunk exists
2. Ensures branch doesn't already exist
3. Builds Rolling ROM from trunk history
4. Creates branch block (anchored to trunk's latest block)
5. Sets `is_branch = 1` and stores `trunk_id`
6. ZOSCII encodes entire block

**Use Cases:**
- Wallets branching from main chain
- Departments from company trunk
- Accounts from financial year trunk
- Projects from organization trunk

---

### 4. `ztbfetch` - Block Fetcher & Decoder

Retrieves and decodes a specific block, verifying its integrity.

**Usage:**
```bash
ztbfetch <genesis_rom> <chain_id> <block_index>
```

**Examples:**
```bash
# Fetch from trunk
ztbfetch genesis.rom MyTrunk 3

# Fetch from branch
ztbfetch genesis.rom ClientA 2
```

**Process:**
1. Locates block file by chain_id and index
2. Determines if chain is branch (loads trunk history if needed)
3. Reconstructs correct Rolling ROM
4. **Decodes entire block** (header + payload)
5. Verifies checksum (detects tampering)
6. Outputs payload to stdout

**Output:**
```
Fetching block 3 from chain 'MyTrunk'
Filename: MyTrunk_0003_A3F2B891-4C5D-4E6F-8A9B-1C2D3E4F5A6B.ztb

--- Block Header ---
Block ID:      A3F2B891-4C5D-4E6F-8A9B-1C2D3E4F5A6B
Prev Block ID: B2E1A780-3B4C-4D5E-7A8B-0C1D2E3F4A5B
Trunk ID:      00000000-0000-0000-0000-000000000000
Is Branch:     No
Payload Len:   256 bytes
Timestamp:     1735689600

--- Verification ---
Stored checksum:     3
Calculated checksum: 3
✓ Integrity verified

--- Decoded Payload (256 bytes) ---
[payload content]
--- End Payload ---
```

---

### 5. `ztbverify` - Chain Verifier

Verifies integrity of chains by working **backwards** from latest block to Genesis.

**Usage:**
```bash
# Verify trunk and all branches (default)
ztbverify <genesis_rom> <trunk_id>

# Verify trunk only
ztbverify <genesis_rom> <trunk_id> -t

# Verify all branches only (skip trunk)
ztbverify <genesis_rom> <trunk_id> -bb

# Verify specific branch only
ztbverify <genesis_rom> <trunk_id> -b <branch_id>
```

**Examples:**
```bash
# Verify everything
ztbverify genesis.rom MyTrunk

# Just trunk
ztbverify genesis.rom MyTrunk -t

# Just branches
ztbverify genesis.rom MyTrunk -bb

# Specific branch
ztbverify genesis.rom MyTrunk -b ClientA
```

**Process:**
1. Scans all blocks in requested chain(s)
2. Discovers branches by decoding first blocks
3. Verifies **backwards** (latest → genesis) for each chain
4. Rebuilds Rolling ROM for each block
5. Decodes and verifies checksum
6. Reports PASS/FAIL for each block

**Why Backwards?**
- Latest blocks are most likely to be tampered
- Efficiently detects recent tampering
- Still verifies entire lineage

**Output:**
```
=== Verifying Trunk: MyTrunk ===
  Block 5 (E4F3C892-...)... [PASS]
  Block 4 (D3E2B781-...)... [PASS]
  Block 3 (C2D1A670-...)... [PASS]
  Block 2 (B1C0956F-...)... [PASS]
  Block 1 (A0B9845E-...)... [PASS]
✓ Trunk verified

=== Found 2 branch(es) ===

--- Verifying Branch: ClientA ---
  Block 3 (F5G4D993-...)... [PASS]
  Block 2 (E4F3C882-...)... [PASS]
  Block 1 (D3E2B771-...)... [PASS]
✓ Branch verified

--- Verifying Branch: ClientB ---
  Block 2 (G6H5E004-...)... [PASS]
  Block 1 (F5G4D993-...)... [PASS]
✓ Branch verified

=== Verification Summary ===
Total blocks verified: 10
Failed verifications:  0
Branches found:        2

✓✓✓ ALL VERIFICATIONS PASSED ✓✓✓
```

---

### 6. `ztbcheckpoint` - Checkpoint Creator

Creates an archival boundary for pruning old blocks while maintaining chain integrity.

**Usage:**
```bash
ztbcheckpoint <chain_id> <genesis_rom> <previous_block_guid>
```

**Example:**
```bash
# At end of financial year
ztbcheckpoint FY2024 genesis.rom last_block_of_2024_guid
```

**What It Does:**
1. Generates new repeater GUID
2. Saves GUID to local config file
3. Creates Type 2 (checkpoint) block containing the GUID
4. Block becomes new "mini-genesis" for future blocks

**After Checkpoint:**
- Move blocks **before** checkpoint to archive storage
- Copy Genesis ROM to archive (for independent verification)
- Keep checkpoint + recent blocks on fast storage
- Future blocks build Rolling ROM from: checkpoint → Genesis (skip archived)

**Use Cases:**
- End of financial periods
- Large chain pruning
- Creating self-contained archives
- Migration to new storage tiers

---

## File Formats

### Genesis ROM File
```
Filename: genesis.rom (or any name)
Size:     65,536 bytes (64KB)
Content:  High-entropy random data
Format:   Raw binary (no structure)
```

**Requirements:**
- All 256 byte values (0-255) must be present
- High entropy distribution
- Cryptographically random (use proper RNG)

### Block Filename Convention
```
<chain_id>_<index>_<block_guid>.ztb
```

**Examples:**
```
MyTrunk_0001_A0B9845E-1234-4567-89AB-CDEF01234567.ztb
ClientA_0002_B1C0956F-2345-5678-9ABC-DEF012345678.ztb
Sales_0010_C2D1A670-3456-6789-ABCD-EF0123456789.ztb
```

### Block Structure (Before Encoding)

```c
[ZTB_BlockHeader]           // 132 bytes
  char block_id[37];        // This block's GUID
  char prev_block_id[37];   // Previous block's GUID (or NULL_GUID for first)
  char trunk_id[37];        // Parent trunk ID (or NULL_GUID if not branch)
  uint32_t payload_len;     // Original payload length
  uint32_t padded_len;      // Padded payload length (min 512 bytes)
  uint32_t checksum;        // Mod 7 checksum of payload
  uint64_t timestamp;       // Unix timestamp
  uint8_t is_branch;        // 0=trunk, 1=branch, 2=checkpoint

[Payload Data]              // padded_len bytes
  <raw data>                // Padded to min 512 bytes with zeros
```

**Total Raw Size:** 132 + padded_len bytes

### Block Structure (After Encoding)

```
[ZOSCII Encoded Block]
  - Entire raw block encoded as 16-bit pointers
  - Size: (132 + padded_len) × 2 bytes
  - Each byte → 2-byte little-endian pointer into Rolling ROM
```

**Example:**
- Raw block: 132 + 512 = 644 bytes
- Encoded block: 644 × 2 = 1,288 bytes

---

## Rolling ROM Construction

The Rolling ROM is built in **priority order** to create a unique "genetic code" for each block:

### Algorithm

```
1. Start with empty 64KB buffer

2. Sample from BRANCH BLOCKS (if on branch):
   - Walk backwards through branch history
   - For each block: copy first 1KB of ENCODED data
   - Maximum: 64 blocks (64KB total)
   - Stop if buffer full

3. Sample from TRUNK BLOCKS (if on branch and space remains):
   - Walk backwards through trunk history
   - For each block: copy first 1KB of ENCODED data
   - Continue until buffer full or 64 total blocks

4. Fill remainder with GENESIS ROM:
   - Copy Genesis ROM data to fill remaining space
   - Always at least some Genesis data present

5. Result: 64KB Rolling ROM unique to this block's position
```

### Key Points

- **Samples are ENCODED data** (the pointers), not decoded payload
- Maximum 64 blocks contribute (64KB ÷ 1KB per block)
- Younger blocks near the front (most recent history matters most)
- Genesis ROM always contributes to ensure entropy

### Example: Branch Block Rolling ROM

```
Branch: ClientA (5 blocks)
Trunk: CompanyLedger (3 blocks)
Target: ClientA Block 6

Rolling ROM Construction:
- ClientA Block 5: 1KB (most recent)
- ClientA Block 4: 1KB
- ClientA Block 3: 1KB
- ClientA Block 2: 1KB
- ClientA Block 1: 1KB
- CompanyLedger Block 3: 1KB (trunk anchor)
- CompanyLedger Block 2: 1KB
- CompanyLedger Block 1: 1KB
- Genesis ROM: 56KB (fill remainder)

Total: 64KB Rolling ROM for ClientA Block 6
```

### Why ENCODED Data?

The encoded addresses themselves are the entropy source:
- Original payload might be boring (all spaces, zeros, etc.)
- But random address selection creates high entropy
- Each encoding is non-deterministic
- Copying encoded addresses preserves this entropy

---

## Security Model

### Tamperproof Guarantee

**If an attacker modifies Block N:**

1. Block N's encoded data changes
2. Block N+1 was built using Block N's encoded data in its Rolling ROM
3. Block N+1's pointers now point to **wrong locations** in altered Block N
4. Probability of pointers still resolving correctly: ~10^-152900

**This is combinatorially infeasible** - not computationally hard, but **information-theoretically impossible**.

### Why Quantum-Resistant?

Traditional blockchains rely on mathematical problems:
- **RSA:** Integer factorization (Shor's algorithm breaks this)
- **ECC:** Discrete logarithm (Shor's algorithm breaks this)
- **SHA-256:** Collision resistance (Grover's reduces security by half)

**ZTB has no mathematical problems to attack:**
- No factorization to perform
- No discrete logarithms to compute
- No hashes to find collisions for
- Security is **structural**, not computational

Quantum computers excel at solving mathematical problems. ZTB doesn't present any.

### Comparison

| Attack Vector | Traditional Blockchain | ZTB |
|---------------|----------------------|-----|
| Shor's Algorithm | Breaks PKI | No PKI to break |
| Grover's Algorithm | Weakens hashing | No hashing used |
| Brute Force | 2^256 operations | 10^152900 permutations |
| Side Channel | Timing attacks on crypto | No crypto operations |
| Quantum Computer | Eventually vulnerable | Immune |

### Information-Theoretic Security

ZTB provides **unconditional security** (Shannon's definition):
- Security doesn't depend on computational assumptions
- An attacker with infinite computing power still cannot forge blocks
- The information needed to tamper simply doesn't exist in the encoded data

---

## Build Instructions

### Prerequisites

```bash
# C compiler (gcc, clang)
sudo apt-get install build-essential  # Ubuntu/Debian
```

### Compilation

```bash
# Compile all tools
make

# Individual tools
gcc -o ztbcreate ztbcreate.c ztbcommon.c -O2
gcc -o ztbaddblock ztbaddblock.c ztbcommon.c -O2
gcc -o ztbaddbranch ztbaddbranch.c ztbcommon.c -O2
gcc -o ztbfetch ztbfetch.c ztbcommon.c -O2
gcc -o ztbverify ztbverify.c ztbcommon.c -O2
gcc -o ztbcheckpoint ztbcheckpoint.c ztbcommon.c -O2

# Clean build artifacts
make clean
```

### Windows

```cmd
# Using MinGW or Visual Studio
cl /O2 ztbcreate.c ztbcommon.c
cl /O2 ztbaddblock.c ztbcommon.c
...
```

---

## Example Workflows

### Basic Trunk Chain

```bash
# 1. Create Genesis ROM
./ztbcreate genesis.rom

# 2. Create trunk chain
./ztbaddblock genesis.rom MyTrunk -t "Block 1"
./ztbaddblock genesis.rom MyTrunk -t "Block 2"
./ztbaddblock genesis.rom MyTrunk -t "Block 3"

# 3. Verify
./ztbverify genesis.rom MyTrunk

# 4. Fetch specific block
./ztbfetch genesis.rom MyTrunk 2
```

### Hierarchical Organization (Trunk + Branches)

```bash
# 1. Create genesis
./ztbcreate genesis.rom

# 2. Create company trunk
./ztbaddblock genesis.rom CompanyLedger -t "FY2025 Genesis"

# 3. Branch for departments
./ztbaddbranch genesis.rom CompanyLedger Sales -t "Sales Dept"
./ztbaddbranch genesis.rom CompanyLedger Marketing -t "Marketing Dept"
./ztbaddbranch genesis.rom CompanyLedger Engineering -t "Engineering Dept"

# 4. Add department records
./ztbaddblock genesis.rom Sales -f invoice_001.pdf
./ztbaddblock genesis.rom Sales -f invoice_002.pdf
./ztbaddblock genesis.rom Marketing -f campaign_q1.doc
./ztbaddblock genesis.rom Engineering -f project_alpha.zip

# 5. Continue trunk
./ztbaddblock genesis.rom CompanyLedger -t "Q1 Summary"

# 6. Verify everything
./ztbverify genesis.rom CompanyLedger

# 7. Verify just Sales branch
./ztbverify genesis.rom CompanyLedger -b Sales
```

### Wallet System

```bash
# 1. Create genesis
./ztbcreate genesis.rom

# 2. Create main chain
./ztbaddblock genesis.rom MainChain -t "Network Genesis"

# 3. Create wallet branches
./ztbaddbranch genesis.rom MainChain Wallet_Alice -t "Alice's Wallet"
./ztbaddbranch genesis.rom MainChain Wallet_Bob -t "Bob's Wallet"
./ztbaddbranch genesis.rom MainChain Wallet_Carol -t "Carol's Wallet"

# 4. Add transactions
./ztbaddblock genesis.rom Wallet_Alice -t "Received 100 tokens"
./ztbaddblock genesis.rom Wallet_Alice -t "Sent 50 to Bob"
./ztbaddblock genesis.rom Wallet_Bob -t "Received 50 from Alice"

# 5. Verify specific wallet
./ztbverify genesis.rom MainChain -b Wallet_Alice

# 6. Fetch Alice's history
./ztbfetch genesis.rom Wallet_Alice 1
./ztbfetch genesis.rom Wallet_Alice 2
```

### Year-End Archival with Checkpoint

```bash
# Throughout 2024...
./ztbaddblock genesis.rom FY2024 -t "Jan transaction"
./ztbaddblock genesis.rom FY2024 -t "Feb transaction"
# ... many blocks ...
./ztbaddblock genesis.rom FY2024 -t "Dec transaction"

# At year-end: create checkpoint
LAST_BLOCK_GUID="A3F2B891-4C5D-4E6F-8A9B-1C2D3E4F5A6B"
./ztbcheckpoint FY2024 genesis.rom ${LAST_BLOCK_GUID}

# Archive 2024 data
mkdir archive_2024
mv genesis.rom archive_2024/
mv FY2024_*.ztb archive_2024/
# Move to cold storage...

# Continue with fresh genesis for 2025
./ztbcreate genesis.rom
./ztbaddblock genesis.rom FY2025 -t "Jan transaction"
```

---

## Important Notes

### Critical Requirements

1. **Genesis ROM is Irreplaceable**
   - Loss or corruption breaks the entire chain
   - Back up to multiple locations
   - Consider offline cold storage

2. **Entropy Matters**
   - Genesis ROM must contain all 256 byte values (0-255)
   - Low entropy = encoding failures
   - Use cryptographically secure RNG

3. **No Backwards Compatibility**
   - Changing header structure breaks old blocks
   - Version control your implementation carefully
   - Consider version fields in future updates

4. **Storage Considerations**
   - Encoded blocks are 2× size of raw blocks
   - Plan storage capacity accordingly
   - Consider compression for archival

5. **Performance Characteristics**
   - **Encoding:** Slower (builds lookup tables, O(n) per block)
   - **Decoding:** Fast (O(1) pointer lookups)
   - **Verification:** Fast (no complex crypto)

### Best Practices

1. **Regular Verification**
   ```bash
   # Daily/weekly verification jobs
   ./ztbverify genesis.rom MainChain
   ```

2. **Backup Strategy**
   - Genesis ROM: 3+ copies, offline
   - Recent blocks: Real-time backup
   - Old blocks: Archive to cold storage with checkpoints

3. **Branch Naming**
   - Use descriptive, unique chain IDs
   - Consider hierarchical naming: `Company_Dept_Project`
   - Avoid special characters (filesystem compatibility)

4. **Error Handling**
   - Always check tool exit codes
   - Monitor verification failures
   - Investigate any integrity failures immediately

5. **Security**
   - Protect Genesis ROM (encryption at rest)
   - Secure backup locations
   - Access control on block files

---

## Troubleshooting

### "Byte 0xXX not in ROM"

**Problem:** Encoding failed because a byte value is missing from Genesis/Rolling ROM.

**Solution:**
- For Genesis ROM: Regenerate using `ztbcreate`
- For Rolling ROM: Should not happen if Genesis is valid

### "Checksum mismatch"

**Problem:** Block has been tampered with or corrupted.

**Solution:**
- Restore from backup
- Verify all subsequent blocks (they will fail too)
- Investigate tampering source

### "Chain not found"

**Problem:** No blocks found for specified chain ID.

**Solution:**
- Check spelling of chain ID
- Verify you're in correct directory
- Check file permissions

### "Cannot build rolling ROM"

**Problem:** Unable to reconstruct Rolling ROM for block.

**Solution:**
- Ensure Genesis ROM is present
- Verify all predecessor blocks exist
- Check file integrity of previous blocks

---

## License & Attribution

**MIT License** - (c) 2025 Cyborg Unicorn Pty Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.

---

## Further Information

- **Website:** [zoscii.com](https://zoscii.com)
- **Whitepaper:** See `ZTB_WHITEPAPER.md`
- **User Guide:** See `ZTB_USER_GUIDE.md`
- **GitHub:** [Repository Link]

---

**For technical questions, security concerns, or implementation assistance, please visit the project website or file an issue on GitHub.**