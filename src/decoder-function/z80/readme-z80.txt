;Cyborg ZOSCII v20250805
;(c) 2025 Cyborg Unicorn Pty Ltd.
;This software is released under MIT License.


Version ASCII: Traditional Pointer Method, Per character: ~104 cycles + char_out time

start:	ld hl, string
		call strout
		ret

strout:	ld a, (hl)
		or a
		ret z
		call char_out
		inc hl
		jr strout
		
		; this is Hello, World!
string: dw "Hello, World!", 0

note: char_out usually has something like this within that is not required for ZOSCII

		push af
		ld hl, ASCIITABLE
		ld b, 0
		ld c, a
		add hl, bc
		pop af




Version 1: Traditional Pointer Method, Per character: 75 cycles + char_out time

start:	ld hl, string
		call strout
		ret

strout:	ld e, (hl)
		inc hl
		ld d, (hl)
		inc hl
		ld a, d
		or e
		ret z
		ld a, (de)
		call char_out
		jr strout
		
		; this might be Hello, World!
string: dw #C245, #C891, #C456, #C023, #C789, #C334, #C667, #C123, #C998, #C445, #C876, #C012, #0000



Version 2: Stack Manipulation, Per character: 64 cycles + char_out time

start:	ld de, string
		call strout
		ret

strout:	ld hl,0			; work out the stack
		add hl, sp
		ex de, hl		; hl = string, de = stack

		ld sp, hl
		
loop:	pop hl
		ld a, h
		or l
		jr z, loope
		ld a, (hl)
		call char_out
		jr loop

loope:	ex de, hl		; de = string, hl = stack
		ld sp, hl
		ret

		; this might be Hello, World!
string: dw #C245, #C891, #C456, #C023, #C789, #C334, #C667, #C123, #C998, #C445, #C876, #C012, #0000



Version 3: Inline Stack Method, Per character: 61 cycles + char_out time

start:	call stroutsp
		dw #C245, #C891, #C456, #C023, #C789, #C334, #C667, #C123, #C998, #C445, #C876, #C012, #0000
		ret

stroutsp:	
loop:	pop hl		; hl = start of string
		ld a, h
		or l
		jr z, loope
		ld a, (hl)
		call char_out
		jr loop
		
loope:	ret		


Performance Summary:

- **Version 1**: 81 cycles per character
- **Version 2**: 64 cycles per character (**21% faster**)
- **Version 3**: 61 cycles per character (**24.7% faster**)
