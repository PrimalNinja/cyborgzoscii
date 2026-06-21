# ZOSCII Modes of Use

*(c) 2026 Cyborg Unicorn Pty Ltd - MIT License*

## Mode 1: Deniability Mode

**Property:** Data absence - addresses without ROM are meaningless

**Security:** Cannot prove which decoding is correct (epistemological)

**Key Distribution:** Genesis ROM NOT shared, kept secret

- Crypto key storage on seized devices ("That's my shopping list, not a key")
- Whistleblower communications (cannot prove content)
- Border crossing with encrypted data (plausible alternative meanings)
- Private document storage where content must remain deniable

## Mode 2a: Noise Communication Mode

**Property:** Messages appear as random data in transit

**Security:** Traffic analysis proof, cannot prove communication occurred

**Key Distribution:** Shared Genesis ROM with communicating parties

- Covert channel communications (looks like network noise)
- Secure messaging between parties (A to B with noise)
- Command and control channels (operational security)
- Anti-surveillance communications (metadata protection)

## Mode 2b: Zero-Config IoT Mode

**Property:** Devices auto-discover and communicate without pairing

**Security:** Wrong devices see noise and ignore, no attack surface

**Key Distribution:** Shared Genesis ROM with communicating devices

- Smart home devices (automatic secure mesh)
- Car security systems (phone unlocks without Bluetooth pairing)
- Industrial sensors (self-organizing secure networks)
- Medical device networks (automatic HIPAA-compliant communication)

## Mode 3: Automatic Key Rotation Mode

**Property:** Rolling ROM generates unique "key" per message/block

**Security:** Self-generating one-time pad effect, no key reuse

**Key Distribution:** Genesis ROM NOT shared, generated keysets are

- Long-term secure communications (automatic OTP-like behavior)
- Message chains (each message has different effective key)
- Session security (automatic rotation without manual intervention)
- Blockchain links (each block uses previous blocks in Rolling ROM)

## Mode 4: Filtered Access Mode

**Property:** Same Genesis ROM, different memory blocks decode different data

**Security:** Hierarchical/selective disclosure, partial ROM exposure doesn't compromise all

**Key Distribution:** Genesis ROM can be shared, but better for some cases to share a Filter ROM for safety

- API key management (different teams access different keys)
- Multi-tenant SaaS (each tenant decodes only their data)
- Corporate hierarchy (CEO sees all, managers see subset)
- Time-based access rotation (Q1 keys vs Q2 keys)
- Partial ROM compromise recovery (exposed subset doesn't lose everything)

## Mode 5: Integrity Proof Mode (ZTB)

**Property:** Tamperproof verification via pointer alignment

**Security:** 10^152900 combinations per block (combinatorially impossible to forge)

**Key Distribution:** Genesis ROM sharing does not affect tamperproof nature

- Legal document timestamping (prove unmodified since creation)
- Academic credential verification (diploma authenticity)
- Supply chain provenance (product history integrity)
- Medical records (prove data unchanged)
- Financial audit trails (tamper-evident transaction logs)

## Mode 6: Signature Mode

**Property:** Encoding a ROM with itself produces a unique self-referential signature

**Security:** Only the original ROM holder can verify; any modification breaks the equality property

**Key Distribution:** Original ROM must be held by verifier

**How it works:**

- Take a ROM (64KB of high-entropy data)
- Encode that ROM **using itself** as the encoding table
- The result is a unique fingerprint of that specific ROM
- When decoded **with the same original ROM**, the output equals the original ROM
- If decoded with any other ROM (even one byte different), the output is gibberish

**Security property:**

- Self-encoding creates a deterministic but non-reproducible signature
- Without the original ROM, the signature cannot be verified
- With the original ROM, verification is instant and definitive
- Any tampering with the ROM breaks the verification equality

**Use Cases:**

- **Software/Firmware Signing:** Sign a ROM with itself — only those with the original ROM can verify authenticity
- **Hardware Attestation:** Prove a device has the correct ROM without revealing it
- **Secure Boot:** Self-sign the boot ROM; verification ensures no tampering
- **Identity Anchoring:** A ROM's self-signature serves as a cryptographic identity fingerprint
- **Genesis ROM Verification:** Prove you have the correct Genesis ROM for a ZTB chain without sharing the entire 64KB
- **Out-of-Band Authentication:** Two parties with the same ROM can verify each other's possession without key exchange

**Mathematical Property:**
Let ROM = R (64KB). Signature S = ZOSCII_Encode(R, R).

Verification: ZOSCII_Decode(R, S) = R if and only if the decoding ROM is identical to the original.

This is a form of **zero-knowledge possession proof** — the verifier learns nothing about R except that the prover has the correct one.

## Mode 7: Agent Delegation Mode (007 Mode)

**Property:** Hierarchical, delegatable, self-destructing signatures for field agents

**Security:** Information-theoretic — no mathematical assumptions. Delegated signatures can be destroyed without compromising the master.

**Key Distribution:** Master (M) issues unique signature-ROMs to each agent. Agents never see the master ROM.

**How it works:**

- Master M creates a master ROM (R_master).
- M generates unique signature-ROMs (S_001, S_002, ..., S_007) by encoding random data **using R_master**.
- Each signature-ROM S_agent is a fully functional encoding table — not a fragment, not a partial key.
- Agents receive only their signature-ROM — never R_master. **Master M still keeps a copy of the S ROMs given to the agents.**
- In the field, Agent 007 uses S_007 to encode sensitive data.
- Agent 007 then **destroys S_007** (secure erase).
- The encoded data can now **only** be decoded by M using **their copy of S_007**.
- Even if Agent 007 is captured after destroying S_007, no future or past data can be compromised.

**Recursive Delegation (Playmate Scenario):**

- Agent 007 (still holding S_007) can create a **sub-signature** S_playmate for a trusted third party.
- S_playmate is generated by encoding random data **using S_007**.
- The playmate receives S_playmate and uses it to encode data.
- Agent 007 receives the encoded data, decodes it with S_007, then re-encodes it with S_007 (or destroys S_007 after).
- The playmate never sees S_007. M never needs to interact with the playmate.
- Result: **Multi-level, deniable, delegatable signature chain** with perfect forward secrecy at every level.

**Security Properties:**

- **No single point of compromise:** Compromised agent reveals only their own signature-ROM, not the master.
- **Self-destructing credentials:** Destroy signature-ROM → all future data encoded with it becomes undecodable _even to the agent who created it_.
- **Recursive delegation:** Any agent can become a "master" for a lower-tier agent without exposing the original master.
- **Deniable chain:** M can deny ever issuing a signature to Agent 007 — the signature-ROM proves nothing without R_master.
- **Quantum-proof:** I(M;A)=0 at every layer.

**Use Cases:**

- **Intelligence Operations:** M (station chief) issues signature-ROMs to field agents. Agents destroy signatures after use. Captured agents reveal nothing.
- **Corporate Espionage Countermeasures:** Executives issue delegatable signatures to project leads, who issue to team members. Revocation = destroy signature.
- **Whistleblower Protection:** Source receives a signature-ROM from a journalist. Source encodes evidence, destroys signature. Journalist decodes with master. Source cannot be compelled to decode — they no longer have the means.
- **Roleplaying as Secret Agents (for fun):** Anyone can experience agent-style delegation with better security than real agents likely have.
- **Temporary Access Credentials:** Issue a signature-ROM that self-destructs after N uses (by destroying it). No need for CRL, OCSP, or expiry dates.

**Why No Other Crypto Can Do This:**
Traditional PKI requires certificate revocation lists, OCSP stapling, or expiry dates. Private keys cannot be "partially destroyed" — once compromised, all past/future data is vulnerable.

ZOSCII Mode 7 allows **true cryptographic deletion** of signing capability without affecting the master. The signature-ROM is not a pointer to a master key — it is a **standalone encoding table**. Destroy it, and the ability to decode data encoded with it is **provably, permanently gone**.

This is not key revocation. This is **key annihilation**.

## Mode 8: Masked Encoding & Decoding Mode

**Property:** Controlled entropy — masks define which addresses are valid for encoding and which bytes are accepted during decoding

**Security:** Perfect plausible deniability with zero garbage; controlled application of physical entropy

**Key Distribution:** Encoding and decoding masks can be shared independently; masks can be ROMs, files, or logical patterns

**How it works:**

- **Encoding mask:** Defines which address ranges are valid for encoding (bounds control). This allows multiple independent "channels" within the same ROM, ensuring no clashes between different messages.
- **Decoding mask:** Filters out noise and garbage after decoding. Only bytes that satisfy the mask conditions are accepted; everything else is discarded.
- Masks can be applied to control the bounds of an encoding, or to eliminate noise injected into an encoding.
- Controlled entropy: multiple masks can use different physical entropy sources (JPEGs, MP3s, sensor data) placed in specific regions of the ROM.

**Security Properties:**

- **No garbage:** Conflicts are eliminated by the mask, providing perfect plausible deniability.
- **Controlled entropy:** The entropy is physical but placed where you want it.
- **Multi-layer deniability:** Different masks reveal different messages from the same address list.
- **Logical control:** Masks are logical filters, not cryptographic operations.

**Use Cases:**

- **Perfect plausible deniability:** Encode a real message with one mask, and a decoy with another — both decode cleanly.
- **Controlled entropy generation:** Use different physical entropy sources (JPEGs, MP3s) for different regions of the ROM.
- **Noise injection control:** Mask out noise that would otherwise corrupt the decoded output.
- **Channel separation:** Multiple messages in the same ROM, each using a different mask.
- **Sprite/transparency-style layering:** Like 8-bit graphics sprites, masks overlay and remove layers of data.

## Mode 9: Address Translation ROM Mode

**Property:** Map addresses from one ROM space to another via a translation ROM

**Security:** New layer of indirection — translation ROM is the shared secret, not the original ROM

**Key Distribution:** Translation ROM shared between parties; original ROMs can be different

**How it works:**

- Sender encodes message using ROM1 → Address list A (addresses in ROM1 space)
- Translation ROM maps each address in A to an address in ROM2 space → Address list B
- Receiver decodes Address list B using ROM2 → Message

The message is encoded in ROM1 and decoded in ROM2 — but the addresses are transformed in between by the translation ROM.  For translation ROMs, use ZOSCII in word mode, looking up words in the translation ROM that redirect to words in the new ROM.

**Transcoding Speed:**

- Full re-encoding requires re-encoding the entire message (slow)
- Translation ROM only maps addresses (fast — O(1) per address)
- If redistribution of randomness is not required, transcoding is extremely fast

**Security Properties:**

- **Deniability:** Sender can claim they used ROM1; receiver can show ROM2
- **Obfuscation:** Address list is transformed before transmission
- **Compatibility:** Different ROMs can be used without re-encoding
- **Key exchange:** Translation ROM is the shared secret — not the ROM itself

**Use Cases:**

- **Key exchange:** The translation ROM is the shared secret
- **ROM replacement:** Old ROMs can be translated to new ROMs without re-encoding
- **Multi-party communication:** Each party uses a different ROM, translated by a shared translation ROM
- **Speed-critical applications:** Real-time communication where re-encoding is too slow
- **Transcoding without redistribution:** Fast address mapping when randomness preservation is not required

## Mode 10: Mask Merging Mode (Controlled Entropy Composition)

**Property:** Merge multiple encoded streams into a single address list using different masks

**Security:** Controlled entropy — each mask decodes to a different message; adversary cannot separate the sources without all masks

**Key Distribution:** Each mask distributed independently; different masks for different parties

**How it works:**

- Encode real message with Mask 1 → Address set A
- Encode noise message with Mask 2 → Address set B
- Merge A and B → Combined address set C
- Decode with Mask 1 → Real message (A extracted)
- Decode with Mask 2 → Noise message (B extracted)

The addresses in C are a mix of A and B. Without the right mask, it looks like random noise. With the right mask, you can extract either the real message or the noise.

**Security Properties:**

- **Perfect plausible deniability:** Multiple clean messages from the same address list
- **Controlled entropy:** The noise is designed, not random
- **Multi-layer extraction:** Different masks reveal different layers
- **No garbage:** Masks ensure clean decoding

**Use Cases:**

- **Secure communications:** Real message + decoy noise
- **Whistleblowing:** Real message + innocent decoy
- **Evidence protection:** Real message + plausible deniability
- **Traffic analysis resistance:** Controlled entropy looks like noise
- **Coercion resistance:** Decoy message satisfies the adversary
- **Sprite/transparency layering:** Like 8-bit graphics, masks overlay and remove data layers

## Combined Modes

- **Secure Blockchain with Private Payloads:** Mode 5 (Integrity Proof) for blockchain structure, Mode 1 or 2a (Deniability) for ZOSCII-encoded payloads within blocks; public can verify integrity, only ROM holders read content
- **Enterprise IoT with Hierarchical Access:** Mode 2b (Zero-Config IoT) for device communication, Mode 3 (Automatic Rotation) for ongoing security, Mode 4 (Filtered Access) for different access levels
- **Whistleblower Document Drop:** Mode 1 (Deniability) for document content, Mode 2a (Noise Communication) for transmission
- **Hardware Identity Anchoring:** Mode 6 (Signature Mode) for device attestation, Mode 4 (Filtered Access) for different privilege levels
- **Secure Boot with Remote Verification:** Mode 6 (Signature Mode) for boot ROM signing, Mode 2a (Noise Communication) for attestation transmission
- **Agent Network with Self-Destructing Credentials:** Mode 7 (Agent Delegation) for field operations, Mode 2a (Noise Communication) for covert transport, Mode 1 (Deniability) for plausible deniability of payloads
- **Controlled Entropy Communications:** Mode 8 (Masked Encoding/Decoding) for channel control, Mode 10 (Mask Merging) for multiple deniable layers

## Key Distinctions

- **Deniability vs Integrity:** Deniability (Mode 1): "Cannot prove what this is"; Integrity (Mode 5): "Can prove this hasn't changed"
- **Authentication vs Verification:** Real-time authentication ❌ not suitable; Historical verification ✅ perfect (ZTB proves data unchanged since creation)
- **Signature vs Traditional Crypto:** Mode 6 (Signature) requires no asymmetric math — pure structural self-reference; verification is instant O(1) lookup
- **Agent Delegation vs PKI:** Mode 7 (Agent Delegation) allows true cryptographic deletion of signing capability; PKI only offers revocation (which can be reversed) and leaves private keys exposed
- **Quantum Proof:** All modes are quantum-proof; ZOSCII files cannot reveal original data even with unlimited computing power; plausible deniability ensures observers cannot know content without key/encodings
- **Mask vs Key:** Masks (Modes 8-10) are logical filters, not cryptographic keys — they control address validity and decoding acceptance, not mathematical transformations
- **Translation vs Re-encoding:** Mode 9 (Translation ROM) maps addresses directly (fast); re-encoding redistributes randomness (slower). Both are secure, but translation is faster when randomness redistribution is not required.

## Important Notes

- With traditional encryption, your data remains in the encrypted file; losing the key makes recovery extremely unlikely, though theoretically possible with future breakthroughs
- With ZOSCII, your data is **never stored** in the encoded file; losing the Genesis ROM or local key material means data is irretrievably lost
- During transmission, ZOSCII sends the treasure map, not the treasure — without the Genesis ROM, the signal is indistinguishable from random noise
- Mode 6 (Signature Mode) is unique to ZOSCII — no traditional cryptographic signature works this way; it's a pure structural self-reference with zero mathematical assumptions
- Mode 7 (Agent Delegation) is also unique — no other system allows recursive, delegatable, self-destructing signatures with information-theoretic security
- Modes 8-10 (Masks, Translation, Merging) are new primitives not in the original 7 usage modes — they represent the next evolution of ZOSCII's capabilities
- The "controlled entropy" of Modes 8-10 allows physical entropy sources to be placed exactly where you want them, enabling perfect plausible deniability with zero garbage

---

**License & Information**

**MIT License** - (c) 2026 Cyborg Unicorn Pty Ltd

[zoscii.com](https://zoscii.com) | [GitHub Repository](https://github.com/PrimalNinja/cyborgzoscii)

For technical questions, security concerns, or implementation assistance,
please visit the project website or file an issue on GitHub.