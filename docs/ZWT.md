# ZWT — ZOSCII Web Tokens

**Version 0.1 (DRAFT)**
**Author:** Julian Cassin

A quantum-proof, opaque session/attestation token. The JWT analogue for ZOSCII: an issuer (e.g. Google) attests a user to a relying party (e.g. Suno), but unlike JWT the token is information-theoretically opaque and its verification structure is concealed.

---

## Keys

| Key | Held by | Role |
|-----|---------|------|
| **SHAREDROM** | Issuer + Relying Party | Shared per-relationship key |
| **ISSUERROM** | Issuer only | Issuer's private ROM — never shared. One for all relying parties, or one per relying party |

---

## Construction

```
sharedsignature = a GUID or similar
issuersignature = encode(ISSUERROM, rollinghash(sharedsignature + privateclaims))   // issuer seals the shared-sig with its private ROM
zwt             = encode(SHAREDROM, rollinghash(sharedsignature + issuersignature + sharedclaims))
```

The `sharedsignature` is **bound inside** `issuersignature`: since `issuersignature = encode(ISSUERROM, rollinghash(sharedsignature + privateclaims))`, a valid `sharedsignature` is defined by matching the copy the issuer sealed — not by the shared key alone.

---

## Verification

| Verifier | Checks | With |
|----------|--------|------|
| **Relying Party** | can open the ZWT and read `sharedsignature` + shared claims | SHAREDROM |
| **Issuer** | `sharedsignature` matches the copy sealed in `issuersignature` | ISSUERROM |

---

## Why nobody but the issuer can forge a valid token

- A forger without SHAREDROM can't open or produce a ZWT — outsiders locked out.
- A relying party **holds** SHAREDROM, so it can produce *a* `sharedsignature` — but it **cannot** produce a matching `issuersignature`, because that requires ISSUERROM (issuer-only).
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
| **Unforgeable** | Valid tokens require the issuer's private ROM (via the sealed-shared-sig binding) |

---

## Not solved by ZWT alone

| Gap | Note |
|-----|------|
| **Replay to the legitimate relying party** | A stolen ZWT can be replayed *to its intended recipient*. Bind a server-issued nonce inside the token; avoid clock-based expiry (clocks are attacker-influenceable). |
| **Revocation** | Stateless local verification can't revoke mid-life. If needed, verify `issuersignature` via issuer introspection instead — gains revocation, costs a round-trip. |

---

## Notes

- `sharedclaims` are readable by the relying party (opened with SHAREDROM); `privateclaims` are sealed inside `issuersignature` and readable only by the issuer.
- SHAREDROM is per-relationship, so a relying party forging a *shared*-sig could only ever affect its own door — a non-event — and the binding to `issuersignature` prevents even that from producing an issuer-valid token.