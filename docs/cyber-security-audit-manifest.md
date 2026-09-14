# CYBER SECURITY AUDIT MANIFEST

### Draft v0.4 — Layered Organisational Audit Structure

---

## Purpose

**Audience:** businesses with software that carry compliance requirements, and the people auditing them — internal audit/security teams, external auditors filling a delegated form, or a founder doing it themselves for a small organisation.

**Scope:** this document verifies that due process was followed — that the right things were checked, recorded, and disclosed, for every layer that applies. It does not assess or mitigate risk, and it deliberately makes no attempt to weigh findings against each other or tell you what to fix first. Whether a given finding matters, and what to do about it, is a judgement call for a separate document that consumes this one's output — see the closing note at the end of this manifest.

This version restructures the manifest around **audit layers**. Every piece of software in an organisation gets sorted into a layer first, then audited to the depth that layer requires. Nothing gets the full technical treatment unless it needs it — but nothing gets skipped either, because even "no security required" software still needs a license-compliance entry.

The document has three main sections plus an appendix:

- **Section 1 — Audit Layers** — the layer framework itself, Layers 0 through 7.
- **Section 2 — Trust Levels** — the ZOSCII Foundation Trust Levels framework, recorded against every Layer 4/5 entry. A cross-cutting attribute, not a layer of its own.
- **Section 3 — Dedicated / Embedded Computing Devices** — routers, IoT, security systems, audited against Section 1's layers to the degree possible, plus hardware-specific fields.
- **Appendix A** — the information-theoretic audit module for ZOSCII/UNSIGNAL-class concealment systems, invoked from Section 1 where relevant rather than run against everything.

Section numbers (1, 2, 3) and layer numbers (Layer 0–7) are deliberately separate things. A layer's name never changes based on where it sits in the document — refer to "Layer 4" as "Layer 4" throughout, and use the §-numbers (§1.5, §1.5.1, etc.) only for pointing at a specific spot in the text.

The layers, low to high:

- **Layer 0** — No security authentication required. License compliance only.
- **Layer 1** — Basic authentication (users/logins exist).
- **Layer 2** — Enhanced authentication (profiles, permissions, fine-grained control).
- **Layer 3** — Compliance-required software (industry/government mandated).
- **Layer 4** — In-house software, full technical security breakdown.
- **Layer 5** — 3rd-party software, observable-only audit + optional delegated form.
- **Layer 6** — Reserved.
- **Layer 7** — Trustworthiness of the compliance organisations and governments named in Layer 3.

A single product can sit in more than one layer at once (e.g. an in-house payroll system is Layer 2 + Layer 3 + Layer 4). Assign every layer that applies, audit each once, don't duplicate the fields that overlap.

The original information-theoretic audit content (observability, inference, distinguishability, I(M;A)=0) is retained as **Appendix A**, and is invoked specifically from Layer 4 and Layer 5 wherever a system's job is data concealment or covert communication (ZOSCII/UNSIGNAL-class systems) — it is not run against every piece of software, only against systems making that kind of claim.

## Re-Audit Cadence and Decommissioning

This document is a point-in-time record. Without a re-audit trigger it goes stale and gets filed away — treat it as a recurring exercise, not a one-off. A sensible default is annually, or on any major version/architecture change to a system, whichever comes first; some layers warrant tighter cadence in practice (Layer 4 cryptographic/mechanism fields alongside every release, Layer 7's trust research at least annually or on a relevant news event) but the document doesn't mandate a schedule — set one appropriate to the organisation and keep it.

**Decommissioning check, run at every re-audit:** compare the current inventory against the previous audit's. Anything that appeared last time and is absent this time is either genuinely decommissioned, or it fell off the radar without due process being followed — both cases need chasing down, not a quiet deletion of the row. Where a system genuinely was decommissioned since the last audit, verify explicitly: have the credentials, keys, and data associated with it been destroyed, migrated, or otherwise secured as required — not simply left sitting, accessible, on a system nobody is watching anymore.

**Decommissioned means genuinely retired — no access at all.** A system kept around for read-only access, occasional lookups, or intermittent use is not decommissioned; it is still a live system and gets audited at full depth like anything else in active use. Don't let "we don't really use it anymore" substitute for an actual decommissioning check.

---

# Section 1 — Audit Layers

## 1.1 Layer 0 — No Security Authentication Required (License Compliance Only)

Software with no login, no user concept, no data classification concern — a local utility, a compiler, a font pack, a codec. Nothing to audit for security. Still gets logged, because license non-compliance is a legal and financial exposure regardless of security posture.

**Fields per item:**

| Field | Notes |
|---|---|
| Software name | |
| Vendor | |
| License type | Freeware / Open source (specify licence, e.g. MIT, GPL) / Perpetual / Subscription / Volume |
| Renewal or expiry date | N/A if perpetual |
| Reason for Layer 0 classification | Why this genuinely has no auth/security surface — state it, don't assume it |
| Seats / install count vs licensed count | Flag any over-deployment |
| Owner / contact | Who's responsible for renewal |

---

## 1.2 Layer 1 — Basic Authentication

Software with the *concept* of users and logins — the Windows-account model. No fine-grained permission system, or a flat one (admin vs everyone).

**Fields per item:**

| Field | Notes |
|---|---|
| Software name / vendor | |
| Is authentication optional or mandatory | Can it be run without logging in? |
| Authentication method | Password / SSO / MFA / other |
| Credential storage | Local, directory-integrated (AD/LDAP/Entra), vendor cloud |
| Shared or individual accounts | Flag shared-login use — common compliance failure |
| Provenance | In-house developed / 3rd-party developed on consignment (built for you, you may hold source) / 3rd-party off-the-shelf (OTS) / 3rd-party SaaS |
| If 3rd party: source code access | Yes/No — determines whether this can ever be promoted to Layer 4 depth or stays Layer 5 |

---

## 1.3 Layer 2 — Enhanced Authentication

Everything in Layer 1, plus fine-grained user control — profiles, roles, permissions, scoped access.

**Additional fields on top of Layer 1:**

| Field | Notes |
|---|---|
| Permission model | RBAC / ACL / custom / none formalised |
| Granularity | Per-module, per-record, per-field |
| Who can grant/revoke access | Named role, not a person |
| Permission-change logging | Is a permission change itself audited/logged |
| Privilege escalation path | How does a user get from standard to admin, and is that logged |
| Orphaned-account process | What happens to access when someone leaves |

---

## 1.4 Layer 3 — Compliance-Required Software

Software required by law, industry standard, or contract to meet a specific compliance regime — regardless of what layer its authentication sits at.

**Fields per item:**

| Field | Notes |
|---|---|
| Applicable regime | PCI-DSS, GDPR, Privacy Act 1988 (AU), ISO 27001, HIPAA, industry-specific, contractual clause, etc. — name it specifically |
| Penalty exposure | Penalised / Not penalised, and by whom (regulator, auditor, contract counterparty) |
| Penalty description | Fine, licence loss, contract termination, criminal liability — be specific |
| Compliance assessment/renewal date | Next audit or attestation due |
| Last assessment result | Pass / conditional / fail, date |
| Responsible internal contact | |
| External contact | Regulator, auditor, or certifying body contact |
| Evidence location | Where the compliance evidence pack lives |

### 1.4.1 Logging Compliance

Compliance regimes that require audit trails often specify a *level* of logging, not just "logging exists". Record which level this system actually provides, against which level the regime requires.

| Field | Notes |
|---|---|
| Data access logging | Is every read/view of a record logged, not just writes |
| Record change logging | Is every write logged: who, when, what field |
| All-changes logging | Does this cover every table/entity in scope, or only some |
| Historical data traversal | Can you reconstruct the record (or the form) as it existed at a given past point in time — not just that a change happened |
| Field-level history | Can you see the value of a specific field before and after a change, or only that "the record changed" |
| Structural change history | If a field was added or removed from the form/schema itself, can you see what the form looked like before vs after — most software cannot do this even when it logs data changes |
| Required level (per regime) | What the applicable compliance regime actually mandates |
| Gap | Where actual logging falls short of required logging |
| Key/credential capture in logs | Are passwords, keys, tokens, or secrets ever written to log output — plaintext or otherwise. Check request/response logs, error logs, crash dumps, debug logs, and third-party logging/telemetry libraries, not just the application's own log statements. This should never happen; if found, it's a finding, not a note, regardless of whether any regime specifically names it |

**Note:** some government/industry regimes require full audit history equivalent to physically traversing a paper filing cabinet — every prior state, retrievable. Most software logs that a change occurred but not what the record or form looked like before/after, especially for structural changes (a field added or removed). Very few systems support true point-in-time reconstruction. Where the regime requires it and the system can't do it, this is a compliance breach, not a minor gap — flag it as a finding, not a note.

---

## 1.5 Layer 4 — In-House Software, Full Technical Security Breakdown

For anything built and controlled internally. This is the deep layer — security is audited at the technical level, and technical is **not limited to cryptography**. Break the audit into the categories below; not every system will have entries in every category, but every category must be explicitly checked, not assumed absent.

### 1.5.1 Cryptographic
- Where the system uses actual encryption: algorithm, key length, at rest / in transit
- Key/ROM management: generation, storage, rotation, destruction — applies whether the underlying mechanism is encryption or not
- Where the system is a ZOSCII/UNSIGNAL-class concealment system, this is cryptographic but is **not encryption** — nothing is enciphered, there is no ciphertext, and the audit must not describe it as one. Its job is that protected information not be observable, not that it be unreadable. **Invoke Appendix A in full** rather than treating it as a standard encryption audit
- **Trust level (§2):** which ZOSCII Foundation trust level (0–6) this system's data flows sit at. Record it here, don't leave it as an implicit assumption — see Section 2
- **Security-layer swap difficulty:** if the encryption/obfuscation/ITS mechanism in use were found broken, deprecated, or otherwise needed replacing, how hard is that to actually do — see §1.5.8 rating scale
- **Security mechanism strength:** Simple / Mathematically Hardened / Quantum Resistant / Quantum Proof — see §1.5.9 rating scale
- **Transport layer independence:** if this system communicates over a public network/internet, is it just as secure over HTTP as HTTPS — see §1.5.10 rating scale

### 1.5.2 Obfuscation
- Code obfuscation (binary, script) — what's obfuscated, from whom, and why (IP protection vs security is a different claim — don't conflate)
- Data obfuscation short of encryption (masking, tokenisation)

### 1.5.3 Physical
Not an afterthought category — explicitly check each:
- Physical media in use: USB sticks, external HDD, tape, floppy disk, microfiche, optical media
- Physical key locks (server racks, cabinets, safes)
- Air-gapping — any systems deliberately offline
- Physical access control to the machines themselves (badge, biometric, sign-in log)
- Media disposal/destruction procedure (degaussing, shredding, wiping standard used)
- Chain-of-custody procedure for physical media that leaves the building

### 1.5.4 Procedural
- Secure SDLC — code review requirement, who reviews, is it enforced or optional
- Source code access control — who can commit, who can merge, who can deploy
- Change control / release approval process
- Secrets management in the codebase (are credentials ever committed, is there a scanner)

### 1.5.5 Network
- Segmentation — is this system isolated from general network traffic
- Firewall rules specific to this system
- Exposed ports/services, and justification for each

### 1.5.6 Logging / Monitoring
- What's logged, retention period, who reviews it
- Alerting on anomalous access

### 1.5.7 3rd-Party Dependencies (Linked Entries)

In-house software is rarely built from nothing — it pulls in libraries, packages, and SDKs (NuGet, npm, etc.). Each dependency where security matters gets its own linked audit entry, separate from the parent software's entry, not folded into it. This applies even to Julian's own published packages (e.g. CyborgUnicorn.UNINTELLIGENCE) when consumed by an in-house system — audit the dependency the same as any other.

**Fields per dependency:**

| Field | Notes |
|---|---|
| Package name / identifier | Include registry (NuGet/npm/etc.), package ID, version pinned |
| Publisher | |
| Parent software (link) | Which in-house system(s) consume this — one dependency entry can link to several parents |
| Functional purpose | What it actually does for the parent system |
| Network activity | Does it make outbound calls on its own — endpoints contacted, and whether that's documented or observed |
| Credential/secret handling | Does it capture, store, log, or transmit credentials, keys, or tokens — check this explicitly, don't assume "no" |
| Uncontrolled/unwanted network transmission | Telemetry, crash reporting, analytics, phone-home behaviour not required for function and not fully controllable by you — e.g. OS-level memory dumps sent to the vendor (Windows Error Reporting sending crash dumps to Microsoft is the standing example: dumps can contain in-memory secrets, and transmission is largely outside the application's control) |
| License | And whether it's compatible with how the parent system is distributed/used |
| Version currency / known CVEs | Is it current, is there a patching process for it specifically |
| Source availability | Open source (auditable) vs closed/binary-only (Layer 5 observable-only treatment applies to this specific dependency even though the parent system is Layer 4) |
| Security-layer swap difficulty | Impossible / Small change / Project — see §1.5.8. A closed-source dependency doing its own encryption/obfuscation/ITS is automatically **Impossible** by definition — no source means no swap |
| Security mechanism strength | Simple / Mathematically Hardened / Quantum Resistant / Quantum Proof — see §1.5.9, if this dependency implements the security mechanism itself |
| Transport layer independence | Transport-Dependent / Transport-Assisted / Transport-Independent — see §1.5.10, if this dependency communicates over a network on its own account |

Where a dependency is closed-source, audit it the way Layer 5 audits a 3rd-party product — observable behaviour only — even though the system it's embedded in is otherwise a Layer 4 in-house audit. The dependency doesn't inherit the parent's audit depth just because the parent is in-house.

### 1.5.8 Security-Layer Swap Difficulty

Every entry that names an encryption algorithm, obfuscation method, or ITS mechanism (§1.5.1, §1.5.2, §1.5.7) also gets a rating for how hard it would be to swap that mechanism out — for a newer algorithm, a fix, or a different approach entirely, if the current one is ever found broken, deprecated, or otherwise no longer trusted. This is a real operational question, not a hypothetical one: a compromised mechanism you can't replace is a standing risk, not a resolved one.

**Rating scale:**

| Rating | Meaning |
|---|---|
| **Impossible** | No source access — closed-source dependency or vendor product. The mechanism can't be swapped without dropping the whole component and finding a replacement, because there's no way to point the existing integration at a different mechanism |
| **Small change** | Source is available and the mechanism is already abstracted behind an interface or plugin point — swapping means implementing/dropping in a new provider, not touching the calling code |
| **Project** | Source is available but the mechanism is hard-coded through the codebase, not abstracted — swapping requires a dedicated piece of work, with its own scope, testing, and migration risk |

**Design note (applies generally, not just to any one package):** a plugin-registry pattern — where the calling code talks to an interface and concrete algorithm implementations are loaded as separate, swappable components — is what turns an otherwise "project"-rated swap into a "small change" one. Rate the actual architecture, not the algorithm's reputation: a strong, currently-trusted algorithm hard-wired through the codebase still rates as a "project" to change later, because the difficulty is about the coupling, not about whether a swap is needed today.

### 1.5.9 Security Mechanism Strength Rating

A separate axis from §1.5.8. Swap difficulty rates how hard it is to *change* the mechanism; this rates how strong the mechanism *is*, on its own terms. Rate every named encryption, obfuscation, or ITS mechanism against this scale — don't rate the product or vendor, rate the specific mechanism in use.

| Rating | Meaning |
|---|---|
| **Simple** | No formal security proof, or a mechanism with known practical breaks — XOR, basic obfuscation, legacy ciphers (DES, RC2), anything relying on the method not being known rather than on the method holding up once known |
| **Mathematically Hardened** | Standard modern computationally-secure cryptography — AES-256, RSA-4096, ECC, current hash functions. Secure against classical computers under present-day mathematical knowledge, but the security rests on a hardness assumption (factoring, discrete log, etc.) holding — not on proof that it can't be broken |
| **Quantum Resistant** | Designed specifically to resist known quantum algorithms (Shor's, Grover's) — post-quantum lattice-based schemes and similar. Still a computational hardness assumption, just a different one chosen because the current quantum attacks don't apply to it. This is a patch to the assumption, not a removal of the assumption — a stronger future attack (quantum or classical) against the new hard problem is not ruled out, only not yet known |
| **Quantum Proof** | No computational hardness assumption at all — information-theoretic security, I(M;A)=0. There is no mathematical relationship between protected information and what's observable for a future computer of any kind to eventually solve. This is the ZOSCII/UNSIGNAL-class rating — not "not yet broken," but nothing there to break. Reserve this rating for mechanisms that actually meet the Appendix A standard; don't award it on a vendor's unqualified marketing claim |

**Distinguishing "resistant" from "proof" matters for Harvest-Now-Decrypt-Later exposure:** anything rated Mathematically Hardened or Quantum Resistant is still, in principle, a recorded artefact today that a sufficiently advanced future attack could retroactively break — the assumption may hold for decades, but it is an assumption, not an absence. Only Quantum Proof mechanisms have nothing in the recorded artefact for a future attack to work against. For data with a long confidentiality requirement (medical, legal, state records), this distinction is the actual audit-relevant fact — not which specific algorithm is fashionable this year.

### 1.5.10 Transport Layer Independence

For any system that communicates over a public network or the internet. The test: **strip away HTTPS (or whatever transport encryption is in use) and ask whether the system is still just as secure.** If the answer is no, the system's actual security depends on the transport layer, not on anything the system itself provides — and transport layer security is a separate party's job (the CA, the TLS implementation, the network path), not the application's own property.

| Rating | Meaning |
|---|---|
| **Transport-Dependent** | The application has no security of its own — confidentiality and integrity rely entirely on TLS/HTTPS. Run it over plain HTTP and the payload is fully exposed, no different from any other unencrypted traffic. This is the default and most common case; state it plainly when it applies rather than letting "we use HTTPS" stand in as an answer to "is the software secure" |
| **Transport-Assisted** | The application has some payload-level protection (e.g. field-level encryption, a signed token) but not complete coverage — some data or metadata still depends on the transport layer for protection. Record exactly what's covered by the application layer and what isn't |
| **Transport-Independent** | The application's security holds regardless of the transport — HTTP and HTTPS are equally secure for this system's actual payload, because the security lives in the payload itself, not the pipe. TLS may still be layered on for defense-in-depth, but it is not load-bearing for the core confidentiality claim. This is the ZOSCII/UNSIGNAL microZOSCII-bootstrap case named in the ZOSCII Foundation's own material — the claim there is explicit: plain HTTP is stated as equally secure to HTTPS for that mechanism, which is exactly the kind of claim this field exists to test, not assume |

**Audit note:** don't accept "it uses HTTPS" as a completed answer to this field — HTTPS answers a different question (is the pipe encrypted) from the one this field asks (is the application's own security equivalent with or without that pipe). A system can genuinely need HTTPS and still be Transport-Dependent; that's not a failure, it's an honest rating. The finding to flag is a system claiming strong security while actually being Transport-Dependent and not disclosing that dependency.

---

## 1.6 Layer 5 — 3rd-Party Software, Observable-Only Audit

For software where you don't hold the source and can't inspect the implementation directly. Audit what's observable from outside the vendor relationship.

**Fields per item:**

| Field | Notes |
|---|---|
| Provenance | Consignment-built (bespoke for you, no source held) / OTS / SaaS |
| Vendor certifications held | SOC 2, ISO 27001, PCI-DSS attestation, etc. — request evidence, don't take the claim unverified |
| Independent pen-test evidence | Available Y/N, date, summary |
| Observed network behaviour | Ports, endpoints contacted, telemetry sent, from your own traffic capture where possible |
| Data handling clause in contract | What the contract actually commits the vendor to, not what the marketing says |
| Incident history | Any known breaches, disclosed or discovered |
| Support / end-of-life status | Is the product still patched |
| Data residency | Where your data physically sits |
| Trust level (§2) | Which ZOSCII Foundation trust level (0–6) this product's data flows sit at, from what's observable |
| Security-layer swap difficulty | Impossible / Small change / Project — see §1.5.8. Without source, this is **Impossible** by default unless the vendor documents an abstracted, swappable security layer (e.g. a plugin/provider model) |
| Security mechanism strength | Simple / Mathematically Hardened / Quantum Resistant / Quantum Proof — see §1.5.9, from vendor disclosure or observable behaviour; don't accept an unqualified "quantum proof" marketing claim without checking it against the Appendix A standard |
| Transport layer independence | Transport-Dependent / Transport-Assisted / Transport-Independent — see §1.5.10, from vendor disclosure or your own traffic capture with and without TLS where testable |

### 1.6.1 Delegated Audit Option

Where the 3rd party's product performs a function that would warrant Layer 4 depth if it were in-house — most relevantly, any product making a data-concealment or covert-communication claim — issue the vendor a **Delegated Cyber Security Audit Manifest**: a cut-down version of this document containing only the Layer 4 section (§1.5.1–§1.5.6) plus Appendix A where applicable, for the vendor to complete about their own product. Track return status per vendor; a vendor's refusal or non-response to a delegated form is itself an audit finding, not a dead end.

### 1.6.2 Dependency Disclosure Request

Include §1.5.7 (3rd-Party Dependencies) in the delegated form. A vendor's product has its own dependency chain, same as an in-house system does — request it, don't assume it doesn't matter because you didn't build the product. At minimum, request confirmation of the same points §1.5.7 asks for: outbound network activity, credential/secret handling, and any uncontrolled telemetry or crash-dump transmission (e.g. platform-level reporting that leaves the vendor's own control, such as Windows Error Reporting).

---

## 1.7 Layer 6 — Reserved

Not yet defined. Numbering kept intact rather than renumbering Layer 7 down, so future additions don't collide with anything already cross-referenced.

---

## 1.8 Layer 7 — Trustworthiness of Compliance Organisations and Governments

Layer 3 records which regulators, standards bodies, and governments an organisation is compliant with or required to answer to. Layer 7 asks the question Layer 3 doesn't: **are they trustworthy, based on their own past behaviour** — not their stated mandate, not their marketing, not the first page of search results.

This layer is a neutral, general-purpose check — it is not aimed at any one body, and it applies to Cyborg Unicorn / the ZOSCII Foundation exactly as it applies to any regulator or standards organisation named in Layer 3. If it's not fair to hold a compliance body to a standard, it's not fair to hold your own organisation to it either — the point of Layer 7 is to hold everyone to the same one.

This is not a governance-theory exercise. If a body demanding audit access, data handovers, or compliance evidence has a documented history of misusing exactly that kind of access, that's an audit finding about your exposure, same as an unpatched dependency is.

### 1.8.1 Research Standard

- Don't stop at a search engine's top results — those are ranked for relevance and recency, not accuracy, and are frequently the body's own press releases or friendly coverage.
- Read the actual news reporting, not just headlines or summaries. Go to the source article, not an aggregator's rewrite of it.
- Cross-reference multiple independent outlets, not multiple outlets citing the same wire story.
- Distinguish **allegation** (reported, unproven) from **proven** (court finding, official inquiry conclusion) from **admitted** (the body itself acknowledged it) — record which one each finding is, don't flatten them into "there were issues".
- Note the date of each incident and whether it's a one-off or a pattern over time — a single incident 20 years ago under different leadership is a different finding to a recurring pattern under the current structure.
- Note who's reporting it — an outlet with a documented axe to grind on this specific body is still evidence, just weigh it as such; don't discard it, and don't take it as settled either.

### 1.8.2 Example Bodies to Research

Not findings — a prompt list. The manifest doesn't pre-load conclusions about any of these; whoever runs this layer does the §1.8.1 research themselves, against whatever is current at the time, and reaches their own evidence-classified result. Listing a body here is not an allegation against it — it's exactly what §1.8.1 says: don't take a name's reputation as settled either way, go and check.

- **NIST** (US National Institute of Standards and Technology) — issues cryptographic and technical standards many organisations comply with by reference.
- **ISO** (International Organization for Standardization) — issues broad international standards (9001, 27001, etc.) commonly required in Layer 3 entries.
- **OWASP** — issues widely-adopted application security standards and guidance (Top 10, ASVS, SAMM).
- **ZOSCII Foundation / Cyborg Unicorn Pty Ltd** — held to the identical process. No conclusion pre-loaded here either — a short public track record is itself a data point to record (limited history ≠ clean history), not a finding of good or bad conduct.
- Any government agency, industry regulator, or certifying body actually named in your Layer 3 entries.

Run each through §1.8.1 independently, on your own timeline — the result of that research is the actual audit content for this section, not this list.

### 1.8.3 Fields per Body

| Field | Notes |
|---|---|
| Body name | Regulator, standards body, or government/agency |
| Jurisdiction | |
| Linked Layer 3 entries | Which compliance requirements trace back to this body |
| Stated mandate | What the body claims its role/authority is |
| Documented history — data handling | Past leaks, breaches, unauthorised sharing, or misuse of data it collected or was given access to |
| Documented history — enforcement conduct | Selective enforcement, regulatory capture findings, conflicts of interest, corruption findings |
| Documented history — overreach | Use of compliance/audit access for purposes beyond the stated mandate (surveillance, political use, competitive disadvantage to audited parties) |
| Evidence class per finding | Allegation / Proven / Admitted — per item above, not one rating for the whole body |
| Sources | Specific articles/inquiries/court records, not "widely reported" |
| Pattern assessment | Isolated incident vs recurring, and whether structural conditions that caused it are still in place |
| Practical effect on your posture | Given the above, what you disclose beyond the legal minimum, what you retain evidence of independently, and whether any data given to this body should be assumed no longer confidential regardless of its stated confidentiality terms |

### 1.8.4 Principle

Compliance with a body's requirements is not optional where the law makes it mandatory — Layer 7 doesn't argue for non-compliance. What it changes is how much you volunteer beyond the legal minimum, how carefully you document what you hand over and when, and whether you treat that body's confidentiality assurances as reliable or as merely their stated position. A body's own track record is evidence like any other in this manifest — audit it the same way you'd audit a vendor's security claims: don't take the claim, check the record.

**On repeat incidents:** once §1.8.1's research turns up a documented failure on a body's record — an admitted or independently proven case, not a bare allegation — continuing to adopt that body's output without additional independent scrutiny is a decision the adopting organisation made with the evidence already available to it. If the same body or the same failure mode recurs, that is not a fresh surprise; an organisation that keeps trusting a body on the strength of its mandate alone, after that body's own track record already showed the mandate isn't sufficient grounds for trust, is negligent in its own right — the failure isn't only the standards body's.

---

# Section 2 — Trust Levels

Every application audited under Layer 4 or Layer 5 also gets a **trust level** recorded against it — which level of trust its data flows actually operate at, per the ZOSCII Foundation Trust Levels framework. This is a summary here; the canonical, maintained reference is:

**https://github.com/PrimalNinja/cyborgzoscii-u/blob/master/trustlevels.md**

## 2.1 What "ZERO Trust" Means Here

ZOSCII's Level 0 "ZERO Trust" is not the industry "zero trust architecture" term. Industry zero-trust still trusts authentication servers, certificate authorities, and the assumption that the encryption in use holds. ZOSCII Level 0 means literally what it says: **trust no one and nothing except yourself** — no server, no third party, no central authority. A breached server holding only ZOSCII-encoded addresses discloses nothing, because there was never any data on it to disclose. Don't conflate the two "zero trust" terms in an audit — they describe different trust models with different guarantees, and the industry one is the weaker claim.

## 2.2 Levels, Summarised

| Level | Name | In short |
|---|---|---|
| 0 | ZERO Trust (Self-Sovereign) | Only you hold the ROM. No third party, no server, no exceptions |
| 1 | Selective Trust (Peer-to-Peer) | Key explicitly shared with chosen parties, no central authority |
| 2 | Organisational Trust | Company/entity-level key management, role-based access, internal system-to-system |
| 3 | Federated Trust | Multiple organisations, controlled interchange, each party keeps sovereignty over its own data |
| 4 | Public Verification | Public can verify (tamperproof timestamping/blockchain) but not decode |
| 5 | Foundation Governance | Specification stewardship — does not compromise Level 0 sovereignty; no-backdoors, no unauthorised ROM capture policy |
| 6 | Mixed Trust Levels | Composite/hybrid — different segments of the same workflow sit at different levels simultaneously |

Full principles, examples, and the no-ROM-capture policy detail per level are in the linked document — don't duplicate that content here, it will drift out of sync with the maintained source.

## 2.3 Recording Trust Level in the Audit

For every Layer 4 and Layer 5 entry, record which trust level (0–6) applies to that system's data flows — see the field already added at §1.5.1 and §1.6. Where a single application spans more than one level (the normal case per the framework's own composite examples — a whistleblower workflow, an OEM design pipeline, a freight system), record each segment and its level separately rather than assigning one level to the whole application. A system that claims Level 0 for one data segment while actually routing another segment through a server it doesn't control is not genuinely Level 0 for that second segment — audit each flow on its own merits, not on the application's overall marketing description of itself.

---

# Section 3 — Dedicated / Embedded Computing Devices

Routers, IoT devices, security systems (cameras, alarm panels, access controllers), and any other purpose-built or embedded computing device. Audit these against Section 1's layers and Section 2's trust levels to the degree possible — the same fields apply (authentication, compliance, technical breakdown, trust level, mechanism strength) — but "to the degree possible" is doing real work here: firmware is frequently closed, update channels are frequently opaque, and physical access to the device by parties outside your control is often the actual attack surface, not the network. Don't skip a field because the device makes it hard to answer; record "unknown, vendor won't disclose" as the finding, don't leave the row blank.

## 3.1 Provisioning Access

Who can put configuration, keys, credentials, or firmware onto this device — record all that apply, don't collapse to one:

| Field | Notes |
|---|---|
| Factory provisioning | Is anything (keys, default credentials, firmware) set at manufacture, by whom, and can you verify what was set — or are you trusting the manufacturer's word for it |
| In-house provisioning | Can your own organisation (re-)provision the device after receipt — what's the process, who's authorised, is it logged |
| Field/installer provisioning | Does a 3rd-party installer or field technician provision or reconfigure the device on-site — same questions as in-house: process, authorisation, logging |
| End-user provisioning | Can the end user (customer, employee, resident) change provisioning themselves — via app, web UI, physical controls |
| Remote/vendor provisioning | Can the manufacturer or vendor push configuration or firmware to the device after deployment without your organisation's direct action — this is a separate and often overlooked provisioning path in its own right |

## 3.2 Tamperproof Hardware

Not a yes/no field. Record either **"No"**, or a description of the actual mechanism — what specifically makes the hardware tamper-evident or tamper-resistant, not a marketing label. A vendor's claim of "tamperproof" or "secure element" with no mechanism description behind it is treated the same as "No" until the mechanism is actually known.

| Field | Notes |
|---|---|
| Tamper-resistance mechanism | Describe it: e.g. epoxy potting over the flash/key storage, a case-open switch wired to zero the key store, a secure enclave with hardware-enforced key isolation, a mesh layer over the die that fails closed if breached. If none, write "No" — don't leave blank and don't accept an undescribed claim as equivalent to a description |
| What it protects | Which specific component the mechanism covers — the whole board, just the key/credential store, just the flash chip. Tamper-resistance on one component doesn't imply it on others |
| Fails open or fails closed | On detected tampering, does the device wipe/lock (fails closed) or continue operating in a possibly-compromised state (fails open) |
| Verification | Has the mechanism been independently tested/defeated in public research, or is its effectiveness only the vendor's claim |

## 3.3 Additional Device-Specific Fields

| Field | Notes |
|---|---|
| Default credentials | Shipped with a default password — is it forced to change on first use, or can the device run indefinitely on the factory default |
| Firmware update signing | Are firmware updates cryptographically signed and verified by the device before install, or will it accept unsigned/unverified images |
| Update channel control | Who controls when and what gets pushed — you, or the vendor unilaterally; is there a way to decline or delay an update |
| Debug/service interfaces | JTAG, UART, serial console, or other service ports — present, and if so, are they disabled/locked in the shipped/deployed configuration or left open |
| Physical accessibility | Is the device physically reachable by people outside your organisation's control — a router in a customer's home, a camera on an external wall, an IoT sensor in a public or semi-public location — versus a device in a controlled server room |
| Network segmentation | Is the device on a segmented network isolated from the rest of the estate, or flat alongside everything else |
| Transport layer independence | Transport-Dependent / Transport-Assisted / Transport-Independent — see §1.5.10. Relevant to most devices in this section by default, since routers, IoT, and security systems commonly talk plain HTTP to local apps/dashboards even when the vendor's marketing emphasises HTTPS elsewhere in the product |

---

## Appendix A — Information-Theoretic Audit Module (ZOSCII / UNSIGNAL and similar concealment systems)

*Invoked from §1.5.1 and §1.6.1 wherever a system's security claim is concealment (not merely unreadability). Retained from Draft v0.1, condensed.*

**Core question:** what can an observer learn from the existence, structure, timing, representation, transmission, absence, or behaviour of a communication — not just from reading its payload.

**Confidentiality is checked at five levels, not one:**
1. Payload — can the content be recovered
2. Metadata — can properties of the communication be determined without recovering content
3. Semantic — can meaning/category/significance be inferred
4. Behavioural — can the communicating system's behaviour be inferred
5. Existence — can the observer determine that a communication or information state exists at all

**Standard for a concealment claim:** I(M;A) = 0 — zero mutual information between the protected message M and everything the adversary is permitted to observe A. This is the target, not an assumption; where it isn't met, quantify the leakage rather than asserting the system is secure.

**Audit against:**
- Distinguishability — can an observer classify the protected state better than chance from permitted observations alone
- Silence as signal — does absence, delay, or change in transmission pattern itself carry information
- Error channels — do decode failures, retries, or timing differences leak information
- Repetition/correlation — does the system stay concealed under multiple or correlated observations, not just a single one
- Implementation channel — timing, memory access, logging, caching, telemetry: the implementation is part of the channel, audit it as such

**Reject unqualified security claims.** "Secure", "untraceable", "zero leakage" etc. are not audit findings unless the answer to all of the following is on record: secure against whom, observing what, under what assumptions, for what information, over how many observations, with what auxiliary information permitted, against what measurable criterion.

**Classification of findings** (Class I–VII, direct disclosure through zero-information failure) and the evidence-preservation requirements from Draft v0.1 §18–19 carry forward unchanged.

---

## Deferred: Combining the Rating Axes

Section 1 now carries several independent rating axes against a single system — trust level (§2), swap difficulty (§1.5.8), mechanism strength (§1.5.9), transport independence (§1.5.10), evidence class (§1.8), tamper-resistance description (§3.2). Deliberately, this manifest does not combine them into a single score, and that omission was a decision, not an oversight.

**Why it's not simple.** Two systems can score very differently on these axes without that difference telling you what actually matters. A public marketing site's contact form might rate Simple on mechanism strength and Transport-Dependent on transport independence — and be entirely fine, because nothing sensitive runs through it. An internal payroll system might rate Mathematically Hardened and Transport-Independent — better on paper — while still being the far more consequential system if something goes wrong, because of what it holds, not how it's built. The axes describe the mechanism. They don't describe the consequence. Collapsing them into one number without also capturing what's actually at stake would produce a score that looks precise and isn't.

**Why it's out of scope here, not just deferred for difficulty.** As stated in the Purpose section: this manifest verifies that due process was followed — that the right checks were made and recorded. It is not a risk assessment, and doesn't attempt to rank findings by how much they matter. Combining the axes into a synthesised risk view is exactly that kind of judgement call, and belongs in a separate document that takes this manifest's completed audits as input.

**Planned: a Risk Mitigation document, as a follow-on.** The natural next document in this series takes the recorded facts from a completed Cyber Security Audit Manifest — the axes, the findings, the decommissioning checks — and does the work this one deliberately doesn't: weighs them against what's actually at stake for each system, prioritises what needs attention, and tracks remediation through to close. That's a different document with a different job, built on top of this one rather than folded into it.

---

## Status

**Document:** Cyber Security Audit Manifest
**Version:** 0.4 — Cadence, decommissioning, and scope clarification
**Change from v0.3:** Added a "Re-Audit Cadence and Decommissioning" section to the Purpose block — a default re-audit trigger, and a decommissioning check run at every re-audit comparing current inventory against the previous one, with an explicit carve-out that read-only/intermittent-access systems are not decommissioned. Added an Audience and Scope statement to Purpose distinguishing this manifest (due-process verification) from a future Risk Mitigation document (which would combine/weigh the recorded rating axes into actual risk decisions). Added a closing "Deferred: Combining the Rating Axes" section explaining why the multiple rating axes introduced in v0.2–v0.3 are deliberately not synthesised into one score here, with a worked example.