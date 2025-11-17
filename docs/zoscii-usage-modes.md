# ZOSCII Modes of Use

## Mode 1: Deniability Mode
**Property**: Data absence - addresses without ROM are meaningless  
**Security**: Cannot prove which decoding is correct (epistemological)  
**Key Distribution**: Genesis ROM NOT shared, kept secret

**Examples**:
- Crypto key storage on seized devices ("That's my shopping list, not a key")
- Whistleblower communications (cannot prove content)
- Border crossing with encrypted data (plausible alternative meanings)
- Private document storage where content must remain deniable

---

## Mode 2a: Noise Communication Mode
**Property**: Messages appear as random data in transit  
**Security**: Traffic analysis resistant, cannot prove communication occurred  
**Key Distribution**: Shared Genesis ROM with communicating parties

**Examples**:
- Covert channel communications (looks like network noise)
- Secure messaging between parties (A to B with noise)
- Command and control channels (operational security)
- Anti-surveillance communications (metadata protection)

---

## Mode 2b: Zero-Config IoT Mode
**Property**: Devices auto-discover and communicate without pairing  
**Security**: Wrong devices see noise and ignore, no attack surface  
**Key Distribution**: Shared Genesis ROM with communicating devices

**Examples**:
- Smart home devices (automatic secure mesh)
- Car security systems (phone unlocks without Bluetooth pairing)
- Industrial sensors (self-organizing secure networks)
- Medical device networks (automatic HIPAA-compliant communication)

---

## Mode 3: Automatic Key Rotation Mode
**Property**: Rolling ROM generates unique "key" per message/block  
**Security**: Self-generating one-time pad effect, no key reuse  
**Key Distribution**: Genesis ROM NOT shared, generated keysets are

**Examples**:
- Long-term secure communications (automatic OTP-like behavior)
- Message chains (each message has different effective key)
- Session security (automatic rotation without manual intervention)
- Blockchain links (each block uses previous blocks in Rolling ROM)

---

## Mode 4: Filtered Access Mode
**Property**: Same Genesis ROM, different memory blocks decode different data  
**Security**: Hierarchical/selective disclosure, partial ROM exposure doesn't compromise all  
**Key Distribution**: Genesis ROM can be shared, but better for some cases to share a Filter ROM for safety

**Examples**:
- API key management (different teams access different keys)
- Multi-tenant SaaS (each tenant decodes only their data)
- Corporate hierarchy (CEO sees all, managers see subset)
- Time-based access rotation (Q1 keys vs Q2 keys)
- Partial ROM compromise recovery (exposed subset doesn't lose everything)

---

## Mode 5: Integrity Proof Mode (ZTB)
**Property**: Tamperproof verification via pointer alignment  
**Security**: 10^152900 combinations per block (combinatorially impossible to forge)  
**Key Distribution**: Genesis ROM sharing does not affect tamperproof nature

**Examples**:
- Legal document timestamping (prove unmodified since creation)
- Academic credential verification (diploma authenticity)
- Supply chain provenance (product history integrity)
- Medical records (prove data unchanged)
- Financial audit trails (tamper-evident transaction logs)

---

## Personal (Single-User) Mode
**Property**: Local-only encoding of personal storage (e.g., a hard drive)  
**Security**: No external key sharing required; secrecy depends on local key storage and device security  
**Key Distribution**: Key material (encodings / Rolling ROM variants) is generated and kept locally — the Genesis ROM can remain static and public if desired

**Examples / Notes**:
- Encrypting your personal hard drive where only you need access (no sharing of keys)  
- Key never needs to be shared — you can derive encodings locally from the Genesis ROM + local seed/entropy  
- Practical considerations:
  - **Backup & recovery**: securely back up your derived key material or recovery seed (loss of key = loss of data)  
  - **Secure storage**: store keys in a hardware secure enclave, TPM, or offline backup  
  - **Deniability**: optional — you may choose to apply deniability layering (Mode 1) even for personal use  
  - **Non-shared Genesis ROM**: not required; Genesis ROM can be static/public because active key material is local and never shared

---

## Combined Modes

Many applications use multiple modes simultaneously:

**Secure Blockchain with Private Payloads**:
- Mode 5 (Integrity Proof) for blockchain structure
- Mode 1 or 2a (Deniability) for ZOSCII-encoded payloads within blocks
- Public can verify integrity; only ROM holders read content

**Enterprise IoT with Hierarchical Access**:
- Mode 2b (Zero-Config IoT) for device communication
- Mode 3 (Automatic Rotation) for ongoing security
- Mode 4 (Filtered Access) for different access levels

**Whistleblower Document Drop**:
- Mode 1 (Deniability) for document content
- Mode 2a (Noise Communication) for transmission

---

## Key Distinctions

**Deniability vs Integrity**:
- Deniability (Mode 1): "Cannot prove what this is"  
- Integrity (Mode 5): "Can prove this hasn't changed"  
- Both use same core technology, opposite purposes

**Authentication vs Verification**:
- Real-time authentication: ❌ Not suitable (login screens need immediate proof)  
- Historical verification: ✅ Perfect (ZTB proves data unchanged since creation)

**Quantum Resistance**:
- All modes are quantum-resistant because security is information-structural, not computational  
- ZOSCII files cannot reveal original data, even with unlimited computing power  
- Plausible deniability ensures observers cannot know the original content without the key/encodings

---

## Important Notes

- With traditional encryption, your data remains in the encrypted file; losing the key makes recovery extremely unlikely, though theoretically possible with future breakthroughs  
- With ZOSCII, your data is **never stored in the encoded file**; losing the Genesis ROM or local key material means the data is irretrievably lost  
- During transmission, ZOSCII sends the treasure map, not the treasure — without the Genesis ROM, the signal is indistinguishable from random noise