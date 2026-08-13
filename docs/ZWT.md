# ZWT — ZOSCII Web Tokens

**Version 0.1 (DRAFT)**
**Author:** Julian Cassin

A quantum-proof, opaque session/attestation token. The JWT analogue for ZOSCII: an issuer (e.g. Google) attests a user to a relying party (e.g. Suno), but unlike JWT the token is information-theoretically opaque and its verification structure is concealed.

---

## Keys

| Key | Held by | Role |
|-----|---------|------|
| **SHAREDROM** | Issuer + Relying Party | Shared per-relationship key |
| **ISSUERROM1** and **ISSUERROM2** | Issuer only | Issuer's private ROM — never shared. One pair for all relying parties, or one pair per relying party |

---

## Construction

```
sharedsignature = a GUID or similar
issuersignature = sharedsignature

// issuer block body: version, length table, then the fields themselves
// (privateclaims is the last field, so it gets no length entry — see Wire format)
issuerbody  = version(0) + length(issuersignature) + issuersignature + privateclaims
issuerblock = rollinghash(issuerbody) + issuerbody
              // hash goes at the FRONT, and covers version + lengths + fields
issuerdata  = encode(ISSUERROM1, encode(ISSUERROM2, issuerblock))
              // issuer seals the shared-sig with its private ROM, double-encoded

// shared block body: version, length table, then the fields themselves
// (issuerdata is the last field, so it gets no length entry either)
sharedbody  = version(0) + length(sharedsignature) + length(sharedclaims)
              + sharedsignature + sharedclaims + issuerdata
sharedblock = rollinghash(sharedbody) + sharedbody
zwt         = encode(SHAREDROM, sharedblock)
```

The `sharedsignature` and `issuersignature` are identical values.

In the concrete construction, `encode(ROM, ...)` is a reversible UNSIGNAL encoding of a **block** (see Wire format below), and `rollinghash(...)` is a 4-byte integrity field carried *inside* that block — not a one-way wrapper around the payload. So `encode` and `decode` are inverses: the relying party opens the shared block with SHAREDROM and reads the fields back out; the issuer additionally opens the issuer block with ISSUERROM1 and ISSUERROM2. The rolling hash is verified on open, binding every field (version, lengths, and blobs).

`issuerblock` is the raw frame before double-encoding; `issuerdata` is the encoded bytes of that frame, carried as a field inside the shared block. Likewise `sharedblock` is the raw frame before encoding; the fully-encoded result is the `zwt` itself.

---

## Wire format

The token is a flat, versioned structure, UNSIGNAL-encoded — readable on any target (Z80, 6502, C, C#, Python) with nothing but base-plus-offset arithmetic. All multi-byte integers are **little-endian**.

A ZWT is a single token: a **shared block** the relying party opens with SHAREDROM, whose `issuerdata` field is itself an **issuer block** the issuer opens additionally with ISSUERROM1 and ISSUERROM2.

Both have the same shape — a header (rolling-hash CRC, version, a 2-byte length for every field except the last), then the fields. The last field in each block has no length entry: it simply runs from where the previous field ends to the end of the block, so no pointer or trailing length is needed for it. The whole token reads top to bottom:

```
offset  size  field
------  ----  --------------------------------------------------------------
(UNSIGNALed with shared ROM)
0		4	rolling hash (CRC)
4		2	version								= 0
6		2	length of sharedsignature			(LE)
8		2	length of sharedclaims				(LE)
10  	..	sharedsignature
..      ..	sharedclaims

..		..	issuerdata							(Double UNSIGNALed with issuer ROMs 1 & 2)
			+0	4	rolling hash (CRC)
			+4	2	version						= 0
			+6	2	length of issuersignature	(LE)
			+8	..	issuersignature
			+..	..	privateclaims
```

`issuerdata` has no length field of its own — it is the last field of the shared block, so it runs to the end of the token. `privateclaims` has no length field of its own for the same reason — it is the last field of the issuer block, so it runs to the end of that block.

The issuer block is built and UNSIGNAL-encoded first, then carried verbatim as the `issuerdata` field of the shared block. The relying party opens only the shared block; the issuer block nested inside stays opaque to it and is opened separately by the issuer.
Each block's rolling hash covers everything in that block after its own 4-byte hash — its version, its length table, and its fields.

**Design choices, and why:**

- **No pointers, only lengths — and the last field needs neither.** Fields are in fixed order, so each blob starts where the previous ended — a pointer would just be the running sum of prior lengths. The last field in a block needs no length either, since nothing follows it: it simply extends to the end of the block. Dropping pointers means the only 16-bit limit is each *length* field, giving every segment its own full 64 KB range while the whole frame may exceed 64 KB.
- **No field-count byte, no type tags, no field IDs.** The segment schema is fixed per version and known by both parties out of band. A reader already knows it is parsing a 2-field issuer block or a 3-field shared block, so self-description would be dead weight. Parties simply read the segments they hold the key for — the RP reads `sharedsignature` + `sharedclaims` with SHAREDROM; the issuer reads `issuersignature` + `privateclaims` with ISSUERROM1 and ISSUERROM2.
- **Version byte for forward compatibility.** Version 0 fixes the segment list above. A future version may append segments without breaking version-0 readers; the version byte is inside the hash coverage, so it cannot be altered undetected.
- **Hash covers everything except itself.** The 4-byte rolling hash at offset 0 is computed over the version byte, the length table, and all blobs (`frame[4 .. end]`). Version and lengths are therefore integrity-bound, not just the payload.
- **Double UNSIGNAL encoded issuer data.** The `issuerdata` is double encoded because the relying party
has the plain text of the `sharedsignature`; double encoding removes any possibility of a plain text attack on
the issuer's `privateclaims`. `issuersignature` and `privateclaims` could individually be single layered
but double layering both has the same overhead with greater protection.

Because `issuerdata` is itself a complete double UNSIGNAL-encoded block carried inside the shared block, `privateclaims` passes through UNSIGNAL three times (twice from the issuer block's own double encoding, once more when the shared block that contains it is encoded) while `sharedclaims` passes through once.

A typical token — a 16-byte GUID `sharedsignature` with short claims — lands around **0.9–1.2 KB**; the size is dominated by UNSIGNAL's three layers of random padding rather than the claims, so putting GUID-sized values in the claims barely changes it.

---

## Verification

| Verifier | Checks | With |
|----------|--------|------|
| **Relying Party** | can open the ZWT and read `sharedsignature` + shared claims | SHAREDROM |
| **Issuer** | `sharedsignature` matches the copy sealed in `issuersignature` | ISSUERROM1 and ISSUERROM2 |

---

## Why nobody but the issuer can forge a valid token

- A forger without SHAREDROM can't open or produce a ZWT — outsiders locked out.
- A relying party **holds** SHAREDROM, so it can produce *a* `sharedsignature` — but it **cannot** produce a matching `issuersignature`, because that requires ISSUERROM1 and ISSUERROM2 (issuer-only).
- A `sharedsignature` is only **valid** when it matches the copy sealed inside `issuersignature`.
- Therefore only the issuer can produce a valid token. A forged shared-sig has no matching sealed copy and fails.

**Even a fully compromised relying party cannot mint a token the issuer will accept.**

---

## Properties

| Property | How |
|----------|-----|
| **Opaque** | Whole ZWT is UNSIGNAL-encoded; payload, signatures, and structure are indistinguishable from noise (I(M;A)=0) |
| **Concealed verification structure** | An observer can't tell how many signatures exist, which keys govern them, or where they are |
| **Cross-site inert** | A ZWT is unvalidatable by any party without the relationship key — no `aud` check needed; misuse is structural, not just forbidden |
| **Quantum-proof** | No asymmetric primitive; nothing for Shor's algorithm to attack |
| **Unforgeable** | Valid tokens require the issuer's private ROM pair (via the sealed-shared-sig binding) |

---

## Not solved by ZWT alone

| Gap | Note |
|-----|------|
| **Replay to the legitimate relying party** | A stolen ZWT can be replayed *to its intended recipient*. Bind a server-issued nonce inside the token; avoid clock-based expiry (clocks are attacker-influenceable). |
| **Revocation** | Stateless local verification can't revoke mid-life. If needed, verify `issuersignature` via issuer introspection instead — gains revocation, costs a round-trip. |

---

## Notes

- `sharedclaims` are readable by the relying party (opened with SHAREDROM); `privateclaims` are sealed inside `issuerdata` and readable only by the issuer.
- SHAREDROM is per-relationship, so a relying party forging a *shared*-sig could only ever affect its own door — a non-event — and the binding to `issuersignature` prevents even that from producing an issuer-valid token.