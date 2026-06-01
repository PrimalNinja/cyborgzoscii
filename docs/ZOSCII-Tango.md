# ZOSCII Tango: Multi-ROM Encoding at 2x Expansion

**Author:** Julian Cassin  
**Date:** 2026-06-01

## MIT LICENSE

ZOSCII Tango is released under the MIT License.

---

## ZOSCII Tango = Multiple ROMs, Same Size

Standard ZOSCII chain-encoding with N ROMs produces 2^N expansion — each ROM doubles the output. Three ROMs means 8x. This is fine for small payloads but impractical for large ones.

Tango solves this. Three ROMs. Still 2x. All three must be present to decode.

---

## How It Works

Instead of encoding the entire payload through each ROM in sequence, Tango round-robins at the byte level.

Byte 0 is encoded through ROM[0]. Byte 1 through ROM[1]. Byte 2 through ROM[2]. Byte 3 back to ROM[0]. And so on.

Each byte produces exactly one 2-byte ROM address — identical expansion to single-ROM encoding. The output is always exactly `input.Length * 2` bytes regardless of how many ROMs are used.

With 1 ROM, Tango is identical to standard ZOSCII encoding. With 2 or 3 ROMs, every byte position uses a different key. An attacker with only ROM[0] can recover every third byte at best — and cannot determine which bytes those are without all ROMs present.

---

## Properties

| Property | Value |
|---|---|
| Output size | input × 2 bytes (same as single-ROM ZOSCII) |
| ROMs required | 1–3 |
| Expansion with 3 ROMs | 2x (vs 8x for standard chain) |
| All ROMs required to decode | Yes |
| Compromise of 1 ROM exposes | At most every Nth byte (N = ROM count) |
| Tango mode detectable | No — output is indistinguishable from single-ROM ZOSCII |
| Algorithm | Round-robin byte-level ROM address lookup |

---

## Usage

```csharp
// Tango encode — round-robins across all ROMs per byte
byte[] arrEncoded = ZEncode.Chain(arrPlain, new[] { rom1, rom2, rom3 }, true);

// Tango decode — must use same ROMs in same order
byte[] arrDecoded = ZDecode.Chain(arrEncoded, new[] { rom1, rom2, rom3 }, true);
```

The `zencode` and `zdecode` command-line tools support Tango via the `-t` flag:

```
zencode.exe rom1 rom2 rom3 input.bin output.zoc -t
zdecode.exe rom1 rom2 rom3 output.zoc recovered.bin -t
```

---

## When to Use Tango

- Large payloads where 8x chain expansion is unacceptable
- Up to 3 people each hold one ROM — all must cooperate to decode
- Information-theoretic security (I(M;A)=0) is maintained per byte position

## When to Use Standard Chain

- Maximum diffusion is required — each ROM encodes the entire output of the previous, making partial ROM compromise yield nothing at all
- Small payloads where expansion is not a concern
- Compatibility with systems that do not support Tango

---

## Security Note

Tango is weaker than chaining in one respect: an attacker who compromises ROM[0] (via a system exploit, torture or coercion) of a 3-ROM Tango encoding can recover every third byte of plaintext. Standard chaining gives an attacker with ROM[0] nothing — the output of ROM[1] and ROM[2] is still fully opaque.

However, Tango remains information-theoretically secure. I(M;A)=0 holds at every byte position — without the correct ROM for that position, the encoded address reveals nothing about the plaintext byte. All ROMs must still be present to fully decode.

Critically, the attacker cannot know whether ROM[0] is 1 of 1, 1 of 2, or 1 of 3 — or whether the file was encoded multiple times, or with a custom tool using more ROMs. The encoded output is indistinguishable from single-ROM ZOSCII. Even with ROM[0] in hand, the attacker cannot determine which byte positions it covers, how many other ROMs exist, or whether any partial recovery is meaningful. This significantly deflates the practical value of compromising any single ROM.

---

## The Philosophy

ZOSCII chain-encoding is powerful but expensive. ZOSCII Tango is the pragmatic alternative — the same number of keys, the same requirement for all to be present, at the cost of per-byte rather than per-file diffusion.

Simple. Verifiable. MIT Licensed.

---

(c) 2026 Cyborg Unicorn Pty Ltd — https://cyborgunicorn.com.au