# ZOSCII Tamperproof Blockchain - User Guide

*(c) 2025 Cyborg Unicorn Pty Ltd - MIT License*

---

## ztbcreate - Create Your Blockchain Foundation

### What it does:
Creates the Genesis ROM - the immutable "seed" that anchors your entire blockchain.

### Why you need it:
Every blockchain starts here. This 64KB file of high-entropy random data becomes the root of trust. Without it, nothing else works.

### When to use it:
Once, at the very beginning, before creating any blocks.

### What happens:
- Generates 64KB of random data
- Analyzes entropy to ensure all 256 byte values (0-255) are present
- Warns if any values are missing (would break encoding later)
- Saves the Genesis ROM file

> **Important:** Keep this file safe and backed up! If you lose it, you can never decode your blocks again. All verification depends on this file.

### Example:
```bash
ztbcreate selfie.jpg genesis.rom
```

ztbcreate <entropyfile1> [<entropyfile2>] [<entropyfile3>] genesis.rom

Note: 1 to 3 entropy files are recommended to be JPEGs or MP3s or some other capture of real-world physics.

---

## ztbaddblock - Add Data to Your Chain

### What it does:
Adds a new block of data to an existing chain (trunk or branch).

### Why you need it:
This is how you store data on the blockchain - documents, transactions, records, anything.

### When to use it:
Every time you want to add new data to your chain.

### What happens:
- Takes your data (text string or file)
- Pads it to minimum size if needed
- Builds a Rolling ROM from previous blocks + Genesis
- Encodes your data using ZOSCII (makes it tamperproof)
- Creates a new block file: `chainid_0001_blockguid.ztb`

### Options:
- `-t "text"` - Add a text string
- `-f filename` - Add a file's contents
- `-x1` - Enable X1 extended security mode (see below)

### Example:
```bash
# First block on a new trunk
ztbaddblock genesis.rom MyTrunk -t "Invoice #001"

# Second block
ztbaddblock genesis.rom MyTrunk -t "Invoice #002"

# Add a file
ztbaddblock genesis.rom MyTrunk -f contract.pdf
```

---

## ztbaddbranch - Create a Parallel Chain

### What it does:
Creates a new branch that splits off from the trunk at the current point.

### Why you need it:
Perfect for organizing data hierarchically. Think: trunk = company, branches = departments. Or trunk = financial year, branches = accounts.

### When to use it:
When you want to create a separate but linked chain of blocks.

### What happens:
- Checks that the trunk exists
- Checks that the branch name isn't already used
- Creates the first block of the new branch
- Links it to the current trunk tip
- Future blocks can be added to this branch using `ztbaddblock`

### Example:
```bash
# Create main company trunk
ztbaddblock genesis.rom CompanyLedger -t "2024 Books"

# Branch off for departments
ztbaddbranch genesis.rom CompanyLedger Sales -t "Sales Q1"
ztbaddbranch genesis.rom CompanyLedger Marketing -t "Marketing Q1"

# Now add blocks to branches
ztbaddblock genesis.rom Sales -t "Sale #001"
ztbaddblock genesis.rom Marketing -t "Campaign #001"
```

> **Note:** Add `-x1` to any `ztbaddbranch` or `ztbaddblock` call to enable X1 mode for that block. All blocks in a chain should use the same mode consistently.

---

## ztbfetch - Read Your Data Back

### What it does:
Retrieves and decodes a specific block from any chain.

### Why you need it:
To actually read the data you stored! Blocks are ZOSCII-encoded, so you need this tool to decode them.

### When to use it:
Whenever you want to see what's in a block.

### What happens:
- Finds the requested block file
- Reconstructs the Rolling ROM (samples from previous blocks)
- Decodes the ZOSCII-encoded data
- Verifies the CRC32 checksum (detects tampering)
- Shows you the decoded payload

### Example:
```bash
# Fetch block 3 from MyTrunk
ztbfetch genesis.rom MyTrunk 3

# Output shows:
# - Block metadata (ID, timestamp, etc.)
# - Checksum verification
# - Your decoded data
```

---

## ztbverify - Prove Integrity

### What it does:
Verifies that your blockchain is intact and untampered. Works backwards from the latest block to Genesis.

### Why you need it:
To prove your data hasn't been altered. If anyone modifies even one byte in block 5, verification will fail on block 6 (and all subsequent blocks).

### When to use it:
- Before presenting data as evidence
- After receiving blocks from someone else
- Periodically to ensure your archive is intact
- After restoring from backup

### What happens:
- Scans all blocks in the chain
- Rebuilds each block's Rolling ROM
- Decodes each block
- Verifies CRC32 checksums
- Tests the tamperproof linking
- Reports PASS/FAIL for each block

### Options:
- No flag - Verify trunk and all branches
- `-t` - Verify trunk only
- `-b branchid` - Verify specific branch only
- `-bb` - Verify all branches only (skip trunk)

### Example:
```bash
# Verify everything
ztbverify genesis.rom MyTrunk

# Verify just the trunk
ztbverify genesis.rom MyTrunk -t

# Verify specific branch
ztbverify genesis.rom MyTrunk -b Sales
```

**Result:** If ANY block has been tampered with, verification fails and tells you exactly which block is corrupt.

---

## ztbcheckpoint - Archive Old Data

### What it does:
Creates an archival boundary that lets you move old blocks to slow/cheap storage while keeping the chain verifiable.

### Why you need it:
Over time, chains grow huge. Checkpoints let you archive historical data without breaking the chain.

### When to use it:
- End of financial year
- When chain gets too large
- Before migrating to new storage
- Creating self-contained archives

### What happens:
- Creates a special Type 2 (checkpoint) block
- This block becomes a new "mini-genesis"
- Blocks after this point can build Rolling ROMs from checkpoint → Genesis (skipping archived blocks)
- You can move blocks BEFORE the checkpoint to slow storage
- Each archive gets its own copy of Genesis ROM for independent verification

### Example:
```bash
# At end of 2024
ztbcheckpoint MyTrunk genesis.rom last_block_of_2024_guid

# Now you can:
# 1. Move all 2024 blocks + Genesis ROM to archive storage
# 2. Keep checkpoint + 2025 blocks on fast storage
# 3. Both archives remain independently verifiable
```

---

## Real-World Workflow Example

**Scenario:** Accounting firm tracking invoices with separate accounts

```bash
# 1. Start fresh
ztbcreate selfie.jpg genesis.rom

# 2. Create main trunk for 2025
ztbaddblock genesis.rom FY2025 -t "Financial Year 2025"

# 3. Create branches for different accounts
ztbaddbranch genesis.rom FY2025 ClientA -t "Client A Account"
ztbaddbranch genesis.rom FY2025 ClientB -t "Client B Account"

# 4. Add invoices to Client A
ztbaddblock genesis.rom ClientA -t "Invoice #1001"
ztbaddblock genesis.rom ClientA -f invoice_1002.pdf

# 5. Add invoices to Client B
ztbaddblock genesis.rom ClientB -t "Invoice #2001"

# 6. Retrieve a specific invoice
ztbfetch genesis.rom ClientA 2

# 7. Verify everything is intact
ztbverify genesis.rom FY2025

# 8. At year-end, create checkpoint
ztbcheckpoint FY2025 genesis.rom last_block_guid

# 9. Archive 2025 data to slow storage, start FY2026
```

---

## Key Concepts for Users

### Tamperproof
If you change ANY byte in block N, all blocks N+1 onwards fail verification. The ZOSCII pointer encoding makes forgery combinatorially impossible (10^152900 combinations).

### X1 Extended Security Mode
X1 mode adds an additional layer on top of the standard tamper-evidence. Each block is XOR'd against the entire previous block file as it sits on disk. This means every block also commits to a CRC32 of the previous block's complete on-disk content — including its own encoding and XOR. An attacker attempting to forge block N would need to simultaneously satisfy two interdependent integrity checks: the current block's own CRC, and the CRC that block N+1 has already committed to for block N's on-disk form. Add `-x1` to any `ztbaddblock` or `ztbaddbranch` call to enable it. The first block of a trunk has no predecessor and is stored plain even in X1 mode — this is by design.

**When to use X1:** The rolling ROM samples the first 1KB of each block's encoded data. For payloads under 1KB, the entire block falls within that sample — every subsequent block's encoding table is directly derived from the full content, and the combinatorial tamper-evidence applies to the whole payload without X1. For payloads larger than 1KB, content beyond the first 1KB is protected only by CRC32 in normal mode. X1 closes that gap. Use normal mode for short records (invoices, transactions, log entries). Use `-x1` when storing large files such as PDFs, images, or contracts where the full payload integrity matters and the modest performance cost is acceptable.

### No Decryption Needed
This isn't encryption - it's addressing. The data is encoded as pointers into the Rolling ROM. Without the exact Rolling ROM, the pointers are meaningless.

### Independent Archives
Checkpoints + Genesis copies let you create self-contained archives that verify independently. Perfect for legal compliance, backups, and long-term storage.

### Branch Organization
Use branches like folders. Trunk = main timeline, branches = parallel sub-timelines. All verified together.

---

## Quick Reference

| Command | Purpose | Usage |
|---------|---------|-------|
| `ztbcreate` | Create Genesis ROM | `ztbcreate selfie.jpg genesis.rom` |
| `ztbaddblock` | Add data block | `ztbaddblock genesis.rom chain -t "text" [-x1]` |
| `ztbaddbranch` | Create new branch | `ztbaddbranch genesis.rom trunk branch -t "text" [-x1]` |
| `ztbfetch` | Read block data | `ztbfetch genesis.rom chain index` |
| `ztbverify` | Verify integrity | `ztbverify genesis.rom trunk` |
| `ztbcheckpoint` | Archive boundary | `ztbcheckpoint trunk genesis.rom prev_guid` |

---

**For more information:** Visit [zoscii.com](https://zoscii.com)

**License:** MIT License - Free to use, modify, and distribute