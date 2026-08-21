# RandomROM — Specification & Implementation Guide

**Author:** Julian Cassin
**Date:** 2026-08-20
**Status:** DRAFT — Implementation Guide Added

---

## 5a. Applied: RandomROM Selection

**IMPORTANT NOTE:** Below has an error in it which has been identified and will be corrected soon. The theory is sound but the implementation stated below needs improvement — and yes, we already have an improvement in testing. Watch this space.

The principle above — use captured entropy you already hold, not a generator — has a direct application in ZOSCII's own selection step. Encoding a value means choosing one address among the positions holding that value; that choice needs randomness, and the obvious source is the system RNG. **RandomROM** replaces it with a walk through a *second secret ROM*: each symbol consumes the next RandomROM byte and uses it (XOR'd with the value, modulo the instance count) to pick the address, advancing a pointer whose start is set from a timer.

This is "captured, not generated" made concrete. The selection is driven by a ROM — captured entropy, no compact regenerating seed, no period — instead of a platform generator. Note carefully what it does and does not buy, because it is easy to overstate:

- **It does not add security.** The address never leaked the value regardless of how selection was driven; a secret ROM was already doing that job. The security was complete before.
- **What it removes is a *system dependency*, not a weakness.** The RNG call was the one place the encoder reached into the platform — a different algorithm on every language and version, and third-party code with its own exposure. RandomROM removes the call: the same arithmetic runs everywhere, so the encoder becomes constant across implementations, and there is no system RNG left to trust or vary.
- **It is trivial and portable.** Another ROM walk, reusing lookup code — implementable identically in C, C#, ES5, or on 1970s hardware. Plausibly as fast or faster than a system RNG, though that is a bonus, not the point.

RandomROM is the tidy end of this document's argument: not only is the *key* captured rather than generated (the ROM), but the *selection* is too (the RandomROM), so no generator appears anywhere in the pipeline. The timer that sets the walk's start carries no security — freshness only — for the same reason weak seed entropy never mattered: the ROMs carry everything. The mechanism's exact index and seed formulas live in the ZOSCII specification (`randomrom.md`); what belongs here is the principle it instances — the last generator in the system was replaced by a second captured source.

One entropy point belongs here, because it is where this document's "captured entropy" theme meets a size question. The RandomROM may be the value ROM itself (one secret, abundant entropy, nothing extra to hold) or a separate ROM — and if separate, it can be **much smaller** than the value ROM without weakening anything. The reason is the same fairness-vs-secrecy split that runs through this whole document: the value ROM *is* the key, so its entropy is unguessability and wants to be large; the RandomROM's bytes are never stored as a key — each is consumed immediately as a selector reduced modulo the instance count, a few bits at a time. Its entropy requirement is set by the selection task (drive a reduction over hundreds of instances), not the key task (be unguessable at key scale). Hundreds to thousands of values suffices — ideally not so small that the walk develops a short, predictable period, nor larger than the selection needs. Captured entropy is still the right source for it; there is just far less of it required, because it is doing the small job, not the large one.

---

## RandomROM — Description

Drive selection from a walk through a **second secret ROM** instead of a system RNG. For each message value `v`, take the next byte `ev` from the RandomROM and select within `v`'s instance list `S_v` by an index computed from `v` and `ev` (e.g. `(v XOR ev) mod |S_v|`), then advance the RandomROM pointer. The walk's start is set from a timer (optionally XOR'd with ROM-derived values for a secret, fresh start); the timer is preferred but not mandatory, and additional seed sources give deprecating benefit.

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

## Implementation Guide — Front/Tail Flipping Method

### The Core Variables

| Variable | Description |
|----------|-------------|
| `iv` | Instance Value (8-bit random byte, 0 to 255) |
| `rv` | Random Value / Entropy (8-bit random byte, 0 to 255) |
| `X` | The combined chaos value (`iv XOR rv`), always between 0 and 255 |

### The Step-by-Step Logic

Instead of using a second modulo (which creates a blind spot in the middle of your 512 instances), use `X` directly as a directional step based on whether it is even or odd:

**When `X` is EVEN (Choose from the Front):**
- Count forward from the very beginning of your list.
- `Chosen Index = X`
- Range Covered: Instances 0 to 254

**When `X` is ODD (Choose from the Tail):**
- Count backward from the very end of your 512-instance list.
- `Chosen Index = 511 - X`
- Range Covered: Instances 256 to 511

### Why This Is the Best Approach

- **Reduced Bias / Increased Fairness:** Every single one of your 512 instances has an exact, identical 1-in-512 chance of being chosen.
- **Closes the Middle Gap:** Eliminating the second modulo removes the mathematical blind spot, meaning instances 255 through 341 are no longer skipped.
- **Preserves Pure Chaos:** Because both inputs are random, `X` is pure noise. This logic translates that noise directly into a seamless 512-slot selector with no code overhead, no multiplication, and no risk of crashing.

### Algorithm Summary

```
X = iv XOR rv                            (Pure 8-bit noise, 0 to 255)

if X is EVEN:
    selectedIndex = X                     (0 to 254)
else:
    selectedIndex = 511 - X              (256 to 511)
```

### For Instance Counts < 512

The Front/Tail method assumes 512 possible instances. For smaller `instanceCount`, fold:

```
selectedIndex = selectedIndex % instanceCount
```

This preserves fairness while adapting to any instance count.

### Two-Byte Extension (16-bit Space)

For higher precision or larger instance spaces:

```
X = iv XOR rv                            (Pure 8-bit noise, 0 to 255)
Y = Next_iv XOR Next_rv                  (A second independent 8-bit noise byte)

Combined_16Bit = (X * 256) + Y           (Raw 16-bit space, 0 to 65535)
Masked_16Bit   = Combined_16Bit | 0x8000 (Forced MSB, locks range to 32768 - 65535)

Final_Index = Masked_16Bit mod ic        (The Collision)
```

---

## Implementation in ZOSCII Encode

### RandomROM Walk State

```csharp
public struct RandomROMState
{
    public RomData RandomROM;    // The RandomROM data
    public long Pointer;         // Current position in the walk
}
```

### Initialization

```csharp
// Set pointer start from timer (freshness only, not security)
RandomROMState state = new RandomROMState
{
    RandomROM = randomRom.GetRomData(),
    Pointer = (long)(Environment.TickCount & 0xFFFF)
};

// Optional: XOR with ROM-derived value for additional freshness
state.Pointer ^= (long)(state.RandomROM.ptrROMData[0] | 
                        (state.RandomROM.ptrROMData[1] << 8));
```

### Get Next Random Byte

```csharp
private static byte GetNextRandomByte(ref RandomROMState state)
{
    // Advance pointer, wrap at ROM size
    state.Pointer = (state.Pointer + 1) % state.RandomROM.lngROMSize;
    
    // Return the byte at the current position
    return state.RandomROM.ptrROMData[state.Pointer];
}
```

### Encoding with RandomROM

```csharp
internal static byte[] zencodeByteToByte(ref RomData objRom_a, 
                                         byte[] arrInput_a, 
                                         ref RandomROMState randomRomState)
{
    byte[] arrResult = new byte[arrInput_a.Length * 2];
    int intPos = 0;
    
    for (int intI = 0; intI < arrInput_a.Length; intI++)
    {
        byte iv = arrInput_a[intI];
        uint intCount = objRom_a.arrLookup[iv].intCount;
        
        if (intCount == 0)
        {
            return null;  // Value not found in ROM
        }
        
        // Get next random byte from RandomROM
        byte rv = GetNextRandomByte(ref randomRomState);
        
        // Front/Tail Flipping
        byte X = (byte)(iv ^ rv);
        uint selectedIndex;
        
        if ((X & 1) == 0)  // Even
        {
            selectedIndex = X;
        }
        else               // Odd
        {
            selectedIndex = (uint)(511 - X);
        }
        
        // Fold to instance count
        selectedIndex %= intCount;
        
        // Pick the address
        ushort intAddr = (ushort)objRom_a.arrLookup[iv].ptrAddresses[selectedIndex];
        byte[] arrAddr = BitConverter.GetBytes(intAddr);
        arrResult[intPos++] = arrAddr[0];
        arrResult[intPos++] = arrAddr[1];
    }
    
    return arrResult;
}
```

---

## Integration with Existing API

### Option 1: Pass RandomROM as a Parameter

```csharp
// In ZEncode.Bytes:
public static byte[] Bytes(byte[] arrInput_a, ZOSCIIRom objRom_a, 
                           ZOSCIIRom objRandomRom_a = null)
{
    if (objRandomRom_a == null)
    {
        // Fallback to System.Random for compatibility
        RomData objRomData = objRom_a.GetRomData();
        return clsZOSCII.zencodeByteToByte(ref objRomData, arrInput_a);
    }
    
    // Initialize RandomROM state
    RandomROMState state = new RandomROMState
    {
        RandomROM = objRandomRom_a.GetRomData(),
        Pointer = Environment.TickCount & 0xFFFF
    };
    
    RomData objRomData = objRom_a.GetRomData();
    return clsZOSCII.zencodeByteToByte(ref objRomData, arrInput_a, ref state);
}
```

### Option 2: Chain with Tango

```csharp
// In ZEncode.Chain with Tango:
if (blnTango_a && arrRoms_a.Length > 1)
{
    // Build Tango ROM
    RomData objTango = clsZOSCII.BuildTangoRom(arrRoms_a);
    
    // Use the first ROM as RandomROM (or a separate one)
    RomData objRandomRom = arrRoms_a[0].GetRomData();
    
    RandomROMState state = new RandomROMState
    {
        RandomROM = objRandomRom,
        Pointer = Environment.TickCount & 0xFFFF
    };
    
    return clsZOSCII.zencodeByteToByte(ref objTango, arrInput_a, ref state);
}
```

---

## Decoder Is Unchanged

RandomROM is **encode-only**. The decoder remains:

```csharp
value = ROM[address];
```

No changes needed — the decoder never touches the RandomROM.

---

## Compatibility Notes

1. **Existing encoded files** are unaffected — RandomROM only changes how addresses are selected, not how they're decoded.
2. **Fallback mode** should be available for backwards compatibility with encoders that don't use RandomROM.
3. **Platform invariance** — with RandomROM, encode produces bit-identical output across all platforms (same ROM, same input = same output).
4. **No security loss** — `I(M;A)=0` holds regardless of selection source. RandomROM removes a system dependency, not a security gap.

---

## Testing

### Verify Correctness

```csharp
// Encode with RandomROM
byte[] encoded1 = ZEncode.Bytes(plaintext, rom, randomRom);

// Encode again with same ROMs — should be identical
byte[] encoded2 = ZEncode.Bytes(plaintext, rom, randomRom);
Debug.Assert(encoded1.SequenceEqual(encoded2));
```

### Verify Distribution (Statistical)

For each value, selection should be fair across its instances:

```csharp
// Count selections for each instance of value 0x00
int[] counts = new int[512];
for (int i = 0; i < 10000; i++)
{
    // Encode 0x00 with fresh RandomROM state
    byte[] result = ZEncode.Bytes(new byte[] { 0x00 }, rom, randomRom);
    ushort addr = BitConverter.ToUInt16(result, 0);
    // Find which instance this address corresponds to...
    counts[instanceIndex]++;
}
// Distribution should be roughly uniform across all instances
```

---

## Summary

| Aspect | Before | After |
|--------|--------|-------|
| Selection source | `System.Random` | RandomROM walk |
| Platform variation | Different per platform | Bit-identical everywhere |
| Seed | ROM hash + TickCount | RandomROM bytes |
| Third-party dependencies | .NET RNG | None |
| Security | Same (`I(M;A)=0`) | Same |
| Encoder output | Varies by platform | **Deterministic** |

---

## See Also

- `randomness.md` — Theory of captured entropy and why balance is not the goal
- `zoscii-unsignal-zeromutualinformation.md` — §6a.5 describes RandomROM in the implementation properties section
- `unsignal-protocol.md` — UNSIGNAL protocol specification

---

*(c) 2026 Cyborg Unicorn Pty Ltd - MIT License*