# ZOSCII / UNSIGNAL - CSP & Sudoku Framing (working notes)

**Purpose:** resume point for the "constraint-satisfaction / sudoku" way of
proving/explaining ZOSCII security, as a companion to the existing
information-theoretic proofs (I(M;A)=0, x=U). Pick this up later - not finished.

---

## The core idea (one line)

ZOSCII/UNSIGNAL security is a **solution-counting** statement: the address
stream is a constraint system with ~256^n satisfying assignments (one per ROM
interpretation). A constraint system with that many equally-valid solutions
carries **zero information** about which was intended. Same conclusion as
I(M;A)=0, reached from combinatorics / constraint-satisfaction instead of
Shannon information theory. Two independent routes to the same result.

Security property is **non-uniqueness / no gradient**, NOT computational
difficulty. "Not hard to solve" - "no unique solution to solve toward."

---

## Why the proof types are what they are

Only two valid proof categories for ZOSCII:

1. **Empirical** - the `ent` tests. Demonstrate no detectable structure (high
   entropy, uniform-looking output). Can't PROVE I=0 (no finite test proves a
   distributional negative) but falsifies the "it leaks" alternative by finding
   nothing.
2. **Mathematical (information-theoretic)** - Shannon-adaptation -> I(M;A)=0,
   and the x=U (universal set) formulation. This is the actual proof, in the
   correct domain.

**Z3 / SMT is categorically the WRONG tool** - discovered first-hand by trying
to build the proof and hitting the wall. Z3 proves *satisfiability over
constraints*; I(M;A)=0 is a statement about *mutual information across a
distribution*. Different mathematical objects. Z3 can model the algorithm's
constraints (key->output) and report "collisions / leakage" - but that answers
a satisfiability question, not an information question. So a Z3 "leakage"
analysis is confused: right tool for the wrong claim. (This became the "Z3
Proofs and ZOSCII/UNSIGNAL" LinkedIn article.)

Key line: the Z3 solver modeled the Private Key as a variable it could freely
choose - but the attacker cannot read the key. It's a proof about a system the
attacker never has access to.

**BUT** - CSP / solution-counting IS a legitimate second framework (unlike Z3),
because CSP theory *does* have vocabulary for "how many solutions" and "when is
it unique" - which is the actual security question.

---

## The brute-force / search argument (why searching fails on two grounds)

Attacker's decision tree, and where each step dead-ends:

1. Stream could be random noise / ZOSCII primitive / UNSIGNAL / microZOSCII -
   all look identical. No test distinguishes them (all high-entropy). Can't even
   classify the target before attacking.
2. Assume bare ZOSCII (weakest). You have a stream of addresses - but indices
   into a ROM you don't have.
3. Enumerate all ROMs? A 64KB ROM = 2^524288 ~= 10^157,800 possibilities.

Scale check: every atom in the observable universe (~10^80) as a computer, each
10^18 ops/sec, for the age of the universe (~10^18 s) = ~10^116 total ops. You
need 10^157,800. Short by ~10^157,684. Not "a long time" - physically
impossible.

**Rate is a red herring.** 1000/sec, 1M/sec, infinite/sec - doesn't matter:
- Finite speed: never finish (unreachable count).
- Infinite speed: finish instantly and learn nothing - you've produced EVERY
  possible message, all equally valid, no signal saying which is real. No
  success condition to stop at. No gradient toward the truth.

The search fails on TWO independent grounds: infeasibility AND non-verifiability.
Either alone kills it.

---

## The sudoku framing (the intuition, and the real maths behind it)

### Primitive vs UNSIGNAL as sudoku boards
- **ZOSCII primitive on structured data = sudoku WITH clues.** If you encode
  documented structure (e.g. raw hard-disk sectors: known boot-sector sigs,
  filesystem layout, fixed-format directory entries), the format documentation
  IS known-plaintext - it fills in "clues." Fixed mapping + clues => potentially
  solvable puzzle. This is the one case the bare primitive isn't for.
- **UNSIGNAL = BLANK sudoku board.** No clues. You don't know where to start
  (variable offset = unknown origin), don't know the boundaries (random
  prefix/suffix), don't know the mappings (per-session variable). Any valid
  completion possible. Not just empty - *un-frameable*: you can't even set up
  the puzzle because you don't know where cell (0,0) is.

### Key insight: it's ambiguity, not difficulty
An empty sudoku isn't "trivially secure" - a blank 9x9 has ~6.67x10^21 valid
completed grids and (with no clues) no basis to pick which was intended. Even
ordinary sudoku with zero clues is unsolvable-toward-unique. Not hard-to-compute
- non-unique. ZOSCII board (256x256 = 64KB squares) is this at 10^157,800 scale
with even WEAKER constraints (only self-consistency: repeated address -> same
value). Those constraints never ELIMINATE a candidate message - every message
has a self-consistent board - so propagation has nothing to converge on.

### Chess vs sudoku (Julian's distinction - this is the ZOSCII property)
- Chess = forced sequences, deterministic moves, uncertainty is about
  opponent's choice, state is certain.
- Sudoku = uncertainty that COMPOUNDS the further you go on a guess. In a
  SOLVABLE sudoku uncertainty REDUCES as you fill cells (each certainty
  constrains neighbours). In ZOSCII uncertainty does NOT reduce - filling one
  cell tells you nothing about others (no rule links them), and you never hit a
  contradiction (every completion valid), so you can't prune. It's the
  compounding-uncertainty feeling of hard sudoku WITHOUT the convergence.

### Skill-ladder note (context)
Easy sudoku = scanning (row/col/box, 1-2 level elimination) = "seeing."
Advanced (X-wing, swordfish, XY-chains, colouring, forcing nets) = "like chess"
= searching hypothetical branches. ZOSCII is BEYOND the chess wall: a search
where no branch ever fails (every line "works"), so it never resolves.

---

## Real, citable maths that applies (verified)

### Sudoku minimum-clue work (McGuire, Tugemann, Civario 2012; arXiv:1201.0749; Nature)
- Proved 17 is the minimum clues for a unique 9x9 sudoku (no 16-clue unique
  puzzle exists). Exhaustive computer search via **hitting-set enumeration**.
- **Unavoidable sets** = the key concept that maps to ZOSCII: sets of filled
  values which, when interchanged, still satisfy all constraints -> produce
  multiple solutions. A grid that's a union of disjoint unavoidable sets needs
  >= that many clues for uniqueness.
- **ZOSCII = the limiting case: the ENTIRE grid is one giant unavoidable set.**
  Every value interchangeable (every ROM), all satisfy constraints, nothing
  clued (ROM secret) => maximum solutions (256^n) => zero information.
- **The elegant sub-result to borrow:** "<=7 clues cannot be unique, because
  two unclued symbols are always interchangeable in any solution." That is the
  ZOSCII argument in miniature: unclued values are interchangeable ->
  non-unique -> no information. ZOSCII maximises it (ALL values unclued).

### Important framing caveat
Mathematicians frame the sudoku minimum-clue result as **combinatorics /
solution-counting, NOT information theory** ("which arrangements of N clues
admit exactly one completion"). That's HELPFUL: it confirms CSP/solution-counting
is a *separate* route from Shannon, and the two CONVERGE for ZOSCII
(combinatorics: count assignments; info theory: measure mutual information;
both give "many solutions / zero info"). Transfer the CONCEPTS and proof
PATTERNS (unavoidable sets; interchangeability -> non-uniqueness;
solution-counting), NOT the specific "17" (that's 9x9's particular structure).

### Euler / Graeco-Latin squares (open thread - not resolved)
- Euler's 36 officers problem: no Graeco-Latin square of order 6 exists
  (conjectured by Euler, proved impossible by Tarry 1901). Order 6 is the
  exceptional case (exist for all orders except 2 and 6).
- Connection to ZOSCII is the **solution-counting spectrum**: zero solutions
  (Euler order 6, unsatisfiable) / one (proper sudoku) / many (under-clued
  sudoku, and ZOSCII at the 256^n extreme). Same underlying question -
  "how many configurations satisfy the constraints" - different answers.
- **UNRESOLVED / to pick up:** which specific Euler-square property Julian is
  connecting. Three candidates raised, not yet settled:
  1. solution-counting (general framework),
  2. impossibility / exceptional-order (order 6 has zero solutions),
  3. **orthogonality** of the two superimposed Latin squares (rank-arrangement
     independent of regiment-arrangement) - flagged as possibly the APTEST,
     because orthogonality/independence of two superimposed structures is close
     to ZOSCII's address/value independence. **START HERE next time.**

---

## The "Different City" / LLM parallel (already in security-proofs.md)

The existing security-proofs.md "Different City" principle (credit: Filiip's LLM
information-theory work) is the SAME non-uniqueness property from another angle:
wrong ROM -> every address points elsewhere -> teleported to a completely
different message space, no path back, no way to tell which "city" was intended.

- "Different city, no path back" == "different completed sudoku, no rule forcing
  one over another." Same fact: every ROM/board equally valid, zero signal
  picking one.
- LLM/divergence framing captures the SENSITIVITY (one wrong choice -> totally
  different output); sudoku captures the NON-UNIQUENESS (every choice equally
  valid). Together = full picture.
- The doc's "256^n valid interpretations" line IS the solution-count of the
  constraint system - the CSP framing is already latent, just not named.

---

## Uniformity is NOT required (settled - keep straight)

Security = **blind selection + secret ROM** (address chosen WITHOUT reading the
value; ROM secret; so address ambiguous across ALL values). The selection
DISTRIBUTION (uniform or skewed) plays NO part.
- Uniform selection -> flat-looking output = UNDETECTABILITY (looks like noise),
  a side effect, NOT the security.
- Skewed selection -> skewed output, STILL I(M;A)=0, because address still
  ambiguous across all values without the ROM.
- Frequency analysis on addresses reveals the SELECTION distribution (info about
  the encoder's choice), NOT the message (address->value mapping is secret).
- security-proofs.md already scopes this correctly (2026-07-25 scope note;
  "output looks uniform is undetectability, not the security itself").

---

## To do / open threads when resuming
1. **Euler orthogonality angle** - settle which property Julian means; develop
   the address/value-independence <-> Latin-square-orthogonality link (likely
   the strongest CSP connection).
2. Draft the **CSP / solution-counting proof** as a formal companion to
   security-proofs.md: "address stream = constraint system with 256^n satisfying
   assignments (unavoidable-set argument) => zero information about intended
   assignment." Two independent routes (combinatorics + information theory) to
   I(M;A)=0.
3. Consider folding a short "constraint-satisfaction view" section into
   security-proofs.md, citing McGuire et al. 2012 (arXiv:1201.0749) for the
   unavoidable-set concept.