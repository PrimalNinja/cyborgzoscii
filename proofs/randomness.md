# Randomness for Keys, Not for Fairness

**Author:** Julian Cassin
**Date:** 2026-07-25
**Version:** 1.0 (DRAFT)

**Scope:** why ZOSCII wants *skewed, physically-captured* randomness rather than *balanced, generated* randomness — and why the encryption industry's demand for "perfect uniform randomness" is both unachievable and aimed at the wrong target. This is a position paper, not a proof. The formal `I(M;A)=0` result is in the companion *Zero Mutual Information* document; that proof requires **no** randomness quality of any kind (see its §4). This document explains what randomness quality actually buys, which is a separate thing from security.

---

## 1. The two questions people never separate

"Do you have good randomness?" is really two questions wearing one coat:

- **Is it unpredictable?** — the draws cannot be foreseen (independence, no small algorithm-expanded seed to regenerate them, no period).
- **Is it balanced?** — the outcomes come out evenly across the possibilities (uniform distribution).

The industry treats these as one requirement and spends billions chasing both at once — TRNGs, hardware security modules, whitening circuits — all to make the "dice" perfectly fair *and* perfectly unpredictable. The central claim of this document is that **these two properties are in tension, cannot both be had exactly, and only one of them matters for a key.** For protecting data, you want unpredictability and you do not want balance. Balance is a requirement for *fairness*, and fairness is a different job.

---

## 2. Balance and independence cannot both be exact

This is a theorem, not a preference.

Take five possible values `1,2,3,4,5` and draw independently. First draw `1`. Second draw `4`. Third draw — if the draws are genuinely independent, the third draw *must* be free to be `1` again, and sooner or later it is. The moment it repeats, the sequence is no longer balanced. You cannot forbid the repeat without making the third draw depend on the first two — and a draw that depends on what came before is not an independent random draw. It is a deal from a shrinking deck.

So there are exactly two ways to get a perfectly balanced sequence, and both destroy randomness:

- **Know the length in advance and force the counts.** Then you are arranging, not sampling.
- **Cycle: iterate so each pass eliminates every imbalance.** Then the sequence is a permutation, and within any incomplete cycle it is *skewed by construction* — the values already used are exhausted, the unused ones are pending, so the available options are never equally likely mid-cycle. Uniformity exists only retroactively, once the cycle closes.

**Uniformity is a property of a completed cycle, never of a partial sequence.** And a completed cycle is predictable at the end — once you have seen `n-1` of `n` values, the last is certain. Balance and unpredictability trade off against each other continuously; you buy one with the other.

The rigorous form of this is established in the mathematics of unfair dice: the distribution induced by rolling a (possibly unfair) die infinitely often is *singular* — concentrated on a measure-zero set — and cannot be massaged into the fair distribution without adding external structure (Pfeffer, Smith & Severa, *Induced Distributions from Generalized Unfair Dice*, arXiv:2309.07366; and Pfeffer et al., *Advantages of imperfect dice rolls over coin flips for random number generation*, Scientific Reports 15:11818, 2025). Their motivation is hardware TRNGs for probabilistic computing, not cryptography — but the core result transfers: **you cannot extract clean uniformity from independent draws, and more-sided (dice) sources degrade less under unavoidable bias than two-sided (coin) sources.** The nibble and byte alphabets ZOSCII uses are the many-sided case; a bit is the coin.

Anything sold as *both* perfectly balanced *and* perfectly random has been **post-processed** — von Neumann debiased, whitened, run through an extractor. That processing is deterministic and known, and it trades away exactly the independence it claims to preserve. Massaged is not random. It is conditioned, and conditioning is information added from outside — the very entropy you just removed.

The ugly output is the certificate. A truly independent source *must* assign nonzero probability to every finite sequence, including a run of all `1`s, however unlikely. A source that *cannot* produce an ugly run is a source with a constraint, and a constraint is dependence. The possibility of the ugly run is the proof the source is honest. A generator that never embarrasses you is lying.

---

## 2a. The claim is about randomness, not about secrecy

Before going further it is worth separating two layers, because they are easy to conflate and the argument is cleaner when they are kept apart:

- **The randomness claim.** Balance is not the goal. A skewed source is not a defective source. The instinct to drive randomness toward uniformity is a category error about what randomness *is*. This claim is domain-independent — it is a statement about randomness, prior to any use it is put to.
- **The applications.** Secrecy is one task that needs randomness. It is not the only one, and the randomness claim does not belong to it. Moment estimation, Monte Carlo sampling, load balancing, and hardware RNG design each need randomness too, and in several of them a *skewed* source has been proved to strictly outperform the balanced one — for reasons that have nothing to do with secrecy.

That the same "balance is not the goal" fact surfaces independently across unrelated tasks is itself the strongest evidence for it. It is not special pleading invented to justify skewed ROMs; it is a general property of randomness that the secrecy application inherits. Four instances, each with its *own* sense of "skewed outperforms," unified only by the randomness claim above:

- **Streaming / dimension reduction.** For estimating the frequency moments and Shannon entropy of a data stream, *maximally-skewed* stable random projections provably beat symmetric ones, cutting sample complexity sharply near the first moment (Ping Li, *Compressed Counting*; skewed stable random projections). Here "skewed outperforms" means *lower sample complexity for the estimate* — the maximally-asymmetric stable distribution has an analytic property the symmetric one lacks.
- **Hardware RNG.** For true-random generation from imperfect physical devices, more-sided ("dice") sources degrade less under unavoidable bias than two-sided ("coin") sources (Pfeffer et al., 2025; §2). Here "skewed outperforms" means *closer to usable randomness under real-device imperfection*.
- **Clinical inference.** Randomisation's purpose is not to produce balanced groups — if balance were guaranteed, the standard analysis would be *invalid*; randomisation exists to license the error estimate, and imbalance is inevitable and fine (Stephen Senn, *Randomisation is not about balance… but about randomness*, 2020). Here the point is the sharpest statement of the layer itself: *balance was never the goal; randomness was.*
- **Secrecy.** For a key, an *unknown* skew denies an adversary a model they could otherwise bank on (§4). Here "skewed outperforms" means *harder to predict without the key* — the sense developed in the rest of this document.

These are four different meanings of "skewed beats balanced," and it would be an error to treat them as one mechanism — Ping Li's is not about unpredictability, Senn's is not about entropy, and neither is about an adversary. What unifies them is only the layer above: **across tasks that need randomness, the drive to balance is a lazy default rather than an optimum.** Secrecy is where getting it wrong is most expensive, which is why the rest of this document is about that case. But the foundation is the domain-independent claim, and the breadth is what makes it solid.

---

## 2b. Four kinds of "random," on two axes

"Random" is one word doing several jobs, and the standard vocabulary collapses distinctions that matter. In particular, **CSPRNG** names an *implementation* (cryptographic, seeded, deterministic underneath) and says nothing about *why* it behaves as it does for this argument. The property that actually differs is better named **fair** or **balanced**. But even that is only one of two independent questions. Pulling them apart:

**Axis 1 — is the source fair, or merely tending fair?** Does each individual draw come from an even distribution over the possibilities? This is a property of the *source*. "Fair" / "balanced" names it; "CSPRNG" is a mechanism label that happens to aim at it.

**Axis 2 — where does the balance live: enforced at the source, or emergent in the results?** This is the sharper cut and the one no standard term captures:

- **Enforced every step.** No matter where you stop, the counts are even. This *requires* each draw to depend on what came before — it is the shrinking deck, the cycle, the conditioned source of §2. Balanced at every prefix, and therefore (by §2) *not independent*. The balance is a rule imposed on the source.
- **Emergent in the results.** Each pick is an honest independent draw from whatever the source is. Imbalance appears as fluctuation and only averages out over the run, if at all. The balance, when it appears, is a *description of accumulated output*, never a constraint on the draw. Nothing was enforced.

Crossing the two axes gives a 2×2, not a line:

| | **Source balanced** | **Source skewed** |
|---|---|---|
| **Balance enforced every step** | Forced / conditioned — the whitened "perfect RNG" ideal. Balanced source *and* balanced at every prefix. The most-forced cell, and not independent. | Incoherent: you cannot hold even counts out of a skewed source without conditioning, and conditioning destroys the skew. This cell collapses into the one to its left. |
| **Balance emergent in results** | Fair independent draws from a fair source — the honest coin. Balanced *on average*, freely, with real fluctuation at every finite prefix. | **Fair independent draws from a skewed source — the ROM case.** No enforcement anywhere; the source is as skewed as it happens to be; whatever balance-or-not is simply what accumulated. |

Three things fall out of the grid:

- **The industry sells the top-left and calls the whole row "random."** "Perfect randomness" almost always means *balanced source, balanced every step* — the most-conditioned cell, the one furthest from independent. The better it is forced, the less free it is (§2). That is the cell to avoid for a key.
- **The bottom-right has no standard name, and it is where ZOSCII lives.** Independent draws, no enforcement at any level, from a source that is itself skewed. Nobody forced anything anywhere. This is the honest cell, and the skew is a feature (§4), not a compromise on the way to uniformity.
- **The right column's top cell is genuinely incoherent, and that is informative.** You cannot *enforce* balance from a skewed source without conditioning it back toward balanced — which is just moving left. "Skewed but forced even" is not a thing. Skew and enforcement are opposites, which is another face of §2.

### Implementation note: balance-by-construction (banker's rounding)

Real implementations sometimes bake balancing into a *mechanism* rather than into a source, and it is easy to mistake the one for the other. A clear example that has nothing to do with randomness at all: **banker's rounding** (round-half-to-even), the default in VB6 and many financial systems. On a `.5`, it rounds **up or down depending on whether the preceding digit is odd or even** — odd rounds up, even rounds down, so the result always lands on the even neighbour: `0.5`→`0`, `1.5`→`2`, `2.5`→`2`, `3.5`→`4`. The purpose is precisely aggregate balance: always-round-half-up accumulates a systematic upward bias across a summed column, whereas sending odds up and evens down makes the two directions roughly cancel over many values, so the total does not drift.

That is balance engineered into the *rule*, appearing only in the *aggregate*, present in no individual result — an Axis-2 *enforcement* wearing the appearance of neutrality. It is the same shape as the enforced-balance cell of the grid: the evenness is a construction, not a property of the inputs. The reason to name it here is the mistake it invites — reading engineered aggregate balance as if it were a quality of the underlying values. In rounding that mistake is harmless; in a *randomness source* the identical mistake (two biased sub-streams arranged to cancel in the statistics, passing aggregate tests because the forcing cancels, not because the source is clean) is exactly the hidden conditioning that makes an implementation's "randomness" a mechanism artifact rather than a captured fact. For a key that is doubly wrong: forced (so not independent), and the forcing rule is itself exploitable structure if known. Balance-by-construction is a general trick; recognising it for what it is — enforcement, not source quality — is the point.

### This is not claimed to be the complete taxonomy

Two axes and a 2×2 are enough to separate the cases this document needs, but they are almost certainly not the whole space. There may be further axes (rate vs quality, per-draw vs blockwise, certified vs asserted, static source vs streaming), other useful theories of what "random" means, and implementation behaviours that do not sit cleanly in any cell. The grid is a tool for making "CSPRNG is the wrong instrument for a key" *precise* — it is the enforced/balanced cell, and a key wants the free/skewed one — not a claim to have enumerated randomness. Treat it as open.



---

## 3. Fairness is the wrong target for a key

Balance is what you want when the randomness is the *product* — a lottery draw, a dice game, a fair sample, anywhere a party is entitled to an unbiased outcome. There, bias is the flaw.

A key is not a product anyone consumes. Nobody rolls a ROM and receives an outcome they have a stake in. The ROM's only job is to be **unmodelable by an adversary**. And here bias — *unknown* bias — is not a flaw. It is protection.

**We are trying to protect data, not fairness.** The entire TRNG/HSM industry optimises for fair output because its mental model is "randomness as fair outcome." For a key, that model is not merely unnecessary — it is counterproductive twice over. Chasing uniformity spends key entropy (a flatter ROM is a smaller, more structured subset of all possible ROMs), and it hands the adversary the one thing that helps them: a *known* distribution to model against.

---

## 4. Why balanced randomness is a gift to the attacker

Frame everything from the adversary's side, because the adversary is who the key is kept from.

The adversary's residual uncertainty is `H(M | their model)`. If they hold the *true* distribution — because it is uniform and everyone knows what uniform is — their uncertainty is exactly the honest entropy of the source and no more. They model you perfectly.

If instead the distribution is skewed and they do **not** know the skew, they are forced to model with a wrong distribution `q` while the truth is `p`. Their effective uncertainty becomes the cross-entropy `H(p) + D_KL(p‖q)` — strictly *larger* than the balanced case by the `D_KL` penalty, and that penalty is real predictive loss they suffer on every draw, permanently, until they learn the true distribution — which, for a secret ROM, they never do.

So the loaded dice nobody has measured are harder to call than the fair dice everybody understands. The unpredictability is not in the dice; it is in the observer's ignorance of *which* bias each die carries. Ten fair dice: everyone has the model, perfectly predictable in distribution. Ten dice each with a different, unmeasured bias: the observer has no model and cannot build one from the outside.

This is exactly why a CSPRNG is the wrong selection rule for ZOSCII, and it is the point that only makes sense once you have stopped thinking like an encryptor:

**A CSPRNG makes balanced, uniform selections — which means it hands the attacker a clean assumption: "the selection is drawn evenly."** That is one fewer unknown for them. A plain `rand()`, with its modulo bias and period artifacts, makes selections that are *skewed in a way the attacker cannot characterise*, because the skew is convolved with the ROM's per-value address counts — and the ROM is precisely what they do not have. The observed distribution is `rand()`'s skew times the ROM's structure, and the ROM term is missing from their side of the equation.

Uniform selection *removes* uncertainty from the attacker's model. Skewed-in-an-unknowable-way *denies them even the assumption of uniformity*. The "defect" of a cheap RNG — non-uniformity — is not a defect here. It is an extra layer of structure the attacker cannot model, sitting on top of a possibility space they already cannot see.

The same logic applies one level up, to the ROM itself. **ZOSCII ROMs deliberately do not have an even spread of every value.** Uniform ROM composition would be the same gift as uniform selection: a clean statistical assumption the attacker can bank on. A skewed ROM they cannot decompose gives them nothing to stand on. This is why a selfie, a PDF, or an MP3 works as a ROM and works *better* than a crafted flat one — natural files are skewed, and the skew is not a bug to be corrected. It is concealment.

---

## 5. Captured, not generated: the seed is the vulnerability

Unpredictability has a stronger and a weaker form, and the difference is provenance, not statistics.

A PRNG — however sophisticated — is a finite-state machine following deterministic rules. It has a seed, a period, and structure. It *will* cycle. Its total entropy is bounded by its seed: feed it 128 bits of seed and there are only `2^128` possible streams no matter how long they run. The "randomness" is an illusion maintained only against an observer who lacks the seed. Whether the adversary finds the structure before the system is retired is a bet, not a guarantee — and "computationally secure" is exactly that bet, stated politely.

A physical capture — thermal noise on a camera sensor, photon shot noise, the acoustic physics in a recording — is a *measurement of the world*. It does not simulate randomness; it captures it. The underlying events are quantum-level and genuinely irreducible: not "hard to predict" but *fundamentally indeterminate*. The precise claim is not "no seed" — everything has an origin, and captured entropy has one too. The claim is **no small, algorithm-expanded seed that regenerates the whole output.** A PRNG's 128-bit seed regenerates gigabytes: the stream has only 128 bits of true entropy however long it runs, and those bits are the entire attack surface. A physical capture has no such compact regenerator — its "seed" is the measurement itself, as large as the output, with no shorter description than itself. Seed and content are the same object, so there is nothing smaller than the output to search.

This is the property a key actually needs. Not "high entropy" as a number — a good PRNG scores 7.999 on `ent` too — but **no small seed for an adversary to target.** You cannot guess 128KB of captured physics, and not because the search is infeasible: because there is nothing to iterate toward. No structure makes one guess warmer than another, and no compact seed collapses the space. Guessing it is not a hard operation; it is not a defined operation.

This is how the seed is actually built in practice. Neptune (using the NuGet classes) gathers its seed by **XOR-aggregating captured environmental state** — memory contents at the moment, mouse travel, number of button clicks, the timer, the number of files in certain folders, and a slew of other dynamic measurements. Each is a *measurement*, not an algorithm's output; the aggregate is a fingerprint of an unrepeatable instant across many dimensions, with no compact value that regenerates it. That is the captured-seed pattern exactly: to reconstruct it an adversary would have to reproduce the entire environmental state at that instant, not search a small key space. One honest caveat, worth stating rather than glossing: some individual sources are weak or partially observable — a click count is low-entropy, folder file-counts may be guessable, timer resolution is limited. The security does not rest on any single source. It rests on (a) XOR-aggregation pooling many independent sources so the *combination* is unpredictable even where components are not, and (b) the same principle as the timer below — the seed's job is entropy-gathering and freshness, while the security-relevant entropy ultimately lives in the ROM. Even a partially-estimated seed does not hand an adversary the ROM.

Compression does not change this. An MP3 or JPEG is *compressed physics* — a lossy transform of measured quantities, repacked denser. The DCT and quantisation do not turn a measured quantity into a computed one. Compressed physics is still physics.

`ent` cannot see the difference between captured physics and a good PRNG — both score near 8 bits/byte. That is exactly why **provenance is the claim and `ent` is only corroboration.** A clean `ent` score says "no randomness test flags this file," which is undetectability, not unguessability. Undetectability is real and useful (nothing for an observer to grab), but it is a weaker property than "no compact regenerating seed," and the two should not be conflated.

### 5.1 "Cryptographically secure" names a bet, not a fact

The industry term for a generator whose output withstands scrutiny is "cryptographically secure" (CSPRNG). Every load-bearing word in it is an *imitation* claim flattened into a *nature* claim.

The concept is due to Blum and Micali and to Yao (1982–84); Blum and Micali's original paper is titled "How to Generate Cryptographically **Strong** Sequences of Pseudo-random Bits" — "strong," a hedged property of the bits, which the field later flattened to "secure," an apparent property of the system. The precise definition: a generator is cryptographically secure if no polynomial-time test can distinguish its output from true randomness — no efficient algorithm predicts the next bit better than chance. So "secure" means, unpacked, *unpredictable to a computationally-bounded adversary under an unproven hardness assumption*. Blum–Micali "get their security from the difficulty of the discrete logarithm"; Blum-Blum-Shub from factoring. Each is a conditional: **IF** the hard problem stays hard, **THEN** the output is indistinguishable. The implication is a genuine theorem and remains true; its antecedent is not guaranteed, and the founders said so plainly.

Two things the term smuggles:

- **"Secure" leaks outside the frame.** Inside cryptography it is well-defined and conditional. In ordinary reading "secure" sounds like *safe, period* — dropping the "if the assumption holds." Quantum computing is that expiry: Shor's algorithm breaks discrete log and factoring, so the antecedent fails and the theorem — still true as an implication — delivers nothing.
- **"Indistinguishable from random" is not "random."** It is a deterministic sequence no *bounded* observer can catch being deterministic. Underneath sits a seed, a period, a structure. The randomness is a costume; "indistinguishable" means only that no efficient inspector, so far, catches it. This is the undetectability-vs-unguessability split above: a CSPRNG is perfectly undetectable and perfectly guessable-with-the-seed, and its entire security is that nobody has the seed.

A subtlety worth stating exactly, because it turns the term against itself: true randomness is *skewed* at any finite length (§2), so a source that comes out *dependably balanced* is thereby *distinguishable* from random — its cleanliness is a signature true randomness lacks. "Reliably balanced" and "indistinguishable from random" are therefore in tension: to imitate randomness a generator must reproduce its skew, including the occasional ugly run. So "balanced random" is not merely loose, it is self-defeating — the more dependably balanced the output, the less it looks like the real thing.

### 5.2 The Dual_EC_DRBG back door — the failure mode, documented

The risk this section describes is not hypothetical. It has a name, a standard, and a proof. Dual_EC_DRBG was a NIST-standardised "cryptographically secure" PRNG built on elliptic curves, mandated for FIPS-140 certification. Thomas Hales, in the AMS *Notices* (2014), gives the elementary mathematics: the standard published points `P` and `Q` in a cyclic group of prime order, so `P = e·Q` for some `e`; anyone holding `e` recovers the generator's secret internal state from its output — Shumow and Ferguson showed 32 bytes of output sufficed to pin the state uniquely. The back door is universal: one number `e` gives its holder access to every user's generator worldwide. (Hales, "The NSA Back Door to NIST," *Notices of the AMS* 61(2), 2014, https://www.ams.org/notices/201402/rnoti-p190.pdf.)

Three points make this the exhibit for everything above:

1. **A thing bearing the exact label "cryptographically secure" was compromised by design.** The label and the back door were simultaneously true. "Secure" certified a generator engineered to be broken by its author.
2. **The back door *is* Diffie-Hellman, run against the user unawares.** The published data lets a "Spy" holding `e` and the user's output complete a Diffie-Hellman exchange the user joins "innocently and unwittingly." This is the worst case of the caution in the proof document's Appendix A: a computational key-exchange does not merely cap security at the computational level — its structure can be *authored by a third party* to share your secret with them.
3. **The invertibility question was undergraduate-level and nobody asked it.** Hales notes the back door could be rediscovered as "an undergraduate homework problem," and that every professional trained to analyse cryptographic algorithms knows to ask about invertibility — yet the standards process did not. The one-step check, skipped by the institution whose mandate was to make it.

The contrast with a captured-entropy ROM is total: no mandated parameter set to trust (the entropy is your own capture), no small algorithmic seed to recover (captured physics has none — its seed is the measurement itself, as large as the output), no hardness assumption to expire (`I(M;A)=0` is unconditional). The failure mode Dual_EC embodies is precisely the one the design forecloses. Honest boundary: this shows the *thing ZOSCII avoids* is real and has bitten the entire industry; it is not itself a proof that ZOSCII is secure — that proof is separate and mathematical.

A verified aside on scale: Knuth's *The Art of Computer Programming* devotes 193 pages — half of volume two — to random number generation (per Hales), and draws the same line this section draws: generators are either *truly random* (values from a physical process such as a quantum mechanical event) or *pseudo-random* (values from a deterministic algorithm displaying a semblance of randomness). Captured-vs-generated is the older distinction; this document only insists on which one a *key* requires.

### 5a. Applied: RandomROM selection

IMPORTANT NOTE: Below has an error in it which has been identified and will be corrected soon.  The theory is sound but the implementation stated below needs improvement - and yes, we already have an improvement in testing. Watch this space.

The principle above — use captured entropy you already hold, not a generator — has a direct application in ZOSCII's own selection step. Encoding a value means choosing one address among the positions holding that value; that choice needs randomness, and the obvious source is the system RNG. **RandomROM** replaces it with a walk through a *second secret ROM*: each symbol consumes the next RandomROM byte and uses it (XOR'd with the value, modulo the instance count) to pick the address, advancing a pointer whose start is set from a timer.

This is "captured, not generated" made concrete. The selection is driven by a ROM — captured entropy, no compact regenerating seed, no period — instead of a platform generator. Note carefully what it does and does not buy, because it is easy to overstate:

- **It does not add security.** The address never leaked the value regardless of how selection was driven; a secret ROM was already doing that job. The security was complete before.
- **What it removes is a *system dependency*, not a weakness.** The RNG call was the one place the encoder reached into the platform — a different algorithm on every language and version, and third-party code with its own exposure. RandomROM removes the call: the same arithmetic runs everywhere, so the encoder becomes constant across implementations, and there is no system RNG left to trust or vary.
- **It is trivial and portable.** Another ROM walk, reusing lookup code — implementable identically in C, C#, ES5, or on 1970s hardware. Plausibly as fast or faster than a system RNG, though that is a bonus, not the point.

RandomROM is the tidy end of this document's argument: not only is the *key* captured rather than generated (the ROM), but the *selection* is too (the RandomROM), so no generator appears anywhere in the pipeline. The timer that sets the walk's start carries no security — freshness only — for the same reason weak seed entropy never mattered: the ROMs carry everything. The mechanism's exact index and seed formulas live in the ZOSCII specification (`randomrom.md`); what belongs here is the principle it instances — the last generator in the system was replaced by a second captured source.

One entropy point belongs here, because it is where this document's "captured entropy" theme meets a size question. The RandomROM may be the value ROM itself (one secret, abundant entropy, nothing extra to hold) or a separate ROM — and if separate, it can be **much smaller** than the value ROM without weakening anything. The reason is the same fairness-vs-secrecy split that runs through this whole document: the value ROM *is* the key, so its entropy is unguessability and wants to be large; the RandomROM's bytes are never stored as a key — each is consumed immediately as a selector reduced modulo the instance count, a few bits at a time. Its entropy requirement is set by the selection task (drive a reduction over hundreds of instances), not the key task (be unguessable at key scale). Hundreds to thousands of values suffices — ideally not so small that the walk develops a short, predictable period, nor larger than the selection needs. Captured entropy is still the right source for it; there is just far less of it required, because it is doing the small job, not the large one.

---

## 6. Where the security actually comes from — and where it doesn't

Two independent things carry the security, and they are not the same thing:

**The ROM's unguessability** — provided by captured physics. No seed, no period, no shorter description. This is not "99% of the security" in the sense of a large slice that could be chipped at; it is a wall. `2^1048576` possible contents for a 128KB ROM, with no structure to exploit and no signal that any guess is closer than any other. It cannot be guessed. Full stop.

**Zero mutual information** — provided by blind selection (the address depends only on the position set, never on the value). This gives `I(M;A)=0` and holds for *any* ROM, even a PRNG one, even a flat one. This is the entire zero-leakage mechanism, and it is the subject of the companion proof.

These do different jobs. Physical capture makes the key **unrecoverable**; blind selection makes the addresses **carry no information in the first place**. Neither substitutes for the other:

- Perfect physical ROM, but deterministic selection (always the lowest instance) → the ROM is unguessable, yet repeated values produce repeated addresses and structure leaks. Security degraded despite perfect entropy.
- Perfect blind selection, but a known-PRNG-seed ROM → `I(M;A)=0` still holds against someone without the seed, but the seed is findable, so the key falls. Security gone despite perfect selection.

By *entropy budget*, essentially all the entropy is in the ROM (~a million bits) and the per-symbol selection consumes only `log₂|S_v|` bits (~8 at byte width). So "99% ROM, 1% selection" is close to literally true **as an entropy ratio**. But it is not a *security* ratio: the selection's tiny slice of entropy does 100% of the zero-leakage work, and the ROM's bulk does 100% of the unguessability work. Two absolutes, not a split.

The practical consequence: **spend your paranoia on the ROM** — not because it is the mathematical weak link (it cannot be guessed) but because it is the only thing that can be *lost operationally*. You cannot guess 128KB, but you can steal it, copy it, photograph it, or leave it on a drive. The mathematics on the ROM is unbreakable; the custody of it is the entire attack surface.

---

## 7. Two objections, answered

**"A CSPRNG output is indistinguishable from true randomness — so uniform has no 'signature' and skew buys nothing."**

Indistinguishability from random is exactly the *undetectability* property of §5, and it is true: a CSPRNG stream will not be flagged by a randomness test. But undetectability is not the claim. Two things the objection misses. First, the CSPRNG has a seed; captured physics does not — so "indistinguishable output" says nothing about *unguessability*, which is the property that matters for a key (§5). Second, and more subtly: the argument in §4 is not that uniform output has a statistical signature an analyst can *see*. It is that uniform selection hands the analyst a correct *model* — the assumption "selections are uniform" — which reduces their uncertainty by the `D_KL` term they would otherwise pay. The skew does not need to be *visible* to help; it needs to be *unknown*. A CSPRNG's uniformity is known by definition. That knowledge is the gift.

**"Skewed sampling beating uniform sampling is a known result in lots of fields — so this is just borrowing a slogan."**

The opposite: that it is a known result in many fields is the *point*, made in §2a. Skewed sources provably outperform balanced ones in streaming moment estimation (Ping Li), in hardware RNG under imperfection (Pfeffer), and the "balance was never the goal" correction is made independently in clinical inference (Senn). These are real and they *support* the randomness claim — because that claim is domain-independent, and its recurrence across unrelated tasks is exactly what shows it is not special pleading.

What must not be done — and this is the only care required — is to treat the *mechanisms* as one. Ping Li's skew lowers sample complexity; it is not about unpredictability. Non-reversible MCMC's skew converges ballistically rather than diffusively; it aims deliberately at the important direction, which for a key would be the *opposite* of what you want. Leverage-score and effective-resistance sampling beat uniform for matrix and graph approximation by weighting toward important components — again, deliberately predictable, the opposite of a key. So these are *not* evidence for the **secrecy** mechanism (§4); importing them there would be an equivocation on the word "skewed." They are evidence for the **randomness claim** (§2a), which secrecy then inherits as one instance among several. The layer matters: cross-field breadth supports "balance is not the goal"; it does not support "unknown skew defeats an adversary" — only §4 does that, and it stands on its own.

---

## 8. Summary

- Balance and independence cannot both be exact. Perfectly-balanced-and-random does not exist; anything claiming it has been massaged, and massaging is added structure, not randomness.
- Balance is a requirement for *fairness*. A key is not a fair outcome anyone consumes; its job is to be unmodelable. For that, unknown skew is protection, not weakness.
- Uniform selection and uniform ROM composition both *hand the attacker a clean statistical model* they can otherwise never obtain. Skewed-in-an-unknowable-way denies them even the assumption of uniformity.
- Physical capture beats generation not because it scores higher on `ent` — it does not — but because it has **no small algorithm-expanded seed**. Its seed is the measurement itself, as large as the output, with nothing shorter to search. Unguessability, not undetectability, is what a key needs, and only captured (incompressible) entropy provides it.
- The security is two absolutes: captured physics makes the ROM unrecoverable; blind selection makes the addresses carry zero information. `I(M;A)=0` itself needs no randomness quality at all — that is proved separately. This document is about what quality buys *beyond* the proof: a key with no compact seed to target and no distribution to model.
- Spend your paranoia on ROM custody. It is the only attack surface, because the math is not one.

---

*Companion documents: the formal `I(M;A)=0` result (Zero Mutual Information); ROM generation from physical entropy sources (ROMGenerator / UNSIGNAL repository).*