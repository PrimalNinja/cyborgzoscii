# ZOSCII Rolling Hash: Tamper Detection from the Inside

**Author:** Julian Cassin  
**Date:** 2026-05-23

## MIT LICENSE

ZOSCII Rolling Hash is released under the MIT License.

---

## ZOSCII Rolling Hash = Tamper Detection Inside ZOSCII 🐍

You already have I(M;A)=0. Why would you need a hash?

Because tamper detection and information-theoretic security are different things. ZOSCII ensures an attacker cannot decode your message without the ROM. ZOSCII Rolling Hash ensures they cannot silently modify it either — and the receiver knows immediately if they tried.

---

## What Is ZOSCII Rolling Hash?

A 32-bit (4-byte) tamper detection hash using a 4-pass BRAINLESS-style XOR chain.

**It is designed to live inside a ZOSCII or UNSIGNAL encoding — not outside it.**

The payload is ZOSCII-encoded *after* the hash is prepended. This means:

- The hash itself is encoded and indistinguishable from payload addresses
- An attacker cannot locate the hash bytes to replace them without the ROM
- Any tamper to any encoded byte will decode to garbage — the hash won't match
- ZOSCII provides confidentiality. ZOSCII Rolling Hash provides integrity.

---

## The Algorithm

ZOSCII Rolling Hash uses 4 independent BRAINLESS-style XOR chains operating on interleaved strides of the input.

**Four strides:** Pass 0 processes bytes 0, 4, 8, 12... Pass 1 processes bytes 1, 5, 9, 13... and so on. Each stride produces one byte of the 4-byte hash. Each pass seeds the next.

**Two modes:**

**Forward (streamable):** Within each stride, seed from the first byte, walk forward XORing each subsequent stride byte into the running state. Circular: the final state XORs back into the seed. Streamable — starts from byte 0, can be computed as data arrives.

**Reverse (non-streamable):** Within each stride, seed from the last byte, walk backward toward byte 0 XORing each stride byte into the running state. Circular: the final state XORs back into the seed. Non-streamable — the last byte of each stride must be known before processing can begin, requiring the entire payload first.

The strides give the hash good diffusion properties when the payload is subsequently ZOSCII-encoded: adjacent input bytes end up in different strides, making their encoded address values less correlated.

---

## Usage

```csharp
// Generate — prepend hash to payload, then ZOSCII encode the combination
byte[] arrHash    = ZRollingHash.Bytes(arrPayload);            // reverse (default)
byte[] arrHash    = ZRollingHash.Bytes(arrPayload, true);      // forward
byte[] arrHash    = ZRollingHash.File("data.bin");             // from file

// Verify — after ZOSCII decode, extract hash and verify against payload
bool blnOk = ZRollingHash.Verify(arrPayload, arrHash);
bool blnOk = ZRollingHash.VerifyFile("data.bin", arrHash);
```

**Intended workflow:**

```
plaintext → [ hash + plaintext ] → ZEncode/UEncode → transmit
receive   → ZDecode/UDecode    → read hash     → verify
```

The hash is always computed on the plaintext before encoding and verified on the plaintext after decoding.

---

## Properties

| Property | Value |
|---|---|
| Output size | 4 bytes (32 bits) |
| Algorithm | 4-pass BRAINLESS-style XOR chain with circular Ouroboros step |
| Collision resistance | 1 in 4,294,967,296 by chance |
| ROM required | No |
| Key required | No |
| Reversible | No |
| Streamable | Forward mode only |
| Designed for | Inside ZOSCII/UNSIGNAL encoding |

---

## Why Inside, Not Outside?

Placing the hash **outside** ZOSCII encoding means an attacker can see it. They can compute the hash of their tampered payload and replace it. The hash provides no protection.

Placing the hash **inside** means:

- The hash is encoded as ZOSCII addresses — indistinguishable from the rest of the payload
- An attacker cannot locate or replace the hash bytes without the ROM
- The hash is the sole means of tamper detection — and it is protected by I(M;A)=0

---

## ZOSCII Rolling Hash vs CRC32

Both are 32-bit tamper detection tools. The choice depends on your context.

| Feature | ZOSCII Rolling Hash | CRC32 |
|---|---|---|
| Output size | 4 bytes | 4 bytes |
| Collision resistance | 1 in ~4 billion | 1 in ~4 billion |
| Algorithm | BRAINLESS-style XOR chain | Polynomial division with lookup table |
| Lookup table required | No | Yes (1KB standard table) |
| Streamable | Forward mode only | Always |
| Non-streamable mode | Reverse mode | Not available |
| Designed for | Inside ZOSCII encoding | General purpose |
| ROM/key required | No | No |
| Deterministic | Yes | Yes |
| Reversible | No | No |

**ZOSCII Rolling Hash requires no lookup table** — CRC32 needs a 1KB polynomial table. On extremely constrained hardware (1971 Intel 4004, for example) this matters.

**ZOSCII Rolling Hash has a non-streamable reverse mode** — CRC32 is always streamable. Reverse BRAINLESS-style mode ensures nothing can be computed until the entire payload is received, which is useful when partial processing must be prevented.

---

## When to Choose ZOSCII Rolling Hash

- Payload will be ZOSCII or UNSIGNAL encoded — use ZOSCII Rolling Hash inside
- Extremely constrained hardware with no room for a CRC32 table
- You need a non-streamable checksum that prevents partial processing
- You want a tamper check that is itself protected by I(M;A)=0

## When to Choose CRC32

- Payload is not ZOSCII encoded — use CRC32
- Standard interoperability with other systems is required
- The checksum will be stored or transmitted separately from the payload
- Can you still use CRC32 inside a ZOSCII encoded file? Sure, go ahead

---

## The Philosophy

ZOSCII encodes. ZOSCII Rolling Hash detects.

Simple tools, each doing one thing well. No complexity. No key schedules. No rounds. No S-boxes.

When your foundation is I(M;A)=0, you don't need sophistication. You need just enough — and ZOSCII Rolling Hash is just enough.

Simple. Verifiable. MIT Licensed.

---

(c) 2026 Cyborg Unicorn Pty Ltd — https://cyborgunicorn.com.au