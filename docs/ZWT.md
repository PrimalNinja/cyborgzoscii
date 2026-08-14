# ZWT -- ZOSCII Web Tokens

**Version 0.2 (DRAFT)**
**Author:** Julian Cassin

A quantum-proof, opaque session/attestation token. The JWT analogue for ZOSCII: an issuer attests a user to a relying party, but unlike JWT the token is information-theoretically opaque and its verification structure is concealed. No asymmetric primitive -- nothing for Shor's algorithm to attack.

---

## Keys

| Key | Held by | Role |
|-----|---------|------|
| **SHAREDROM** | Issuer + Relying Party | Shared per-relationship key |
| **ISSUERROM1** and **ISSUERROM2** | Issuer only | Issuer's private ROM -- never shared. One pair for all relying parties, or one pair per relying party |

---

## Construction

sharedsignature = a GUID or similar
issuersignature = sharedsignature

The issuer block body contains: version, length of issuersignature, issuersignature, and privateclaims (the last field has no length).

The issuerblock is the rollinghash of the issuerbody, prepended to the issuerbody. The hash covers version, lengths, and fields.

issuerdata = encode(ISSUERROM1, encode(ISSUERROM2, issuerblock)) -- the issuer seals the shared signature with its private ROM, double-encoded.

The shared block body contains: version, length of sharedsignature, length of sharedclaims, sharedsignature, sharedclaims, and issuerdata (the last field has no length).

sharedblock = rollinghash(sharedbody) + sharedbody

zwt = encode(SHAREDROM, sharedblock)

`sharedsignature` and `issuersignature` are identical values.

`encode(ROM, ...)` is a reversible UNSIGNAL encoding. `decode(ROM, ...)` is its inverse. The relying party opens the shared block with SHAREDROM and reads the fields. The issuer additionally opens the issuer block with ISSUERROM1 and ISSUERROM2. The rolling hash is verified on open, binding every field.

`issuerblock` is the raw frame before double-encoding. `issuerdata` is the encoded bytes of that frame, carried as a field inside the shared block. `sharedblock` is the raw frame before encoding. The fully-encoded result is the ZWT.

---

## Wire Format

The token is a flat, versioned structure, UNSIGNAL-encoded -- readable on any target (Z80, 6502, C, C#, Python) with base-plus-offset arithmetic. All multi-byte integers are **little-endian**.

A ZWT is a single token: a shared block (opened with SHAREDROM) whose `issuerdata` field is itself an issuer block (opened with ISSUERROM1 and ISSUERROM2).

Both blocks have the same shape: a header (rolling-hash CRC, version, a 2-byte length for every field except the last), then the fields. The last field in each block has no length entry -- it runs from where the previous field ends to the end of the block.

### Shared Block (UNSIGNALed with SHAREDROM)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | rolling hash (CRC) |
| 4 | 2 | version (= 0) |
| 6 | 2 | length of sharedsignature (LE) |
| 8 | 2 | length of sharedclaims (LE) |
| 10 | .. | sharedsignature |
| .. | .. | sharedclaims |
| .. | .. | issuerdata (double UNSIGNALed with ISSUERROM1 and ISSUERROM2) |

### Issuer Block (Double UNSIGNALed with ISSUERROM1 and ISSUERROM2)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | rolling hash (CRC) |
| 4 | 2 | version (= 0) |
| 6 | 2 | length of issuersignature (LE) |
| 8 | .. | issuersignature |
| .. | .. | privateclaims |

`issuerdata` has no length field -- it is the last field of the shared block, so it runs to the end of the token. `privateclaims` has no length field -- it is the last field of the issuer block, so it runs to the end of that block.

The issuer block is built and UNSIGNAL-encoded first, then carried verbatim as `issuerdata` inside the shared block. The relying party opens only the shared block; the issuer block nested inside stays opaque and is opened separately by the issuer.

Each block's rolling hash covers everything after its own 4-byte hash -- version, length table, and fields.

### Design Choices

- **No pointers, only lengths -- and the last field needs neither.** Fields are in fixed order. Each blob starts where the previous ended. The last field in a block extends to the end of the block. The only 16-bit limit is each *length* field, giving every segment its own 64 KB range while the whole frame may exceed 64 KB.
- **No field-count byte, no type tags, no field IDs.** The segment schema is fixed per version and known out of band. The RP reads `sharedsignature` + `sharedclaims` with SHAREDROM. The issuer reads `issuersignature` + `privateclaims` with ISSUERROM1 and ISSUERROM2.
- **Version byte for forward compatibility.** Version 0 fixes the segment list. Future versions may append segments without breaking version-0 readers. The version byte is inside the hash coverage and cannot be altered undetected.
- **Hash covers everything except itself.** The 4-byte rolling hash at offset 0 is computed over the version byte, the length table, and all blobs. Version and lengths are integrity-bound.
- **Double UNSIGNAL encoded issuer data.** `issuerdata` is double-encoded because the relying party has the plaintext `sharedsignature`. Double encoding removes any known-plaintext foothold on `privateclaims`.

`privateclaims` passes through UNSIGNAL three times (twice from the issuer block's double encoding, once more when the shared block is encoded). `sharedclaims` passes through once.

A typical token -- a 16-byte GUID `sharedsignature` with short claims -- is approximately **0.9-1.2 KB**.

---

## Verification

| Verifier | Checks | With |
|----------|--------|------|
| **Relying Party** | opens the ZWT and reads `sharedsignature` + shared claims | SHAREDROM |
| **Issuer** | `sharedsignature` matches the copy sealed in `issuersignature` | ISSUERROM1 and ISSUERROM2 |

---

## Updating Shared Claims

The relying party can update `sharedclaims` without re-issuing the entire token. This is useful for:
- Session state management
- Nonce rotation (ping-pong)
- Client-side claim updates that don't affect issuer validation

Updating shared claims creates a **new token** with the same `sharedsignature` and `issuerdata`. The old token remains valid with old claims -- the RP chooses which token to use.

### Security Properties

| What | Can Change | Why |
|------|------------|-----|
| `sharedclaims` | RP | RP's own view -- affects only its own door |
| `privateclaims` | Issuer only | Sealed with issuer ROMs -- never exposed to RP |
| `sharedsignature` | Issuer only | Binding to issuerdata -- changing it breaks introspection |
| `issuerdata` | Issuer only | Sealed with issuer ROMs -- RP can't read or modify |

---

## Ping-Pong (Challenge-Response)

The RP initiates a challenge-response protocol by updating the nonce in `sharedclaims`. The issuer validates the nonce and acts upon it once.

### Nonce and Challenge ID in Issuer Claims

The `privateclaims` contain both the nonce and a challenge identifier.

| Field | Purpose |
|-------|---------|
| **nonce** | One-time freshness marker -- prevents replay |
| **challenge** | Identifies which challenge/request this token belongs to |
| **user** (or other claims) | Actual identity/authorization data |

### Protocol Flow

1. Issuer -> RP: token (nonce: N1, challenge: C1 in both claims)
2. RP -> Issuer: token + updated sharedclaims (nonce: N2)
3. Issuer validates: N2 is fresh (not used before)
4. Issuer -> RP: new token (nonce: N2, challenge: C1 in both claims)
5. RP -> Issuer: token + updated sharedclaims (nonce: N3)
6. Issuer rejects: N3 already used

### State Tracking

The issuer tracks:
- **Used nonces** -- to prevent replay attacks
- **Challenge state** -- to track which challenges are pending or completed
- **Session state** -- to track the current nonce per session

### Workflow: FOB-CAR Double Ping-Pong

**NOTE** If both parties are trusted (the FOB and the CAR, then it isn't mandatory to even have an internal claim, that is an option)

**Step 0: FOB Initiates**
FOB sends request to CAR. Timer starts.

**Step 1a: CAR Sends Ping 1**
CAR issues token with nonce R1 in BOTH private and shared claims.
CAR sends token to FOB.

**Step 1b: FOB Sends Pong 1**
FOB updates shared claims -- nonce changes from R1 to R1 + 1.
Private claims still have R1. Shared claims now have R1 + 1.
FOB sends updated token back to CAR.

**Step 1c: CAR Validates Pong 1**
CAR introspects the token.
CAR reads private claims nonce = R1.
CAR reads shared claims nonce = R1 + 1.
CAR KNOWS shared = private + 1. Validation passes. FOB is authenticated.

**Step 2a: CAR Sends Ping 2 (MANDATORY)**
CAR issues a NEW token with nonce R2 in BOTH private and shared claims.
CAR sends token to FOB.

**Step 2b: FOB Sends Pong 2**
FOB updates shared claims -- nonce changes from R2 to R2 + 1.
Private claims still have R2. Shared claims now have R2 + 1.
FOB sends updated token back to CAR.

**Step 2c: CAR Validates Pong 2**
CAR introspects the token.
CAR reads private claims nonce = R2.
CAR reads shared claims nonce = R2 + 1.
CAR KNOWS shared = private + 1. Validation passes.

**Step 3: CAR Opens/Unlocks and Sends New Token to FOB**
CAR UNLOCKS THE DOOR.
CAR issues a NEW token with nonce R3 in BOTH private and shared claims.
CAR sends new token to FOB so the FOB can chirp again if needed.

**Timer stops. If Step 2c is not completed within the timeframe, CAR rejects the session.**


### Summary Table

| Step | Who | Action | Private Nonce | Shared Nonce | Validation |
|------|-----|--------|---------------|--------------|------------|
| 0 | FOB | Initiate (Timer starts) | -- | -- | -- |
| 1a | CAR | Send Ping 1 | R1 | R1 | -- |
| 1b | FOB | Send Pong 1 | R1 | R1 + 1 | -- |
| 1c | CAR | Validate Pong 1 | R1 | R1 + 1 | shared = private + 1 PASS |
| 2a | CAR | Send Ping 2 | R2 | R2 | -- |
| 2b | FOB | Send Pong 2 | R2 | R2 + 1 | -- |
| 2c | CAR | Validate Pong 2 | R2 | R2 + 1 | shared = private + 1 PASS |
| 3 | CAR | Open/Unlock + Send new token | R3 | R3 | -- |

### Rule

The CAR always issues a new random nonce.
The FOB always returns nonce + 1.
The CAR always validates that shared = private + expected increment.

Two complete rounds are mandatory:
- Round 1: FOB chirps, CAR responds (authenticates FOB)
- Round 2: CAR chirps, FOB responds (verifies proximity, stops relay)

The token is never modified. It is replaced with a new token after each exchange.

### Why This Works

- RP can update `sharedclaims.nonce` (it is in the shared envelope)
- Issuer validates the new nonce is fresh (not used before)
- Issuer updates `privateclaims.nonce` to match
- Challenge ID tracks which request this belongs to
- The binding between sharedsig and issuerdata is preserved
- Replay attacks are prevented -- used nonces are rejected

---

## Car Relay and Replay Attack Prevention

The double ping-pong challenge-response is **mandatory** to stop relay attacks. Replay is prevented by nonce progression. Relay is prevented by requiring two rapid alternating exchanges within a tight time window.

### Attack Vectors

| Attack | Description | Prevention |
|--------|-------------|------------|
| **Replay** | Attacker captures and resends an old valid message | Nonce progression -- each nonce used once |
| **Relay** | Attacker forwards messages between FOB and CAR in real-time | Double ping-pong + timing/proximity bound |
| **MITM** | Attacker intercepts and modifies messages | ROM binding prevents forgery |

### Why Single Ping-Pong Is Not Enough

Single ping-pong prevents replay because a captured nonce cannot be reused.

However, relay still works. An attacker forwards messages between FOB and CAR in real-time. The attacker passes the challenge from CAR to FOB and the response from FOB to CAR. The CAR accepts because the response is valid.

### Double Ping-Pong (Relay Prevention)

Round 1 (Authentication):
- FOB -> CAR: NONCE=1 (initiate)
- CAR -> FOB: NONCE=2 (ping 1)
- FOB -> CAR: NONCE=3 (pong 1)

Round 2 (Proximity Verification -- MANDATORY):
- CAR -> FOB: NONCE=4 (ping 2)
- FOB -> CAR: NONCE=5 (pong 2)

### Why Double Ping-Pong Stops Relay

Round 1 authenticates the FOB. The CAR accepts.

Round 2 requires a fast exchange. The relay adds measurable latency. If the second response arrives after the expected time window, the CAR rejects the session.

### Required Timing Window

| Parameter | Requirement | Why |
|-----------|-------------|-----|
| **Proximity Window** | Tight (e.g., < 50ms) | Physical proximity requirement |
| **Round 1 -> Round 2** | Rapid exchange | Relay introduces measurable delay |
| **Both Rounds Required** | 2 rounds | Single round does not detect relay |

### Security Properties

| Attack | Prevention | Mechanism |
|--------|------------|-----------|
| **Replay** | Yes | Nonce progression -- each nonce used once |
| **Relay** | Yes | Double ping-pong + timing/proximity bound |
| **MITM** | Yes | ROM binding prevents forgery |
| **Outsider** | Yes | Cannot forge without SHAREDROM + ISSUERROM |
| **Compromised RP** | Yes | Can only affect its own door |

### Summary

- Single Ping-Pong -> Prevents REPLAY (nonce used once)
- Double Ping-Pong -> Prevents RELAY (timing + proximity)
- ROM Binding -> Prevents FORGERY (needs both ROMs)
- Paired CAR-FOB -> Prevents SPOOFING (only paired works)

**The double ping-pong is mandatory for relay protection. Single ping-pong is not enough.**

---

## Multi-Server Issuer Claims

Since `privateclaims` are sealed inside the issuer block with ISSUERROM1 and ISSUERROM2, **only the issuer's servers** can read them. This enables a trusted multi-server architecture.

### Architecture

All issuer servers share ISSUERROM1 and ISSUERROM2. All servers share SHAREDROM. The relying party has only SHAREDROM.

### Claim Types

| Claim Type | Location | Readable By | Contains |
|------------|----------|-------------|----------|
| `sharedclaims` | Shared block | Issuer + RP | Public claims, nonces, state |
| `privateclaims` | Issuer block | Issuer only | User ID, roles, permissions, secrets, nonce, challenge |

### Multi-Server Validation

Any issuer server with the ROMs can introspect a token. `privateclaims` remain consistent because all servers share the same ROMs.

### Shared State

The issuer servers must share state to prevent replay attacks:
- Used nonces (prevent replay across servers)
- Challenge state (shared across servers)
- Session state (shared across servers)

### Server Failover

If one issuer server fails, another can take over. The `privateclaims` remain consistent because all servers share the same ROMs. The shared state must be replicated or stored in a shared database.

---

## Comparison: Shared vs Private Claims

| Aspect | Shared Claims | Private Claims |
|--------|---------------|----------------|
| **Readable by RP** | Yes | No |
| **Readable by Issuer** | Yes | Yes |
| **Modifiable by RP** | Yes (UpdateSharedClaims) | No |
| **Modifiable by Issuer** | Yes (Issue new token) | Yes (Issue new token) |
| **Sealed with** | SHAREDROM | ISSUERROM1 + ISSUERROM2 |
| **Typical contents** | Scope, state, nonce | User ID, roles, permissions, nonce, challenge |
| **Use case** | RP-local state | Identity verification, challenge tracking |

---

## Summary: Trust Model

- **SHAREDROM** -- Trusted by RP + Issuer
  - RP can decode shared block
  - RP can update shared claims
  - RP cannot read or modify private claims

- **ISSUERROM1 + ISSUERROM2** -- Trusted by Issuer only
  - Only issuer servers have these
  - Seals private claims
  - Prevents RP forgery

- **Shared Signature (GUID)** -- Binding between layers
  - Must match in both shared and issuer blocks
  - Cannot be forged without issuer ROMs

- **Nonce** -- One-time freshness marker
  - Stored in both shared and private claims
  - RP can update shared copy
  - Issuer validates and updates private copy
  - Used nonces are rejected

- **Challenge ID** -- Request/state tracker
  - Stored in private claims only
  - Links token to specific challenge
  - Issuer tracks challenge state
  - Prevents re-use of completed challenges

---

## Why Nobody but the Issuer Can Forge a Valid Token

- A forger without SHAREDROM can't open or produce a ZWT -- outsiders locked out.
- A relying party holds SHAREDROM, so it can produce a `sharedsignature`, but it cannot produce a matching `issuersignature` -- that requires ISSUERROM1 and ISSUERROM2 (issuer-only).
- A `sharedsignature` is only valid when it matches the copy sealed inside `issuersignature`.
- Therefore only the issuer can produce a valid token. A forged shared-sig has no matching sealed copy and fails.

**Even a fully compromised relying party cannot mint a token the issuer will accept.**

---

## Properties

| Property | How |
|----------|-----|
| **Opaque** | Whole ZWT is UNSIGNAL-encoded; payload, signatures, and structure are indistinguishable from noise (I(M;A)=0) |
| **Concealed verification structure** | An observer can't tell how many signatures exist, which keys govern them, or where they are |
| **Cross-site inert** | A ZWT is unvalidatable by any party without the relationship key -- no `aud` check needed; misuse is structural, not just forbidden |
| **Quantum-proof** | No asymmetric primitive; nothing for Shor's algorithm to attack |
| **Unforgeable** | Valid tokens require the issuer's private ROM pair (via the sealed-shared-sig binding) |

---

## Not Solved by ZWT Alone

| Gap | Note |
|-----|------|
| **Replay to the legitimate relying party** | A stolen ZWT can be replayed to its intended recipient. Bind a server-issued nonce inside the token; avoid clock-based expiry (clocks are attacker-influenceable). |
| **Revocation** | Stateless local verification can't revoke mid-life. If needed, verify `issuersignature` via issuer introspection instead -- gains revocation, costs a round-trip. |

---

## Notes

- `sharedclaims` are readable by the relying party (opened with SHAREDROM). `privateclaims` are sealed inside `issuerdata` and readable only by the issuer.
- SHAREDROM is per-relationship, so a relying party forging a shared signature could only ever affect its own door -- a non-event -- and the binding to `issuersignature` prevents even that from producing an issuer-valid token.