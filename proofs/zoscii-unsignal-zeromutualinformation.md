# ZOSCII / UNSIGNAL — Zero Mutual Information

**Status:** formal specification
**Scope:** proves `I(M;A) = 0` for the ZOSCII primitive (Lemma 1, §5); identifies position as the sole channel prior knowledge can act on and structured-data misuse as its only exposure (§6); states the primitive's implementation properties — platform invariance, minimal trusted computing base, timing (§6a); shows UNSIGNAL closes the position channel (§7); shows what tier-2 constructions inherit — integrity and threshold sharing (§7a); and establishes that a sliding window over one ROM is separate ROMs for any bounded-span message (§8)

**This document proves. It does not specify.** The mechanism is defined elsewhere and is not restated:

| For | See |
|---|---|
| UNSIGNAL file layout, header, encode/decode steps | `unsignal-protocol.md` §2–4 |
| Attack-class enumeration and why each fails | `Finding Unknown Causation…` §1–13 |
| Intercept walkthrough, verification problem | `Cryptanalysis Walkthrough` |
| Measured entropy / chi-square behaviour | `Demonstration (ent only)` |
| Modes, masks, translation, merging | `ZOSCII-Modes.md` |
| Multi-ROM round-robin | `ZOSCII-Tango.md` |
| Nibble width and bootstrap | `microZOSCII.md` |
| Integrity inside the encoding | `ZOSCII-Rolling-Hash.md` |
| Threshold secret-sharing over the encoding | `pentagone.md` |

What is here and nowhere else: the exact condition under which `I(M;A)=0` holds (one line, §4), what the primitive does and does not claim across many messages (§6), how UNSIGNAL supplies the rest (§7), and the primitive's implementation properties — platform invariance and minimal trusted computing base (§6a).

### Three tiers

The ZOSCII ecosystem is three layers, and every claim in this document is about the first:

1. **The primitive — ZOSCII itself.** Blind selection over a ROM, `value = ROM[address]`, with a few compatibility parameters: **endianness, element size** (nibble / byte / word), **address size**. `I(M;A)=0` is proved here (§5) and holds for *every* setting of the parameters — they are compatibility knobs, not security knobs. This tier is fixed and provable.
2. **Protocols — constructions on the primitive.** UNSIGNAL (§7), Tango, masks, microZOSCII, the rolling hash. Customisable: anyone can build one. The *reference* protocols are vetted, and each either inherits the primitive's security by composition (as UNSIGNAL does, §7.1) or carries its own argument. This tier is open.
3. **Products — applications built with the toolkit.** Neptune and the like, using whatever primitive settings and protocols they choose. A product is not "ZOSCII"; it is built *with* ZOSCII. Product assurance is "it used vetted tier-1/tier-2 pieces correctly," not a mathematical claim, and is out of scope here.

The parameters at tier 1 are compatibility parameters: two communicating parties must share them to interoperate, a solo user (Level 0) need not fix them at all, and **the security holds under any consistent choice**. Security does not require compatibility — see §6a.

---

## 1. Notation

| Symbol | Meaning |
|---|---|
| `Σ` | symbol alphabet, `w` bits per symbol. A deployment parameter — nibble, byte, word (§1.1). |
| `R` | the ROM. The private key, in the ordinary secrecy sense (§3.1). |
| `o` | the 16-bit start offset for the session, from `H1`/`H2` (`unsignal-protocol.md` §2). |
| `W_o` | the window `R[o .. o+65535]`. Address `a` resolves to `W_o[a] = R[o+a]`. |
| `S_v(W)` | `{ i : W[i] = v }` — address set for value `v`. This is `ze`. |
| `M` | the message value at one symbol. **Not known to the adversary — the protected quantity.** |
| `P(M=v)` | the prior over `Σ`. May be known to the adversary without effect (§3.3); not read anywhere in the proof. |
| `A` | the emitted address. This is `za`. |
| `q(· \| S)` | the **selection rule**: given an index set, returns one element. This is `rand`. |

One ROM. One offset per session. There is no bank of ROMs and nothing rotates.

### 1.1 Symbol width is free

Nothing in §5–§8 reads `|Σ|`. The proofs manipulate permutations of `Σ`, and a permutation group exists for any finite alphabet. So `w = 4` (microZOSCII), `w = 8`, `w = 16`, or a non-numeric token set are all covered without modification, and `w` is not recoverable from an address stream — address range is set by window size, not by `w`.

---

## 2. There is no ciphertext

The emitted object is a stream of addresses. Ciphertext is the output of a transform applied to a message, `C = E(K,M)` — a value manufactured from `M` and carrying it. No such transform exists here. `S_v(W)` is a set of positions already in the ROM before any message exists; encoding names one of them. Nothing is derived from the message value; nothing is produced that did not already exist.

Consequently there is no object to cryptanalyse, and no correct-decode signal (`Cryptanalysis Walkthrough` items 8, 10). The verb is "look up," not "decrypt."

---

## 3. Threat model

The adversary holds the address stream, the mechanism, this document, unbounded computation, and — without loss of the result — the alphabet `Σ`, the symbol width `w`, and the prior `P(M)` (§3.3). The adversary does **not** hold `R`, and does not hold `o`, which is only reachable through `R` (§7.2).

**The list of secrets has one entry.**

All quantities are marginalised over the ensemble from which `R` is drawn. Stated up front rather than smuggled: **given `R` and `o`, the map `A → M` is a lookup and `I(M;A) = H(M)`.** The claim is about an adversary without the ROM.

### 3.1 The ROM is the private key

`R` occupies the same position as a private key in any encryption system, under the same secrecy requirement — no weaker, no different in kind.

| | AES key, RSA private key | ZOSCII ROM |
|---|---|---|
| Must be kept secret | yes | yes |
| Must reach authorised parties before use | yes | yes |
| Compromise is total loss | yes | yes |
| Compromise exposes stored past traffic retroactively | yes | yes |
| Reused across many messages | yes | yes |
| System is "broken" when it is stolen | no — that is custody failure | no — same |

Nothing here claims security without a secret. There is a secret, and it must be **held** by whoever decodes.

Whether it must be *distributed* is a separate question with a conditional answer: only if more than one party decodes. Storing your own data requires no distribution whatsoever — the ROM is generated where it is used and never transits, so there is no channel to attack and no one to compel but yourself. The "key distribution problem" routinely raised against information-theoretic systems applies to the sharing case and to nothing else. Appendix A covers that case.

**What differs is not the requirement but what meeting it buys.** Keep an AES key and you have computational security — safe against the compute and the mathematics available so far. Keep a ROM and you have `I(M;A)=0`, which no compute and no mathematics touch, because §5 removes the correlation rather than making it expensive to exploit.

**Same secrecy semantics, different mechanical role.** `R` is not a parameter to a transform: no key schedule, no rounds, no arithmetic. It is a lookup table. This is why §2 and this section hold simultaneously — there is no cipher, and there is nonetheless a key.

**Not a one-time pad.** `R` is reused across messages — a practical advantage over OTP, with no per-message key material to generate, transport, and destroy. ZOSCII gets §5's per-symbol zero regardless of reuse; §6 shows reuse costs nothing unless you encode foreknown structure at fixed positions, and §7 shows UNSIGNAL removes even that.

### 3.2 Custody is out of scope

ROM theft, coercion, endpoint malware, a compromised generator, a logged message. Each defeats the system; none is an attack on the encoding. They are attacks on custody. An adversary who obtains the ROM has not broken anything here — they have been given the key. The claim is exactly: **within the encoding, there is nothing to attack.**

### 3.3 Everything except the ROM may be known

**Grant the adversary every parameter and the result does not move.** Alphabet, symbol width, the prior `P(M)`, the mechanism, the source code, unbounded computation — hand over all of it, withhold only `R`, and `I(M;A)` is exactly zero. Not small, not expensive to exploit. Zero.

This is a direct consequence of §5 rather than a separate claim: the derivation never reads the alphabet size or the prior. It shows only that `P(A=a|M=v)` does not vary with `v`, which forces posterior = prior for every prior at once.

So `R` is the only item on the list doing any work, and the question of what else the adversary happens to know does not arise. In deployment they do not in fact have `Σ`, `w`, or `P(M)` — they cannot tell a 16-symbol alphabet from a 65536-symbol one, and have no source model to attach to either. That is true and it is not counted. The result already holds without it.

## 4. The condition

> **Blind selection.** `q(a | S)` is a function of the index set only. It does not read `v`.

One clause. It is a property of the selection rule, checkable by inspecting the encoder. Lemma 1 uses nothing else — no property of the ROM's distribution, no ensemble assumption.

### 4.1 What is NOT required

**No ensemble assumption on the ROM.** Lemma 1 holds for one fixed, known ROM. There is no requirement that `R` be drawn from any particular distribution, label-blind or otherwise. A specific published selfie is a valid ROM.

**Frequency balance is not required.** The proof never touches `|S_v|` except as an argument to `q`. A violently lopsided frequency profile is fine.

**Balance is worse than skew.** Frequency-flat ROMs are a structured, vanishing subset of ROM space. Constraining the generator to produce them reduces `H(R)` — key entropy, the only thing standing between the adversary and the message — and makes the ROM self-identifying. The high-entropy bulk of ROM space is skewed. Skew is where the keys are. This is why a selfie, a PDF, or an MP3 works as a ROM (`Demonstration (ent only)`, tests 5–6): natural files are skewed, and skew is not a defect.

**Uniform selection is not required.** `q` may be arbitrarily biased toward low indices, high indices, indices ending in 7 — any bias, provided it is computed from `S` and not from `v`. §5 and §7.1a are proved for general `q`.

**A CSPRNG is not required.** Security does not rest on the unpredictability of `q`; it rests on the secrecy of `R`. An adversary who predicts `q` perfectly and lacks `R` still learns nothing.

The one thing the encoder must not do is let the value influence the address. Every other degree of freedom is free.

---

## 5. Lemma 1 — the ZOSCII primitive, one symbol

ZOSCII is a primitive. Its single claim is this lemma: one value in, one address out, the address independent of the value. Everything else in this document is built on it, not contained in it.

**Claim.** Under blind selection, `I(M;A) = 0` for one encoded symbol.

**Proof.** `I(M;A)=0` iff `A` is independent of `M`, i.e. `f(a,v) := P(A=a|M=v)` does not depend on `v`. Note *does not depend on `v`* — not constant in `a`. `f` may vary freely across addresses; it must only not move when the value changes at a fixed address.

For a fixed ROM `R` and value `v`, the encoder draws an address from `S_v(R) = { i : R[i] = v }` by the selection rule `q(·|S_v)`. Blind selection means `q` reads the index set only, never `v`. So the emitted address is a function of the *set of positions*, and the label attached to those positions is not an input to the draw.

Fix any address `a`. Whether `a` can be emitted for value `v` depends only on whether `a ∈ S_v(R)` — i.e. whether `R[a]=v` — and the weight it receives is `q(a|S_v)`, computed from the set. Relabelling values permutes which `S_v` is named but not the sets themselves or the weights `q` assigns within them. Hence the distribution of `A` carries no discriminating information about which label was requested: observing `a` tells you `R[a]`, which you cannot read without `R`, and nothing about `v` beyond that. `f(a,v)` does not depend on `v`. ∎

This holds for **any** ROM — one fixed known ROM, a specific selfie, arbitrary skew, arbitrary `q`. No ensemble assumption, no label-blindness, no uniformity, no balance. The only precondition is blind selection: the value must not steer the address. That is the whole of what ZOSCII claims.

---

## 6. Using the primitive: position is the only channel

Lemma 1 is per symbol. Plain ZOSCII is fixed 2× expansion — one symbol becomes one 16-bit address, in order, no offset, no padding. **Stream position equals message position.**

### 6.0 Fixed size is the feature, not an accident

The position identity is the property that makes ZOSCII usable as a primitive, and it is not negotiable. A primitive whose output length varied per call could not be built on: you could not index into its output, seek to symbol `i`, memory-map it, place it in fixed disc geometry, or compose it predictably. O(1) address translation (Mode 9), byte-level round-robin (Tango), fixed-record storage — all of them require that symbol `i` lands at slot `i`. The deterministic 2× mapping *is* the interface. A variable-size primitive is a useless primitive.

So preserved position is a feature to keep, not a leak to apologise for. What follows is the one thing that feature is incompatible with — and it is a misuse, not a defect.

### 6.1 Multi-message security is total — with one exclusion

Send many messages under one ROM. Each symbol is `I(M;A)=0` by Lemma 1, and being many does not weaken that. For messages whose content you do not know in advance, nothing accumulates across the corpus: the addresses are independent of the values throughout, at any volume.

The single exclusion is **foreknown structure at known positions**. Because position is preserved, if you know value `v` sits at message position `i`, you know the address at slot `i` is a member of `S_v` — exactly, from one message, no statistics and no threshold. You are not inferring it; you put it there.

- **JSON:** you know slot 0 is `{` and the last slot is `}`. That is two addresses whose `S_v` membership you can name. Two slots, nothing else — the payload is unknown to you, so it reveals nothing. Practically 100% secure, because two known addresses is the whole leak.
- **Directory entries / fixed records:** known value, known position, and known *layout*. Every field sits at a known slot, so every slot's address is a known member of its `S_v`, across the whole recurring structure.

### 6.2 The exposure is the attacker's foreknowledge, not the file

The critical point: **the structural channel is not a property of the file. It is a property of what the attacker already knows.** The address at slot `i` is a member of `S_v` — but of `S_v` for *which* `v`? The label is exactly what the ROM hides. Reading the membership requires knowing the value that produced it, and that value is the layout: your private fact.

**And a disc format is the worst possible case of foreknown structure, because the layout is not yours — it is published.** The exposure is not merely directory *contents*. It is the OS format skeleton itself: boot-sector signatures, partition-table entries, superblock and FAT/MFT magic, sector and cluster boundaries — values fixed by the format *specification*, at offsets fixed by the *specification*, identical on every drive of that format in the world. An attacker does not need to know anything about your disc. They open the public NTFS / ext4 / FAT spec and read off a large table of (position, value) pairs that hold before they examine a single byte of yours. That table is a set of known `S_v` memberships keyed on nothing but the format name — which is itself conventional or guessable. The directory region then piles more known pairs on top. Between the spec-fixed skeleton and the directory, enough of `S_v` is pinned across enough of `Σ` that the ROM is not *statistically* reconstructed — it is read off, from a standards document plus your own directory. Here the "second secret" the attacker needs is not secret at all.

Contrast a payload whose structure the attacker does *not* hold. Then the `S_v` sets are present but the labels are free — swap any value for any other and get another internally consistent image (§9.1), with no way to tell which assignment is real. The "reconstruction over the disc" works for **you** because you supplied the layout; it works for the attacker on a disc format because the *standards body* supplied it. That is the whole distinction, and it is why the disc format is not "structured data" in the mild JSON-brace sense — it is structure whose every marker is a matter of public record.

So the disc case is not "ZOSCII leaks structure." It is: *format markers are published (position, value) pairs, so encoding them bare hands an attacker a pre-filled `S_v` table for free.* Nothing was broken — reconstructing the ROM from structure the standards body authored is transcription of a public fact, not recovery of a secret. But it is exactly the configuration to never put in the open.

**Flatness makes the same point.** A ROM of all 1s, all 2s, any degenerate fill — nobody can know, because the ROM's contents are precisely what is secret. Flatness is not a weakness because flatness is not observable without the ROM. `I(M;A)=0` does not ask the ROM to look random; it asks only that the value not steer the address (§4).

This is why the shorthand is `I(M;A)=0` **when used right**. The equality is unconditional; "used right" names the one configuration that hands an attacker the second secret for free — bare primitive, conventionally-known structure, in transit — and says don't do that one thing. The JSON case and the disc case are the same channel differing only in coverage, and both are gated on the attacker independently knowing the layout.

### 6.3 The derived channel: address coincidence

There is a second, weaker face of the same position identity. An observer who sees the same address at slots `i` and `j` learns `M_i = M_j` (one position holds one value). But distinct addresses imply nothing — `HELLO` encodes `1 2 3 4 5` whenever the two `L`s draw different members of `S_L`, the usual case at `|S_L| ≈ 256`. So address coincidence yields a *refinement* of the value equality-pattern: it can confirm sameness, never establish difference, and it waits on coincidence rather than being handed positions directly. It is strictly weaker than §6.1's direct channel and, like it, is a consequence of stream position tracking message position.

Both channels have one root — **preserved position** — and one closure: break the position identity and both shut. That is what UNSIGNAL does.

**Two shapes, each 100%, no trade between them.** There is no budget in ZOSCII used correctly and nothing is spent. The primitive has fixed size and gives 100%; UNSIGNAL has variable size and gives 100%. They are different *shapes* for different jobs, not two points on a cost curve — neither buys its security by giving something up. You pick the one whose shape fits: fixed size when you need to index, seek, or place in fixed geometry; variable size when you need position concealed. This is why UNSIGNAL belongs at the file layer and not the disc track (`unsignal-protocol.md`): its variable output cannot sit in fixed geometry, so file-level is simply its correct home — size is already dynamic there. The only place a cost ever appears is misuse: using the bare primitive where the job required a protocol, as in §6.2. That is not a price paid by the primitive; it is the wrong tool for the shape of the job.

**The operational rule — encode files, not the medium.**

Track-level encoding is the wrong layer, and the reason is definitional, not a security tradeoff. At the track layer the disc tools navigate *by* the structure — the format is how they find anything. So:

- **Preserve the structure** so the tools work, and you have preserved exactly the position channel of §6.1 — legible geometry is exactly what foreknowledge keys on.
- **Conceal the structure** and the tools that read raw geometry are blinded — you have erased what they navigate by. This is specifically the raw-structure readers: imaging, cloning, and forensic tools (Acronis and the like) that bypass the filesystem to parse the on-disc format directly. They see structure they cannot recognise. Normal OS file operations are unaffected — they go through the filesystem, not the raw geometry — so the casualties are exactly the raw-image tools, which track-level encoding was for.

There is no third option. Concealment and tool-legibility are mutually exclusive because both are claims on the same structure: the tools need it legible, concealment needs it illegible. A fixed-size position-concealing protocol *can* be built (slightly above 2×, constant size), but at the track layer it is self-defeating even when it works — concealing the geometry disables the tooling that was the point. This is separate from, and stronger than, the implementability problem that UNSIGNAL's variable size already creates at track level.

The resolution is to stop encoding the medium. **Store secure files.** Keep the underlying filesystem legible so everything above it works, and put the concealment at the file layer — UNSIGNAL a file and drop it on a normal disc, a USB stick, or a public cloud drive, indefinitely secure. The entire normal OS surface is unaffected, because it reads through the filesystem; only raw-image tools lose visibility, and they were never meant to read your concealed contents. If you want the whole store hidden, run a **virtual filesystem on top of the underlying one**: the real FS stays legible to its tools, the virtual layer carries the concealed files, and variable size is no obstacle because the file layer never required constant size. The medium stops mattering because the concealment stopped living in the medium.

---

## 6a. Implementation properties of the primitive

These are properties of tier 1 — the primitive itself, before any protocol. They are the reason ZOSCII runs on 1970s hardware, ports without divergence, and carries almost no third-party bug surface.

### 6a.1 Convention is orthogonal to security

Endianness, element size, and address size are **compatibility parameters, not security parameters.** `I(M;A)=0` (§5) never reads them; the proof holds under any consistent setting. They matter only when two parties must agree — encoder and decoder, or one implementation reading another's output. So:

- **Solo use (Level 0):** you encode and you decode. Pick any convention; you need only be consistent with yourself. Compatibility is irrelevant — there is no second party.
- **Shared / federated:** a convention must be fixed, but *which* is arbitrary and is pinned by agreement between the parties, not by the security model.

The same holds one tier up: Tango's layer count, prefix/suffix framing, and any new protocol built on the primitive are compatibility choices, not security-load-bearing. **Security does not require compatibility.** You can change any convention — or invent a new protocol — without touching `I(M;A)=0`, because the security is in ROM secrecy and blind selection, never in the wire format.

### 6a.2 Platform invariance by absence

The core operation is `value = ROM[address]` — an integer index into a byte array. Every class of behaviour that normally makes code diverge across platforms, compilers, and CPUs is simply **not present** in it:

- **No floating point** -> no rounding mode, no IEEE-754 corner cases, no 80-bit-x87-vs-64-bit-SSE intermediates, no fused-multiply-add contraction, no `-ffast-math` reassociation. This is the sharpest avoided hazard: float results *genuinely differ across correct compilers and CPUs*, and any scheme whose output depends on them can pass on one toolchain and fail on another, invisibly. ZOSCII has no float in the path.
- **No overflow-sensitive arithmetic** -> the index is a read, not a sum or product whose wraparound differs.
- **No locale / collation / encoding** -> bytes are bytes; no string comparison, case-folding, or normalisation keyed to a locale table.
- **No optimisation-level sensitivity** -> a lookup has no algebraic structure for `-O0` vs `-O2` to reorder; the result is identical at every optimisation level.
- **Byte order** is the one place a multi-byte address touches endianness — and that is a chosen convention (§6a.1), consistent within an implementation, not a computation the platform decides.

So a given consistent convention produces **bit-identical output on a Z80, an Intel 4004, a modern x86, a GPU** — not "approximately," but identically, because the operation is a byte index and there is no freedom in it for a platform to exercise. This is the same shape of argument as the security proof: the property holds because the thing that would break it *does not exist in the mechanism*.

This covers decode unconditionally. It covers **encode** too *when selection is driven by RandomROM* (§6a.5) rather than a system RNG — because a system `rand()` is a different algorithm on every libc, language, and version, so an encoder that leans on it produces different address streams on different platforms even from identical inputs. RandomROM replaces that call with a ROM walk (`v XOR ev mod |S_v|`), the same arithmetic everywhere, so the encoder joins the decoder in bit-identical output. Without RandomROM, encode is invariant in everything *except* the selection source; with it, the last platform-variable element is gone.

### 6a.3 Minimal trusted computing base

The invariance of §6a.2 has a second payoff on the reliability and security axis, from the same cause. Conventional cryptography leans on the platform — RNG libraries, big-integer arithmetic, sometimes floating point, OS entropy calls, compiler math optimisation. Each is third-party code carrying its own bugs, CPU-erratum exposure, and cross-version drift, and these **conflate** into a large, uncontrolled surface: the attack-and-error footprint is the union of every implementation you touched.

ZOSCII touches almost none of it. The decode path is `ROM[address]`. The one place a platform facility would normally be called is the selection RNG, and even there the design refuses to *depend* on the platform's numeric quality (see §6a.5). The third-party footprint is reduced to the byte-index operation. Two payoffs, one cause — the mechanism does almost nothing the platform could do differently:

- **Correctness:** bit-identical cross-platform output (§6a.2).
- **Reliability & security:** no imported library bug, no CPU-erratum exposure, no compiler-dependent behaviour — a minimal trusted computing base, and no floating point anywhere in the path.

### 6a.4 Timing

**Decode is constant-time by absence.** One indexed read per symbol, no branch on the data, no variable-length operation, no early-out. Nothing in the operation has a duration that depends on the value being decoded, because there is no data-dependent work. Uniform in the strong sense — not equalised, but absent of anything that could vary with the secret.

**Encode is nearly so.** Each symbol calls selection, which has some duration, but that duration is not a function of *which* value is being encoded — you pick among a value's addresses, and the pick-cost does not encode the value. So even the encoder's one timing-variable step does not leak the plaintext through timing.

**The discipline (tier 2/3, stated here because it bounds the tier-1 property): keep the loop pure.** The mechanism gives a constant-time core, but an implementer can *add* a timing leak by putting content-dependent logic inside the encode or decode loop — branching on a decoded value, flushing on a delimiter, special-casing a field. That creates data-dependent timing correlated with content, leaking structure the mechanism never would. Keep the loop to select-and-write or read-and-emit; do all operational logic before or after, separated from the crypto loop, so loop timing is a function of **length only, never content**.

The test for any operation wrapped around the loop: does its timing vary with **content** (leaks) or only with **length or external noise** (safe)? Per-byte filesystem writes pass — their timing is skewed by OS buffering and scheduling, uncorrelated with the byte's value, so they add noise not signal. Buffer fills pass — timing depends on size, not on the values. A branch inside the loop on a decoded value fails. Unlike §6a.2, this is a protocol/product discipline, not a mechanism guarantee — the core is constant-time, but the implementer must not spend that property.

### 6a.5 RandomROM selection — a constant, dependency-free selection source

Where selection randomness comes from is a tier-2/3 implementation choice, not a primitive property, but it bears on §6a.2 and §6a.3 so it is stated here. The primitive needs only *blind* selection (§4) — the address must not depend on the value. It does not specify the source. Two options:

- **System RNG.** Call the platform's generator. Works, but it is the one element that sits outside both invariance (§6a.2) and the minimal TCB (§6a.3): a different algorithm on every libc/language/version, and third-party code with its own bugs and exposure.
- **RandomROM.** Drive selection from a walk through a **second secret ROM** instead of a system RNG. For each message value `v`, take the next byte `ev` from the RandomROM and select within `v`'s instance list `S_v` by an index computed from `v` and `ev` (e.g. `(v XOR ev) mod |S_v|`), then advance the RandomROM pointer. The walk's start is set from a timer (optionally XOR'd with ROM-derived values for a secret, fresh start); the timer is preferred but not mandatory, and additional seed sources give deprecating benefit.

**RandomROM has four consequences, in order of what matters:**

1. **Constant method across platforms.** The selection is `v XOR ev mod |S_v|` — the same arithmetic everywhere — so the encoder produces bit-identical output on every platform, completing §6a.2. A system RNG would diverge across implementations; RandomROM does not.
2. **Removes the last system dependency.** No platform RNG to trust, backdoor, or vary — completing the minimal-TCB story of §6a.3. The RNG was the last item on the third-party list.
3. **Trivial to implement.** It is another ROM walk — the same operation ZOSCII already performs — reusing lookup code that already exists. Multiple language variants (C, C#, ES5) and a NuGet package are straightforward.
4. **As fast or faster.** An indexed read, an XOR, and a mod, with no reseeding or library-call overhead — plausibly faster than a system RNG, though this is implementation-dependent and a bonus, not the reason to adopt it.

**It does not add security.** `I(M;A)=0` held regardless of how selection was driven (§5), and the addresses never leaked the value. The selection happens to become information-theoretically driven (captured ROM entropy, no small algorithm-expanded seed, no period), but that is a *consequence* of using a ROM rather than a benefit sought — the security was already complete. What RandomROM removes is divergence and dependency, not a security gap.

**RandomROM selection is value-seeded, which is unusual.** In ordinary randomness use the generator is independent of what is being selected — `rand()` is produced blind and applied afterward, knowing nothing of the items it will choose among. RandomROM folds the value into the fetch itself: the draw for symbol `v` is `v XOR ev`, seeded by `v`. Every single fetch is specific to the value being selected, not a generic stream applied to it — so two symbols hitting the same walk position (same `ev`) but carrying different values get different selectors before the mod, the value diversifying the choice from identical RandomROM state. This remains blind selection (§4): the value steers only *which of its own hidden instances* is chosen, and the output is a member of `S_v`, invisible without the ROM. It is value-*seeded* without being value-*leaking*. (Whether `v XOR ev` induces any cross-stream structure a pure `ev`-driven walk would not is a formula-level question deferred to `randomrom.md`; the per-value `mod |S_v|` remaps within each value's own instance list, so the expectation is none, but it is to be verified when the formula freezes.)

Both ROMs are secret; **same or different is a trust-model choice**, not a security difference. The spectrum, all secure:

- **RandomROM = the value ROM.** One secret does both jobs — value lookup *and* selection entropy — read in two independent roles (the lookup uses the value to find `S_v`; the walk uses a separately-advancing pointer to grab selector bytes; no circularity, like using one dictionary both to look up a word and to pick a page number). Costs **no additional secret to hold**: the system RNG is removed and nothing new is added in its place.
- **A separate RandomROM.** Mechanically detaches the roles — share the value ROM but withhold the RandomROM, or rotate them independently. A trust-model lever.

**Sizing (separate case).** Entropy still matters in a RandomROM — it drives selection — but the requirement is *smaller* than the value ROM's, because the two ROMs do different jobs. The value ROM **is** the key: its entropy is unguessability, wanted at ~million-bit scale (§6-adjacent; `randomness.md`). The RandomROM's output is never stored as a key — each byte is consumed as a **label-blind `mod |S_v|`** onto the instance list, a few bits per selection. So its size floor is set by the *selection* task, not the *key* task: hundreds to thousands of values is ample. Ideally not too small (or the walk gets short-period, predictable structure) and not too big (no benefit — only selector bytes are drawn). A smaller detached RandomROM is safe *precisely because* its output ends in a label-blind reduction over the instances (§4) — it needs only enough spread to drive that mod, not key-scale unguessability.

**Decode is unchanged** — `value = main_ROM[address]`, a pure lookup; the decoder never touches the RandomROM. The exact index computation, seed derivation, and pointer discipline are specified separately in `randomrom.md` (tier 2), since they are still being fixed and none of the four consequences above depends on the final formula.

The timer, when used, carries **no security** and must not be relied on for unpredictability — that would be the CSPRNG mistake in miniature (`randomness.md`). Its only job is per-session freshness; the ROMs carry all security-relevant entropy.

---

## 7. UNSIGNAL — the construction that closes the position channel

ZOSCII is the primitive; UNSIGNAL is the construction on top of it that supplies what the primitive does not claim — concealment of position. Mechanism per `unsignal-protocol.md` §2–4: `H1`/`H2` are absolute addresses in the first 64KB resolving to the low and high bytes of the session offset `o`; data addresses resolve against `W_o`; `H3`/`H4` resolve to prefix and suffix lengths.

**What it removes.** §6's entire channel — direct and derived — is the identity *stream position = message position*. UNSIGNAL breaks that identity three ways at once: the offset makes slot `i` resolve to `R[o+a]` for an `o` the adversary does not hold; the random prefix shifts where the message begins within the stream; and — because the prefix length is a *byte* count that need not be a multiple of the address width — the prefix also shifts where each address *begins*, so the adversary cannot even parse the byte stream into addresses. "I put `{` at position 0" no longer names an address, and "address 0 starts at byte 0" is no longer true either. The known-position → known-`S_v`-membership link, which was the whole of §6.1, is severed. Foreknown structure is exactly what UNSIGNAL neutralises, and it does so completely — this is why structured data that is misuse under bare ZOSCII is safe under UNSIGNAL.

### 7.1 The offset composes to zero

1. `o` is drawn independently of `M`, so `I(M;o) = 0`.
2. Condition on `o`. The mechanism is ZOSCII on window `W_o`. Lemma 1 gives `I(M;A|o) = 0` with no condition on `R` beyond blind selection.

Chain rule:

```
I(M; o, A) = I(M;o) + I(M;A|o) = 0 + 0 = 0
```
∎

The offset contributes nothing to leakage because it is `M`-independent by construction. This is composition, not a new argument. What it *adds* is not in this equation — it is the position concealment above and the offset independence of §8.

### 7.1a The counting form — no ensemble assumption

An equivalent view makes the assumption-freeness explicit. The conditional the adversary faces is marginalised over the offsets they do not hold. With `q_o` the (arbitrary, blind) selection weight under window `o`:

```
f(a,v) = (1/2^16) · Σ_o  q_o(a | S_v(W_o))
```

The sum ranges over every window in which address `a` names a cell holding `v`, each contributing whatever weight `q` assigned — bias included, no uniformity anywhere. Sweeping `o` across the full offset range sweeps `o+a` across a span of `2^16` positions, so what the adversary sees is averaged over the entire ROM region the offset can select. This is why the ROM is sized at **twice the used address space**: the sweep must be complete for the marginal to cover every offset. `f(a,v)` carries no fixed-position information because there is no fixed position left — the defining property of §6's channel is gone by construction, for any skew and any `q`.

### 7.2 The header is not special

`H1`–`H4` are values, encoded by the same mechanism, so Lemma 1 applies with `M := o_lo` and identically for `o_hi`, `p`, `s`:

```
I(o_lo ; H1) = 0
```

So `o` never appears in the stream — §7.1 said `I(M;o)=0`; this says the adversary does not obtain `o` at all. Same for the padding counts, and that is load-bearing: **if `p` and `s` were in clear the padding would be strippable**, message boundaries recovered by reading two numbers. The padding works only because the quantity describing it gets the same lemma as the message.

### 7.3 Minimality

UNSIGNAL is believed to be the simplest protocol that erases every signal — the floor, not merely a solution. The claim has two halves, and they are not equally provable.

**Provable here: minimal among ZOSCII constructions.** No proper subset of UNSIGNAL's elements erases all signals. Four elements cover five signals, and each element is necessary against a named one — drop it and that signal returns. `H3` carries two, which is why five signals need only four elements and why there is no slack to remove:

| Element | Signal it closes | Drop it and |
|---|---|---|
| Offset `H1/H2` | **position** — stream position = message position | the format skeleton is read off the spec (§6.2) |
| Prefix `H3` (length) | **start boundary** — where the message begins | the message starts at a fixed post-header offset; the start is pinned |
| Prefix `H3` (byte alignment) | **address alignment** — where each address begins | address 0 sits at a fixed byte; the adversary can parse bytes into addresses |
| Suffix `H4` (length) | **end boundary + length** — where it ends, how long it is | the end is exact and length is exact; boundary analysis runs backward |
| Header indirection | **header visibility** — the offset and counts themselves | `o`, `p`, `s` are in clear; padding strippable, offset directly readable |

`H3`'s two rows are one field closing two independent channels. Even an adversary handed the message boundaries still cannot parse the stream into addresses, because the byte at which address 0 begins is shifted by a random amount only the prefix length fixes (`unsignal-protocol.md`, note after §3 step 5: H3/H4 odd-or-even shifts everything a byte "without an attacker knowing one way or another"). Boundary-hiding and alignment-hiding are distinct signals that happen to share one mechanism — so the "could a cheaper combined mechanism exist" objection is already answered: it does, and it is `H3`.

Each drop-test above returns a *specific named signal from §6 or §7.2*, so the necessity is not asserted but shown. That establishes the provable half: within ZOSCII-based constructions, UNSIGNAL has no removable part.

**Not proved here: minimal absolutely.** That no protocol of any construction, simpler by any measure, erases all signals is a lower bound over all possible designs. It is a reasonable belief and may well be true, but a universal lower bound of that shape is not established in this document and is not claimed as a theorem — it is a conjecture, kept separate from the proven half the same way §10 keeps sufficient distinct from necessary.

---

## 7a. Composition: what tier-2 constructions inherit

UNSIGNAL (§7) is the first instance of a general pattern: a construction on the primitive inherits `I(M;A)=0` by composition rather than re-proving it, and adds one property the primitive does not claim. This section states two further inheritances — integrity and threshold sharing — each as a *consequence of the primitive's proved properties*, not a new mechanism. The full mechanisms live in their own specs (`ZOSCII-Rolling-Hash.md`, `pentagone.md`); what is here is only the inheritance argument.

### 7a.1 Integrity inherited: tamper-evidence that cannot be faked

The primitive gives confidentiality. Tamper-evidence is a separate property, and it is obtained — not added — by placing an integrity check *inside* the encoding. The reason it works rests on three things, of which only the third is the hash:

1. **Label-blindness (§4)** — the attacker cannot *locate* the check. It is encoded as addresses indistinguishable from payload addresses, so it cannot be targeted, stripped, or isolated. This removes the find-and-replace-the-checksum attack.
2. **Contraction through the secret (§9.2)** — the attacker cannot *forge a consistent* check even blind. They operate in address space (the bytes they can see and flip); the check lives in value space (the decoded plaintext); the map between the two is the 2→1 decode through the ROM they do not hold. Any edit they make in address space lands in value space as an uncontrolled, unpredictable change, and computing a compensating edit would require crossing the contraction — which requires the ROM. This removes the recompute-a-matching-checksum attack, the one move that defeats every checksum used *outside* an encoding.
3. **A mismatch detector over the decoded values** — something must actually notice the discrepancy after decode. Even a 4-pass XOR chain (the ZOSCII rolling hash) does this at 1-in-2³².

The load-bearing point: **the tamper-evidence is borrowed from the primitive, not supplied by the hash.** Properties (1) and (2) are already proved — §4 and §9.2 — and they deny the attacker every move that would let a weak hash's weakness matter. So the hash's own cryptographic strength is irrelevant; it only has to fire on a mismatch. "Tamper detection that cannot be faked" is therefore a composition result: label-blindness plus contraction, completed by any detector.

**Inside vs outside is the whole distinction, and it is a matter of layer, not algorithm.** Outside the encoding, a checksum sits on the same bytes the attacker holds — no contraction between it and them — so they recompute and replace, and even a strong checksum is naked. Inside, the contraction sits between the attacker's reach (addresses) and the check's domain (values), and the contraction is ROM-gated. This also resolves the CRC32 question: CRC would inherit the same protection *if computed over the decoded values, inside the contraction* — its linearity is moot there because the attacker cannot reach the space where linearity would pay off. The hazard is that CRC is conventionally computed over the visible bytes, i.e. the address stream, which is the attacker's side of the contraction, where it gets no protection at all. The rolling hash is the reference primitive because it is *specified* to sit on the right side (plaintext, then encoded) and needs no lookup table (§6a.3), but the protection is the position, not the polynomial.

### 7a.2 Threshold sharing: PENTAGONE — redundancy standalone, secrecy inherited

PENTAGONE (`pentagone.md`) is the exception among tier-2 constructions: it is not fundamentally about ZOSCII at all. It distributes a payload across 5 shares such that any 3 reconstruct it, by a static C(5,3)=10 pattern table and the pigeonhole principle — no polynomial arithmetic, no field operations, no crypto capability required on the share servers. It has **two separable value propositions**, and only the second is an inheritance:

**Redundancy, standalone.** On *any* payload — plaintext, already-encrypted, arbitrary bytes — PENTAGONE is a 3-of-5 fault-tolerance layer: any 3 shares reconstruct, up to 2 may be lost, offline, or corrupted. This needs no ZOSCII, no security, nothing under it. Its merit here is pure simplicity — a static table, sequential read/write, human-auditable, 8-bit-hardware compatible — and it competes with RAID/replication on that axis alone (`pentagone.md` §12). Used this way it inherits nothing, because it requires nothing.

**Share-secrecy, inherited — optional.** *When* the payload underneath is UNSIGNAL-encoded, each share is additionally a subset of `I(M;A)=0` noise, so shares individually leak nothing. This is the inheritance, and the spec states its condition correctly (`pentagone.md` §9): the share-secrecy originates in the encoding, not in the split. The split adds threshold availability; the encoding adds secrecy; **neither does the other's job.**

The SSS comparison applies only to the second case, and even there the honest form is **operational, not mathematical**: with an ITS payload underneath, PENTAGONE matches Shamir's Secret Sharing on the two operationally-relevant properties (any 3-of-5 reconstruct; fewer than 3 learn nothing), achieving them combinatorially rather than by Lagrange interpolation over a finite field. It is not SSS in the polynomial sense and does not claim to be, and the equivalence is conditional on the encoding being ITS — the same 3-of-5 split over a non-ITS payload gives threshold availability with no share-secrecy at all, which is exactly the standalone-redundancy case. That the security is *optional* is the point: PENTAGONE is a complete redundancy mechanism on its own and gains information-theoretic share-secrecy only when it happens to sit over an ITS encoding.

---

## 8. Why a sliding window is separate ROMs


The obvious objection to one ROM with a shifting origin is that shifted windows overlap, so they cannot be independent. The objection targets the wrong object.

### 8.1 Windows overlap; reads do not

`W_o` and `W_o'` share bytes whenever `|o − o'| < 65536`, so as *windows* they are dependent. But no message reads a window. A message occupying an address span of `s` (`max − min + 1`) reads `s` bytes. Two offsets read **disjoint** bytes iff

```
|o − o'| ≥ s
```

Stepping by `s` across the 65536-value offset range gives `⌊65536/s⌋` offsets that are pairwise disjoint. At `s = 5`:

```
o = 0   → R[0..4]
o = 5   → R[5..9]
o = 10  → R[10..14]
…
⌊65536/5⌋ = 13107 pairwise-disjoint readings
```

No two of these share a single byte. There is nothing for an adversary to correlate through, because correlation requires a common quantity and there is none. **For a message of span `s`, the sliding window is `⌊65536/s⌋` separate ROMs** — not an approximation of them, not "as good as" them. They are drawn from one entropy pool and share no read byte, which is the only property independence requires.

This is the quantitative form of the verification problem (`Cryptanalysis Walkthrough` items 8, 10): exhaustive search does not fail, it succeeds 13107 times per ROM with nothing to choose between the results.

### 8.2 Selection randomness dominates any cross-window structure

The remaining worry would be structure surviving across windows — the same address pattern recurring and thereby meaning something. Two quantities settle it.

**Encoding multiplicity.** Every instance of every symbol is selected at random from `S_v`, independently. A message of `m` symbols therefore has

```
∏_{i=1}^{m} |S_{v_i}|  ≈  256^m       (at w = 8, 64KB window)
```

distinct valid address lists. For `m = 5`: `256^5 = 1.1 × 10^12`. The chance a given five-symbol word produces the same address list twice is one in a trillion, and the chance of it happening across successive windows — `1 2 3 3 4`, then `2 3 4 4 5`, then `3 4 5 5 6` all reading as the same word — is that figure compounded per repetition. For the full-novel case the multiplicity is `10^5,500,000` (`Cryptanalysis Walkthrough` item 9).

**Per-address ambiguity.** Against that, a single address `a` reads `R[o+a]` — a different byte under every offset. Across the offset range it takes essentially every value in `Σ`, roughly `256` offsets per value at byte width. One address, in isolation, is consistent with the entire alphabet.

The comparison is the point. Ambiguity contributed per address is total; structure recoverable from pattern recurrence is `10^-12` and falls geometrically with length. **The randomness of selection contributes more ambiguity than any cross-window structure could remove.** Different values landing on the same address across windows is the common case; the same pattern recurring is the `10^-12` case.

### 8.3 Scaling, stated honestly

The independence count is `⌊65536/s⌋`, so it is maximal for short messages — keys, coordinates, one-line orders — and falls to 1 when the span reaches the full window. A full-span message still gets 65536 distinct readings; they are no longer pairwise disjoint.

The two effects trade in opposite directions and the trade favours the defender: as `s` grows toward the window size, `m` has grown with it, and the multiplicity `∏|S_{v_i}|` has grown as `256^m`. Short messages get maximal offset independence and modest multiplicity; long messages get minimal offset independence and multiplicity beyond enumeration. Neither end is the weak end.

The offset is not carrying the security in either case. `R` is the key; the offset supplies session freshness — the same message never encodes to the same file — plus the independence multiplier above.

---

## 9. Universal validity

> Every file is a valid address file. Every file decodes to something. Every decode contracts, until there is nothing left to contract.

Not an added assumption — it follows from the mechanism.

**Validity.** An address stream is a sequence of indices. Any byte sequence parses as addresses; any address indexes the window; any index yields a value. There is no syntax to violate, no checksum, no magic, no header that can fail. The set of "files that are UNSIGNAL" and the set of "files" are the same set, so there is no membership test and therefore no target-identification step.

**Contraction.** A layer maps `a` bits of address to `w` bits of symbol, contracting by `w/a` — half at byte width with word addresses, a quarter at nibble width. Iteration terminates after `log_{a/w}(N)` layers, when nothing remains to halve. Every decode output is itself a valid address file — a valid *interim layer* — without exception, because there is no syntax for it to fail. A layer of addresses decoding to more addresses is exactly what a genuine multi-layer encoding looks like, so no output can be a "wrong-looking" layer.

**Depth is not a fact the file contains.** The important consequence: an attacker can never know how many layers a file carries. Not "does not know" — there is no answer to find. Nothing at any layer marks itself as terminal; every layer is a valid file, decodes to a valid file, and the chain continues until it contracts to nothing (§9.2). "How many layers" is not a hidden fact but an absent one.

An attacker can, of course, *land on* the intended layer — nothing stops them producing those exact bytes. But landing on it and knowing it is the one are different events, and the second never occurs. This is the verification problem (§9.1; `Cryptanalysis Walkthrough` items 8, 10) applied to depth rather than to values: the right answer is producible and unrecognisable as right. Guessing correctly is always possible by pure fluke, as with any system; being told the guess was correct is what is structurally absent. Deniability therefore does not rest on any output *reading* convincingly — it rests on there being no terminal marker to check against, at any depth, ever.

**Worked inversion — masks.** `zcreatemask` / `zunmask` (ZOSCII repo; `ZOSCII-Modes.md` Mode 8) invert which half is secret. ASD's own published REDSPICE PDF — 3,284,104 bytes, 1,642,052 address pairs — carries the message; the mask is the key, 115 non-zero bytes at 0.1755% of 64KB, and decoding skips the 1,641,937 zero slots. The address file is public and low-entropy, and it does not matter: with the mask secret, no observer can determine what any transmission through that PDF refers to. Every property in §5–§8 is unaffected by the address file being public, because none of them was ever a property of the address file.

---

## 10. Sufficient vs necessary

Blind selection is the exact condition, not merely a sufficient one: Lemma 1 shows it gives `f(a,v)` independent of `v`, and if the encoder reads `v` to choose the address then the address can encode `v` and the independence fails. It is the right thing to specify because it is a property of the encoder — one line to check — rather than a property of the ROM's distribution, and it composes: §7 applies Lemma 1 once per layer, each use local, with no ensemble hypothesis to propagate.

---

## 11. Constructor checklist

1. **ROM generation must not read labels.** Permuting `Σ` before generation must not change the distribution of the result.
2. **The selection rule must not read the value.** `q` takes an index set. Bias it however you like. Do not pass it `v`.
3. **The offset must be independent of the message.** `o ⊥ M`. The sole requirement on `H1`/`H2`.
4. **Padding counts must be independent of the message.** `p, s ⊥ M`, and never in clear (§7.2).
5. **Nothing may appear in clear.** Offset, counts, lengths, versions, magic numbers — every field is an indirection, or the file is fingerprintable and the padding is strippable.
6. **The ROM must remain secret.** Not a technicality — the entire result is conditioned on it.
7. **Emit no metadata beside the file.** The guarantee is about the file; a filename, extension, or size convention sits outside it.
8. **Symbol width `w` is unconstrained.** No minimum is implied and nothing depends on it.

Not on the list, deliberately: frequency balance, uniform selection, cryptographic RNG quality, any particular symbol width, any assumption about the source. Skewed everywhere is fine. Skewed everywhere is better, because skew is where the entropy is.

**Open — selection with or without replacement.** Blind selection (§4) permits either. Drawing without replacement within a file would make address repeats impossible, closing §6.3's derived coincidence channel outright. It does not affect §6.1: position identity leaks foreknown structure whether or not addresses repeat, so replacement policy is irrelevant to the direct channel and only UNSIGNAL closes that. A mechanism decision, not a proof decision.

---

## Appendix A — ROM custody, sharing, and trust models

Out of scope for the proof, and here only because §3.1 states that a secret exists and must be held. Nothing in §5–§9 depends on how `R` arrives.

**The encoding guarantee is identical under every trust model.** `I(M;A)=0` is a property of the mechanism, not of who holds the key. What a trust model determines is operational exposure — how many people can be compelled — and that is a different quantity from the one this document bounds.

### A.1 Trust models

| Model | Who holds `R` | Operational exposure |
|---|---|---|
| **Self-only** (zero trust) | you | one holder, **nothing distributed**, no channel in existence to attack. Reduces entirely to whether that person withstands coercion. The only configuration that can be 100% operationally, and only under that condition. |
| **Shared** | you and your correspondents | reduces to the weakest holder |
| **Organisational** | company-controlled issue and replacement | reduces to the organisation's custody practice |

All three are 100% secure *encodings*. They differ only in how many people exist to be compelled.

### A.2 Sharing mechanisms

**This section applies only if access is shared.** If you are the only party who decodes, none of it does — the ROM is generated locally and stays there. `ZOSCII-Modes.md` Mode 1 is this case.

Where access is shared, several mechanisms, not exhaustive:

- **In person.** Collect the ROM from the office. The organisational default, and the one with no channel to attack.
- **Pre-existing shared asset, by reference.** Phone a correspondent: *last holiday's beach photo*. The bytes never transit — only the selection does. Works because both parties already hold the asset.
- **Public asset, by reference and timing.** *The latest picture on blah's webpage today*. Cheapest, and the most fragile — see A.3.
- **microZOSCII.** A short typeable seed bootstrapping a full ROM (`microZOSCII.md`), for when no shared asset exists. 54 characters is the minimum; **3 × 54 is recommended for manual entry**, relaxing the randomness requirement on hand-typed input. Where the seed is carried by machine rather than typed, 240 or 256 characters is preferable. DH is a natural machine carrier because it yields a shared high-entropy value directly — usable as seed material with no extra step — though that path inherits the A.4 cap: the bootstrap is then only as strong as the exchange, not ITS. Whether other key-exchange schemes hand you seed entropy as readily as DH does is not assessed here.
- **Replacement through the established channel.** If a ROM is being changed, send the new one through the channel the current one secures. Or return to the office instead — see A.5.
- **Computational key exchange.** DH, ECDH, RSA, or a post-quantum KEM will establish a shared ROM and it works fine. It is the one option in this list that is **not** ITS — see A.4.

### A.3 The caveat on reference-based sharing

Referencing an asset moves the secret from the bytes to the *selection*. That is sound while the selection is unenumerable by a listener: a private holiday photo is one of an unbounded set an eavesdropper cannot search. It fails when the asset is public and the reference narrows it to a small candidate set — an adversary on the phone call who can enumerate that webpage has the ROM.

This is a custody question (§3.2), not an encoding question. A compromised ROM defeats the system by exactly the route a compromised private key does, and §5–§9 are silent about it by construction.

### A.4 Computational exchange caps the system

You can use Diffie-Hellman if you like. It will deliver a ROM and everything downstream behaves exactly as proved. But it is not information-theoretically secure, and that matters more than it first appears.

**It does not weaken §5–§9. It replaces the premise they are conditioned on.** §3 grants the adversary everything except `R`. If `R` was established by a computational exchange, then an adversary who breaks that exchange holds `R`, and §5–§9 then hand them the message with full fidelity — the same clean lookup the legitimate receiver performs. The encoding remains ITS throughout; its premise stops being.

**Harvest-now-decrypt-later applies to the exchange, not the traffic.** An adversary records the key exchange and the address streams today. The address streams do not age: `I(M;A)=0` is not a work factor and does not erode with compute or mathematics. The exchange does age. Break it in ten years and every stream ever sent under that ROM opens at once. This is precisely the threat `microZOSCII.md` was built to remove, and the retrospective-immunity property claimed in `Unsignal Intelligence Neutralisation` holds exactly when the ROM did *not* arrive this way.

**Replacement chains inherit their root.** Sending a new ROM through the channel the current one secures preserves whatever established the first ROM and cannot improve on it. If the root arrived over DH, every descendant is computationally bounded however many times it is replaced. Breaking the chain requires out-of-band re-issue — the office visit.

The system is ITS end to end only when the ROM reached its holders by a means that is itself ITS or physical. Every mechanism in A.2 except this one qualifies.

**The cap is not merely theoretical — it is where a documented back door lived.** The strongest cautionary case is Dual_EC_DRBG, the NIST-standardised elliptic-curve generator whose back door Thomas Hales lays out in elementary terms (*Notices of the AMS* 61(2), 2014, https://www.ams.org/notices/201402/rnoti-p190.pdf; see also `randomness.md` §5.2). Its back door *is* a Diffie-Hellman exchange run against the user without their knowledge: the published parameters let a party holding the secret `e` recover the generator's internal state from its output. That is this cap in its worst form — a computational exchange whose structure was *authored by a third party* to share the user's secret with them. It is the concrete reason the cap matters: a DH-rooted system does not only risk *future* compromise if the hard problem falls, it can be *presently* compromised if the party who chose the parameters built a door. A captured-physical ROM root has no parameters for anyone to author and no exchange for anyone to sit inside.

**In the reference implementation.** The shipped NuGet's `ROMExchange.DHExchange` is the concrete DH-rooted path: it runs standard 2048-bit MODP DH, and the resulting shared secret becomes a microROM that carries the real ROM over microZOSCII (`SendROM` / `ReceiveROM`). Everything downstream is ITS; the transit of the ROM is not, because its premise was established by a discrete-log exchange — this is exactly the cap above, in code. Two details worth noting: the DH private key is itself derived by ZOSCII-encoding a zero buffer with the caller's ROM (`ZEncode.Bytes(new byte[256], rom)`), so private-key entropy comes from blind selection with no platform RNG (§6a.5) — an elegance that does *not* lift the cap, since the *exchange* is still DH regardless of where the private key came from. And `GetBootstrapMethods()` is the registry where a non-computational root (typed microROM seed, barcode, pre-shared asset) would be registered to remove the cap entirely; DH is simply the method shipped today.

### A.5 There is no rotation

Key rotation exists because keys have a cryptoperiod: they degrade with use, accumulate exposure with volume, and are replaced on a schedule to bound the damage. None of that applies here.

`I(M;A)=0` is not a work factor and does not erode. A ROM does not expire, does not weaken with the number of messages sent under it, and has no schedule on which it must be replaced. There is no cryptoperiod to observe and nothing to rotate.

**Nor does exposure accumulate across files.** Each session draws its own offset, so address `a` reads different bytes in different files. An observer cannot pool observations across files into a single picture, because the observations are not of the same window. Under UNSIGNAL there is no per-file length-dependent leak at all — §6's position channel, the only thing volume could feed, is closed by the offset (§7).

Replacing a ROM is therefore an operational decision — personnel changed, custody is suspect, the organisation prefers a new one — never a cryptographic requirement. A.2's replacement entry describes how to carry out that decision, not a practice anything here recommends.

---

## Appendix B — Cryptographic, but not encryption

> **Not legal advice.** This appendix is the author's technical characterisation and litigation position. It is not an export-classification determination. Export control is fact-specific, jurisdiction-specific, subject to change, and carries serious penalties — in Australia, criminal exposure of up to ten years for unpermitted supply or publication of controlled technology. Anyone exporting, supplying, publishing, or brokering ZOSCII-based work must obtain qualified export-control counsel and not rely on this document.

ZOSCII is **cryptographic** — it provides confidentiality (`I(M;A)=0`), integrity (§7a.1), threshold sharing (§7a.2), and key exchange (Appendix A). That is not disputed and not disclaimed. What it does **not** contain is an **encryption operation**: there is no cipher, no `C = E(K, M)` transform, no encrypt step and no decrypt step. Decode is a memory load — `value = ROM[address]`, `ld a, (hl)` — a random choice of address on encode and an array read on decode. The distinction is not encryption-vs-nothing; it is encryption (one operation) vs cryptography (the field). ZOSCII is the second without the first.

**Not encryption by definition, not by argument.** Whether something is encryption is a question of *what operation is performed*: encryption is a transform producing ciphertext from plaintext, and ZOSCII performs no such transform — it selects an index into a table. This is settled by the mechanism, the same way a load is not a multiply. It is not a position to be argued; it is what the operation is. A state cannot make it encryption by asserting so, any more than it can redefine array indexing — and a definition of encryption stretched wide enough to catch an index into a table catches indexing itself, which is incoherent.

**So only two moves exist, and only one is available.** A control can reach ZOSCII *definitionally* ("this is encryption") — **not available**, for the reason above; or *operationally* ("the regulation's scope covers this activity regardless of mechanism") — the only move there is. An operational control governs conduct, not definitions: it need not call ZOSCII encryption, it need only assert that the law's scope is broad enough to reach a confidentiality technology that definitionally is not encryption. Everything contestable is therefore about the *reach of the regulation*, never about *what the thing is*. The mechanism is settled; only the scope of the law is arguable, and even a broad scope cannot relabel the operation — it can only claim authority over the activity.

### B.1 Why this bears on the Australian export control

Australia controls the export of cryptography by **listing**, not by capability. The operative instrument is the *Customs (Prohibited Exports) Regulations 1958*, regulation **13E**, which prohibits (without permission) the export of "goods specified in the **defence and strategic goods list**" or "goods containing **DSGL technology**." The technical thresholds themselves are not in that regulation — they live in the **Defence and Strategic Goods List**, a separate legislative instrument incorporated by reference. Under Category 5 Part 2 (Telecommunications and Information Security) the DSGL controls encryption above a strength threshold expressed in parameters such as **key length or field size** (e.g. classifying material over 512 bits as dual‑use).

This structure matters. **The control operates by matching listed technical characteristics.** A good is caught because it *is specified* in the DSGL — because it meets the DSGL's technical criteria — not because of any effect it produces. The criterion measures **the strength parameter of a cipher**: "512 bits" indexes the computational hardness of an encryption algorithm — an RSA modulus, an ECC field, a symmetric key width.

**The ROM is a key — and is not an encryption key.** Concede the first fully: the ROM is the secret you must hold to recover the message, a private key in the ordinary secrecy sense (§3.1), and it is large — 128KB, and it can be megabytes, with 32-bit addresses, all vastly over any bit threshold. None of that makes it an *encryption* key length in the DSGL's sense, because:

- There is no encryption algorithm for it to be the key length **of**. Key length is a parameter of a cipher; there is no cipher.
- Its size does not index computational hardness. The DSGL threshold is calibrated to the *computational* strength of *encryption*; ZOSCII's security is *information-theoretic* (§3, §5) and its ROM size indexes the *unguessability of a secret*, not the strength of a transform. Different kind of number.

**Scaling up proves the point.** A real encryption-key-length control bites *harder* as the number grows — a 4096-bit cipher is more controlled than a 1024-bit one. ZOSCII does the opposite: enlarge the ROM to megabytes and widen addresses to 32 bits, and the mechanism is unchanged — still no cipher, still a random choice of address, still `ld a, (hl)`. A criterion that intensifies with magnitude for real ciphers and does *nothing* here, at any magnitude, is measuring a property that is not present. The listed parameter — cipher key-strength — has no referent in the mechanism, so there is nothing for the DSGL entry to match.

### B.2 Technical listing versus capability — and where the real contest is

The Australian control, as it currently stands, is **technical**: it catches what is *listed* in the DSGL by technical specification (§B.1). It is **not** a capability control — it does not catch a thing merely because that thing keeps information confidential. This is visible in the operative regulation itself: the permission criteria in reg 13E(4) are about *strategic risk* — whether goods may reach a sanctioned country, aid a WMD program, increase an adversary's military capability, or facilitate human-rights abuses — not about whether something provides secrecy. The whole apparatus is oriented to military and dual‑use *goods*, matched by *listed technical characteristics*.

So under the current law the position is strong and narrow: ZOSCII is not listed, because it has none of the technical characteristics the cryptography entries specify (no cipher, no key-length parameter — §B.1). To bring it within the current control, a regulator would have to show it matches a specific DSGL technical entry, and there is no entry whose parameters it possesses.

The genuinely different regime — the one that *would* reach a cryptographic-but-non-encryption confidentiality system — is a **capability** control: one that bites on the *effect* (keeping information confidential) regardless of mechanism or technical parameters. The author has been told, but has **not** personally verified, that some jurisdictions (France is the example cited to the author) lean this way. That is a *different kind of law* from the Australian technical-listing regime, and Australia has not enacted it. The contest, therefore, is not "does the current Australian control reach ZOSCII" (on its technical-listing terms, it does not) but "would a jurisdiction move to, or already have, a capability-defined control." Against a capability control the §B.1 argument does not help, because parameters were never the trigger.

The author's position on the current Australian law: it is hard to bring a register-indirect memory load within a list written around encryption strength parameters, when the mechanism has no such parameter to match — and hard to maintain that a thing is "encrypted" when it was never decrypted to be read, the read being the decode and the decode a lookup. This is stated as a position, and the author accepts it may have to be tested. Keeping the protocols and implementations strictly free of any encryption operation is a deliberate design choice, and forcing any control to justify itself by capability rather than by technical match is one of its reasons.

### B.3 Other jurisdictions

Regimes differ in *kind*, and the distinction that matters is **technical-listing versus capability**. A technical-listing control (the current Australian model, §B.1–B.2) catches what matches specified technical parameters; a system with none of those parameters is not caught. A capability control catches confidentiality however achieved; being encryption-free does not help against it. Being encryption-free places ZOSCII outside a technical-listing control whose entries are cipher parameters; it does **not** place ZOSCII outside cryptography, nor outside a capability-defined control. The author has reviewed the current Australian instrument (*Customs (Prohibited Exports) Regulations 1958*, reg 13E, incorporating the Defence and Strategic Goods List by reference) and characterises it as technical-listing; the author has **not** reviewed the law of other jurisdictions and makes no claim about any of them, including the capability-style regime described to the author as existing elsewhere. See counsel per the disclaimer above.

### B.4 A note for travellers

**Not legal advice — do your own due diligence and check the laws of any country you plan to travel to before you go.** Many countries have export restrictions on encryption. ZOSCII and the UNSIGNAL Protocol **are not encryption** — by definition, not by argument (see above): they are index systems, and the methods are not secret — they are published, public, and free to everyone. Import or use restrictions on cryptographic tools are less common than export restrictions, but some countries do regulate them, so confirm for your destination rather than assume. If a country restricts the *export* of secure-encoding software, a cautious option is to uninstall or delete the software before you leave and reinstall it in your home country — though note that in some jurisdictions possession or prior use, not just carrying software across a border, can be the issue. When in doubt, consult qualified counsel for the specific country.