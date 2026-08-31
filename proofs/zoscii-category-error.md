# ZOSCII — Selected Posts & Demos

*Julian Cassin — Cyborg Unicorn Founder, Director, ZOSCII Foundation*

> **Note:** "ROM" and "Private Key" are used interchangeably throughout. In many implementations the private key is physically a ROM, or a ROM copied into RAM — it's still treated as a ROM.

## Encryption Is a Category Error

The biggest trap in cybersecurity is assuming that every method of securing data must be "encryption." It's a massive category error, and the "experts" make it every single day.

By strict academic definition, encryption takes plaintext and runs it through a mathematical function (like AES or RSA) to physically transform and mangle those bits into a brand-new object called ciphertext.

But look at how a pure Address Indirection Engine like ZOSCII operates. A single, invariant address stream primitive — for example, the sequence `1 2 3 3 4` — satisfies four completely different states simultaneously without changing a single bit of its own layout:

- **Plain Output Mode (the origin):** At its most basic, the address stream is just pointers to values that get read out directly — text, glyphs, whatever the bytes are. This is where ZOSCII comes from: it's an address table, nothing more. For example, `#0048,#0065,#006c,#006c,#006f` is five 16-bit addresses, each pointing to one byte — and it decodes to `hello`. No security, no concealment, just indirection reading out what it points at. Everything below is this same primitive used differently.

- **Data Concealment Mode:** The address stream acts as a coordinate index map pointing to where those exact bytes already naturally reside inside a private binary file (the ROM / Private Key).

- **Mask Mode (Steganography):** You don't generate or transmit a custom file at all. A live, 100% unmodified public file (like the ASD's REDSPICE.PDF hosted on their official site) IS the address stream. The network transmission is completely pure, trusted, and untouched. A local, pre-shared Manufactured Mask ROM acts as a spatial stencil, instantly giving meaning to those public bytes upon arrival.

- **Blockchain Mode:** The address stream acts as a self-referential validation path that indexes backwards into the fully transparent, open-source history of the ledger itself.

Now, ask a traditional cryptographer: How can the exact same data stream layout simultaneously read out plain text, be a hidden file index, a clean public PDF download, and a transparent distributed ledger if it's "encryption"?

It can't. A mathematical block cipher cannot shift between an unmodified public document and an open blockchain validation layer without entirely altering its code.

ZOSCII achieves this because it isn't encrypting data; it is re-indexing spatial geometry. The emitted addresses aren't ciphertext tokens — they are a trajectory through a data landscape. Every file without exception can parse any other file. If you use the wrong ROM, it happily spits out structured, plausible binary noise with zero error codes.

To an outside observer, brute-forcing the stream is a logical paradox. Because a single vector maps to all possible meanings depending entirely on the local operational state, testing a candidate ROM will simultaneously yield every 5-letter word in existence. You have zero mathematical feedback to tell you which guess is correct.

The ultimate logical tautology cannot be bypassed:

> **X = Unknowable**

If you don't possess the local spatial map, you don't know the coordinates. And if you don't know something, you don't know something. The mutual information is a flat, zero.

Stop evaluating a universal, multi-mode data transformer using the outdated rules of a defensive mathematical codebook. The payload is inert. The transport is irrelevant.

## What Is a Mask ROM in the Context of ZOSCII?

Based on old 8bit Software Sprites, in order to put a sprite on a background and move it, you need to restore the background — this is usually done with XOR which is reversable but XOR almost never gives any good results... I see a pattern here.

Or... you can create sprite masks — they can either be masks of what to keep of the background, or what to draw — they are the most common two masks, but there are other variants when you get into spirate compositions etc.

In the world of ZOSCII, it serves a similar purpose, it either indicates what shouldn't be encoded/decoded, or what should.

The results are that you can hide information inside an address stream — in fact you can have multiple concurrent streams in the same address file by using different masks.

This concept is not entirely new from a security point of view either — Ethan Winer had created The Wire Noise Theory in which the medium is totally different but the concept is very similar — where there are differences, is that wire noise is very sequential where-as masks are fully random access.

Next time you find a file you cannot recognise, consider... how many secret messages might actually be in that file? Maybe... that file was not even conceived to be a ZOSCII file, but it could be the latest list of ASD CyberSecurity Recommendations in PDF form for which you created masks to hide secrets within. Yet we might have used their very own file AS the ZOSCII encoding masked!

The demo below shows exactly this — secret messages retrieved from ASD's OWN assets. The following video game names are pulled from an ASD asset via Mask ROMs: "Operation Wolf" "Rush'n Attack" "Green Beret" "Silent Service" "Soviet Strike" "Guerrilla War" and my favourite "Ikari Warriors".

## Demo: zcreatemask.c and zunmask.c

Here is a demonstration of `zcreatemask.c` and `zunmask.c` now available from within the ZOSCII GitHub repo.

**Notes:** Both ASD's downloadable PDF is low entropy as is the mask — but... given the mask is the secret, nobody can ever know what the messages transmitted using the ASD's address file relate to. Note: `zunmask.c` is very similar to the standard `zdecode.c` but honoring the chosen mask value and skipping duplicates. This isn't the only way to create a mask — just one simple demonstrable way. The mask is for decoding only, not encoding.

The Red Spice PDF is available from ASD's website.

```
D:\dev\zosciiutils>zcreatemask.exe ASD-REDSPICE-Blueprint.pdf "Operation Wolf Rush'n Attack Green Beret Silent Service Soviet Strike Guerrilla War and my favourite Ikari Warriors"
ZOSCII Mask ROM Generator v20260628
(c) 2026 Cyborg Unicorn Pty Ltd - MIT License
=============================================

Asset file: ASD-REDSPICE-Blueprint.pdf (3284104 bytes, 1642052 address pairs)
Message:   "Operation Wolf Rush'n Attack Green Beret Silent Service Soviet Strike Guerrilla War and my favourite Ikari Warriors" (115 bytes)
Output ROM: ASD-REDSPICE-Blueprint.pdf.mask

Mask ROM Created
================
Addresses mapped: 115 / 115
Unmapped bytes:  0
Non-zero bytes:  115 (0.1755% of 64KB)

To decode:
 zunmask "ASD-REDSPICE-Blueprint.pdf.mask" "ASD-REDSPICE-Blueprint.pdf" decoded.txt

D:\dev\zosciiutils>zunmask ASD-REDSPICE-Blueprint.pdf.mask ASD-REDSPICE-Blueprint.pdf decoded.txt
ZOSCII Mask Decoder v20260628
(c) 2026 Cyborg Unicorn Pty Ltd - MIT License
=============================================

Mask ROM:  ASD-REDSPICE-Blueprint.pdf.mask
Asset file: ASD-REDSPICE-Blueprint.pdf
Output:   decoded.txt

Pairs processed: 1642052
Bytes written:  115
Zero slots skipped: 1641937

SUCCESS: Message decoded to "decoded.txt"

D:\dev\zosciiutils>type decoded.txt
Operation Wolf Rush'n Attack Green Beret Silent Service Soviet Strike Guerrilla War and my favourite Ikari Warriors
```

For dummies and dumb AIs that think it isn't steganography, google 'Coverless Steganography'