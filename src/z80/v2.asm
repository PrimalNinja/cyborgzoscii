; ============================================================
; ZOSCII Speed Test - NTC Version
; SP is instruction pointer
; Program is sequence of dw keyword addresses
; Each keyword ends with ret
; org at #2000
; (c) 2025 Cyborg Unicorn Pty Ltd - MIT License
; ============================================================

            org #2000

; ============================================================
; Test selection (change which one is uncommented)
; ============================================================

TEST_ENCODE_ONLY equ 1
TEST_DECODE_ONLY equ 0
TEST_BOTH        equ 0

; ============================================================
; Constants
; ============================================================

ROM_SIZE       equ 8192
MSG_SIZE       equ 64
ENC_SIZE       equ 128

; ============================================================
; Startup
; ============================================================

startup:
            ld (#BE80), sp
            di
            ld sp, main_thread
            ret

; ============================================================
; Main Thread (NTC - sequence of keyword addresses)
; ============================================================

main_thread:
            dw kw_prepare_rom
IF TEST_ENCODE_ONLY = 1
            dw kw_test_encode_loop
ENDIF
IF TEST_DECODE_ONLY = 1
            dw kw_test_decode_loop
ENDIF
IF TEST_BOTH = 1
            dw kw_test_encode_loop
            dw kw_test_decode_loop
ENDIF
            dw kw_stop

; ============================================================
; Test 1000 encodes (NTC thread)
; ============================================================

kw_test_encode_loop:
            dw kw_let16, encode_count, 0
encode_loop_start:
            dw kw_cplt16, encode_count, 1000, tmp0
            dw kw_goto_iffalse, tmp0, encode_loop_done
            dw kw_encode
            dw kw_inc16, encode_count
            dw kw_goto, encode_loop_start
encode_loop_done:
            dw kw_return

; ============================================================
; Test 1000 decodes (NTC thread)
; ============================================================

kw_test_decode_loop:
            dw kw_let16, decode_count, 0
decode_loop_start:
            dw kw_cplt16, decode_count, 1000, tmp0
            dw kw_goto_iffalse, tmp0, decode_loop_done
            dw kw_decode
            dw kw_inc16, decode_count
            dw kw_goto, decode_loop_start
decode_loop_done:
            dw kw_return

; ============================================================
; ZOSCII Encode (NTC thread - not a primitive)
; ============================================================

kw_encode:
            dw kw_ld_ptr, enc_src, test_msg_ptr
            dw kw_let16, enc_len, MSG_SIZE
            dw kw_ld_ptr, enc_dst, enc_buffer
            dw kw_let16, enc_idx, 0
enc_loop:
            dw kw_cplt16, enc_idx, enc_len, tmp0
            ; FIX #1: was kw_goto_iftrue (inverted - would exit when idx < len).
            ; Correct: exit loop when NOT (idx < len), i.e. when tmp0 is false.
            dw kw_goto_iffalse, tmp0, enc_done
            dw kw_add16, enc_src, enc_idx, tmp1
            dw kw_peek, tmp1, enc_char
            dw kw_mul16, enc_char, 2, tmp2
            dw kw_add16, offset_table_ptr, tmp2, tmp3
            dw kw_peek, tmp3, enc_off_lo
            dw kw_add16, tmp3, 1, tmp3_plus1
            dw kw_peek, tmp3_plus1, enc_off_hi
            dw kw_add16, tmp3, 2, tmp4
            dw kw_peek, tmp4, enc_cnt_lo
            dw kw_add16, tmp4, 1, tmp4_plus1
            dw kw_peek, tmp4_plus1, enc_cnt_hi
            dw kw_or16, enc_cnt_lo, enc_cnt_hi, tmp5
            ; FIX #2: was kw_goto_iffalse (would jump to enc_ok when count==0, i.e. no occurrences).
            ; Correct: jump to enc_ok when count != 0 (there are occurrences to pick from).
            dw kw_goto_iftrue, tmp5, enc_ok
            dw kw_return
enc_ok:
            dw kw_random, enc_cnt_lo, enc_pos
            dw kw_add16, enc_off_lo, enc_pos, enc_index
            dw kw_mul16, enc_index, 2, tmp5
            dw kw_add16, addr_list_ptr, tmp5, tmp6
            dw kw_peek, tmp6, enc_adr_lo
            dw kw_add16, tmp6, 1, tmp6_plus1
            dw kw_peek, tmp6_plus1, enc_adr_hi
            dw kw_mul16, enc_idx, 2, tmp7
            dw kw_add16, enc_dst, tmp7, tmp8
            dw kw_poke, tmp8, enc_adr_lo
            dw kw_add16, tmp8, 1, tmp8_plus1
            dw kw_poke, tmp8_plus1, enc_adr_hi
            dw kw_inc16, enc_idx
            dw kw_goto, enc_loop
enc_done:
            dw kw_return

; ============================================================
; ZOSCII Decode (NTC thread - not a primitive)
; ============================================================

kw_decode:
            dw kw_ld_ptr, dec_rom, rom_data
            dw kw_ld_ptr, dec_src, enc_buffer
            dw kw_ld_ptr, dec_dst, dec_buffer
            dw kw_let16, dec_len, MSG_SIZE
            dw kw_let16, dec_idx, 0
dec_loop:
            dw kw_cplt16, dec_idx, dec_len, tmp0
            ; FIX #3: was kw_goto_iftrue (inverted - would exit when idx < len).
            ; Correct: exit loop when NOT (idx < len).
            dw kw_goto_iffalse, tmp0, dec_done
            dw kw_mul16, dec_idx, 2, tmp1
            dw kw_add16, dec_src, tmp1, tmp2
            dw kw_peek, tmp2, dec_adr_lo
            dw kw_add16, tmp2, 1, tmp2_plus1
            dw kw_peek, tmp2_plus1, dec_adr_hi
            dw kw_add16, dec_rom, dec_adr_lo, tmp3
            dw kw_peek, tmp3, dec_byte
            dw kw_add16, dec_dst, dec_idx, tmp4
            dw kw_poke, tmp4, dec_byte
            dw kw_inc16, dec_idx
            dw kw_goto, dec_loop
dec_done:
            dw kw_return

; ============================================================
; NTC Primitives (keywords that end with ret)
; ============================================================

kw_ret:
            ret

kw_return:
            ret

kw_goto:
            pop hl
            ld sp, hl
            ret

kw_goto_iftrue:
            pop hl
            pop de
            ld a, (de)
            inc de
            or (de)
            jr nz, goto_true
            ret
goto_true:
            ld sp, hl
            ret

kw_goto_iffalse:
            pop hl
            pop de
            ld a, (de)
            inc de
            or (de)
            jr z, goto_false
            ret
goto_false:
            ld sp, hl
            ret

; FIX #4: kw_let16 had a spurious "ex de, hl" which swapped the variable address
; and the value, causing the value to be written to the wrong (value-as-address) location.
; Correct: pop destination address into HL, pop immediate value into DE, write DE to (HL).
kw_let16:
            pop hl          ; HL = address of destination variable
            pop de          ; DE = immediate 16-bit value
            ld (hl), e      ; store low byte of value
            inc hl
            ld (hl), d      ; store high byte of value
            ret

kw_inc16:
            pop hl
            ld a, (hl)
            inc a
            ld (hl), a
            jr nz, inc_done
            inc hl
            ld a, (hl)
            inc a
            ld (hl), a
inc_done:
            ret

; FIX #5: kw_add16 had multiple bugs:
;   (a) First param address was popped into HL, then immediately overwritten when
;       the second param's value was loaded into H:L - the first param address was lost.
;   (b) The third pop (result address) was being used as a second source operand,
;       then a spurious fourth pop corrupted the NTC thread.
; Correct: pop first param addr into DE, load its 16-bit value into HL;
;          pop second param addr into DE, load its 16-bit value into DE;
;          add; pop result addr into DE, store HL.
kw_add16:
            pop de          ; DE = address of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a         ; HL = value of var1 (little-endian: lo in L, hi in H)
            pop de          ; DE = address of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a         ; DE = value of var2 (lo in E, hi in D)
            add hl, de      ; HL = var1 + var2
            pop de          ; DE = address of result variable
            ld (de), l      ; store low byte
            inc de
            ld (de), h      ; store high byte
            ret

; FIX #6: kw_mul16 had the same first-param-address-into-HL bug as kw_add16,
; and the same extra spurious pop that would corrupt the NTC thread.
; Correct: use DE for first param address (same pattern as kw_add16), then call mul16_z80.
; mul16_z80 expects multiplicand in HL, multiplier in DE, returns result in HL.
kw_mul16:
            pop de          ; DE = address of var1 (multiplicand)
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a         ; HL = value of var1
            pop de          ; DE = address of var2 (multiplier)
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a         ; DE = value of var2
            call mul16_z80  ; HL = HL * DE (low 16 bits of product)
            pop de          ; DE = address of result variable
            ld (de), l      ; store low byte
            inc de
            ld (de), h      ; store high byte
            ret

mul16_z80:
            ld bc, 0
            ld a, 16
mul_loop:
            add hl, hl
            rl c
            rl b
            jr nc, mul_skip
            add hl, de
            jr nc, mul_skip
            inc bc
mul_skip:
            dec a
            jr nz, mul_loop
            ret

; FIX #7: kw_cplt16 (compare less-than 16-bit) had the same first-param-address-into-HL
; bug as kw_add16: param1 address was put into HL, then param2's value was loaded
; into H:L, destroying the param1 address before it could be dereferenced.
; Also: result write used an extra spurious pop, corrupting the thread.
; Correct: load param1 value into HL via DE, load param2 value into DE, subtract,
;          then pop result address into DE and store 0 or 1.
kw_cplt16:
            pop de          ; DE = address of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a         ; HL = value of var1
            pop de          ; DE = address of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a         ; DE = value of var2
            or a
            sbc hl, de      ; HL = var1 - var2; carry set if var1 < var2
            pop de          ; DE = address of result variable
            jr c, lt_true
            xor a
            ld (de), a      ; result = 0 (false)
            inc de
            ld (de), a
            ret
lt_true:
            ld a, 1
            ld (de), a      ; result = 1 (true)
            inc de
            xor a
            ld (de), a
            ret

; FIX #8: kw_peek had two bugs:
;   (a) Only the low byte of the source pointer variable was read (missing inc hl + second byte),
;       so the high byte of the address was always 0.
;   (b) After the second pop hl (destination address), "ex de, hl" swapped HL with an
;       uninitialised DE, causing the store to go to a garbage address.
; Correct: read both bytes of the source address into DE, dereference DE to get the byte,
;          then pop destination address into HL and store with zero-extension.
kw_peek:
            pop hl          ; HL = address of source pointer variable
            ld e, (hl)      ; E = low byte of source address
            inc hl
            ld d, (hl)      ; D = high byte of source address; DE = full source address
            ld a, (de)      ; A = byte at source address
            pop hl          ; HL = address of destination variable
            ld (hl), a      ; store byte (low byte of 16-bit variable)
            inc hl
            xor a
            ld (hl), a      ; zero-extend high byte
            ret

; FIX #9: kw_poke had two bugs:
;   (a) "ld a, e" read the low byte of the SOURCE VARIABLE'S ADDRESS (the label value
;       popped into DE), not the value stored at that address.
;   (b) "ld (hl), a" wrote to the address of the destination pointer variable itself
;       (the label popped into HL), not to the address STORED in that variable.
; Correct: dereference HL to get the destination address into DE, then dereference
;          the source variable address (second pop into HL) to get the byte value.
kw_poke:
            pop hl          ; HL = address of destination pointer variable
            ld e, (hl)      ; E = low byte of destination address
            inc hl
            ld d, (hl)      ; D = high byte; DE = full destination address
            pop hl          ; HL = address of source value variable
            ld a, (hl)      ; A = low byte of source value (the byte to write)
            ld (de), a      ; write byte to destination address
            ret

kw_ld_ptr:
            pop hl          ; HL = address of destination variable
            pop de          ; DE = address of source pointer variable
            ld a, (de)      ; A = low byte of source value
            ld (hl), a      ; store low byte to destination
            inc hl
            inc de
            ld a, (de)      ; A = high byte of source value
            ld (hl), a      ; store high byte to destination
            ret

; FIX #10: kw_or16 had the same first-param-address-into-HL bug as kw_add16.
; Correct: use DE for first param address (same pattern as kw_add16).
kw_or16:
            pop de          ; DE = address of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a         ; HL = value of var1
            pop de          ; DE = address of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a         ; DE = value of var2
            ld a, l
            or e
            ld l, a
            ld a, h
            or d
            ld h, a         ; HL = var1 OR var2
            pop de          ; DE = address of result variable
            ld (de), l      ; store low byte
            inc de
            ld (de), h      ; store high byte
            ret

; FIX #11: kw_random had a calling-convention mismatch with random_range.
; kw_random called random_range via "call", which pushes a return address onto the
; stack. But random_range began with "pop hl" to fetch its parameter - this popped
; the return address, not the range value. The entire call chain was broken.
; Additionally the result of divmod was discarded before being returned.
;
; New design: kw_random is self-contained. It pops both param addresses from
; the NTC thread, reads the range value, calls a fixed helper that takes range
; in HL and returns remainder in HL, then writes the result to the output variable.
kw_random:
            pop hl          ; HL = address of range variable
            ld e, (hl)
            inc hl
            ld d, (hl)      ; DE = range value
            pop hl          ; HL = address of result variable
            push hl         ; save result address
            ex de, hl       ; HL = range, DE = result addr (temporarily)
            push de         ; save result address again (ex clobbered registers)
            ; HL = range value; call random_range_hl which returns remainder in HL
            call random_range_hl
            pop de          ; DE = result address
            ld (de), l      ; store low byte of remainder
            inc de
            ld (de), h      ; store high byte of remainder
            pop hl          ; clean extra push (balance stack - this was the first push hl)
            ret

; ============================================================
; Random number generator
; ============================================================

rng_state:      dw #1234

random_next:
            ld hl, (rng_state)
            ld a, h
            xor l
            ld h, a
            add hl, hl
            ld a, h
            xor l
            ld h, a
            ld (rng_state), hl
            ex de, hl
            ret

; FIX #12: random_range_hl replaces the old random_range (which was broken due to
; the calling convention mismatch described in FIX #11).
; Input:  HL = range (must be > 0)
; Output: HL = random number in [0, range-1]
; Clobbers: A, BC, DE
random_range_hl:
            ld a, h
            or l
            jr z, rand_hl_zero  ; range == 0: return 0
            push hl             ; save range
            call random_next    ; DE = 16-bit random number
            pop hl              ; HL = range
            ; now compute DE mod HL; result returned in HL by divmod_16
            call divmod_16      ; HL = DE mod HL
            ret
rand_hl_zero:
            ld hl, 0
            ret

; FIX #13: divmod_16 was completely broken:
;   (a) The divisor (passed in HL) was immediately overwritten ("ld hl, 0") inside
;       the loop body with no prior save, so the comparison was always against 0.
;   (b) "ld de, hl" on line 427 is not a valid Z80 instruction.
;   (c) The algorithm logic was incorrect regardless of the above.
;
; New correct implementation using restoring non-restoring binary long division.
; Input:  HL = divisor, DE = dividend
; Output: HL = remainder (DE mod HL)
; Clobbers: A, BC, DE
divmod_16:
            ld b, h
            ld c, l         ; BC = divisor (preserved throughout)
            ld hl, 0        ; HL = partial remainder, starts at 0
            xor a           ; clear carry for first rotation
            ld a, 16        ; process 16 bits
div_loop:
            ; Shift DE left by 1; MSB of D goes to carry.
            ; Then shift carry into HL's LSB (remainder grows from dividend's MSBs).
            rl e
            rl d            ; carry = old MSB of DE (processed MSB-first)
            rl l
            rl h            ; remainder <<= 1, LSB gets the dividend bit
            ; If remainder >= divisor, subtract divisor (non-restoring step).
            push hl         ; save remainder before subtraction attempt
            or a
            sbc hl, bc      ; HL = remainder - divisor
            jr c, div_nosub ; borrow: remainder < divisor, don't subtract
            ; remainder >= divisor: keep the subtracted value (non-restoring)
            pop de          ; discard old remainder (DE is scratch here)
            jr div_next
div_nosub:
            pop hl          ; restore remainder (subtraction overshot)
div_next:
            dec a
            jr nz, div_loop
            ; HL = final remainder
            ret

; ============================================================
; ROM Preparation (NTC thread)
; ============================================================

kw_prepare_rom:
            dw kw_let16, count_ptr, count_array
            dw kw_let16, byte_counter, 0
prep_init_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_init_done
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, count_ptr, tmp1, tmp2
            dw kw_poke, tmp2, 0
            dw kw_add16, tmp2, 1, tmp2_plus1
            dw kw_poke, tmp2_plus1, 0
            dw kw_inc16, byte_counter
            dw kw_goto, prep_init_loop
prep_init_done:
            dw kw_let16, rom_ptr, rom_data
            dw kw_let16, rom_index, 0
prep_count_loop:
            dw kw_cplt16, rom_index, ROM_SIZE, tmp0
            dw kw_goto_iffalse, tmp0, prep_count_done
            dw kw_add16, rom_ptr, rom_index, tmp1
            dw kw_peek, tmp1, byte_val
            dw kw_mul16, byte_val, 4, tmp2
            dw kw_add16, count_ptr, tmp2, tmp3
            dw kw_peek, tmp3, cnt_lo
            dw kw_add16, cnt_lo, 1, cnt_new
            dw kw_poke, tmp3, cnt_new
            dw kw_inc16, rom_index
            dw kw_goto, prep_count_loop
prep_count_done:
            dw kw_let16, offset_ptr, offset_table
            dw kw_let16, running_offset, 0
            dw kw_let16, byte_counter, 0
prep_off_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_off_done
            dw kw_peek, running_offset, off_lo
            dw kw_add16, running_offset, 1, running_offset_plus1
            dw kw_peek, running_offset_plus1, off_hi
            dw kw_poke, offset_ptr, off_lo
            dw kw_add16, offset_ptr, 1, offset_ptr_plus1
            dw kw_poke, offset_ptr_plus1, off_hi
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, count_ptr, tmp1, tmp2
            dw kw_peek, tmp2, cnt_lo
            dw kw_add16, tmp2, 1, tmp2_plus1
            dw kw_peek, tmp2_plus1, cnt_hi
            dw kw_add16, offset_ptr, 2, offset_ptr_plus2
            dw kw_poke, offset_ptr_plus2, cnt_lo
            dw kw_add16, offset_ptr, 3, offset_ptr_plus3
            dw kw_poke, offset_ptr_plus3, cnt_hi
            dw kw_add16, running_offset, cnt_lo, running_offset
            dw kw_add16, offset_ptr, 4, offset_ptr
            dw kw_inc16, byte_counter
            dw kw_goto, prep_off_loop
prep_off_done:
            dw kw_let16, fill_ptr, fill_table
            dw kw_let16, byte_counter, 0
prep_fill_init_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_fill_init_done
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, offset_table, tmp1, tmp2
            dw kw_peek, tmp2, off_lo
            dw kw_add16, tmp2, 1, tmp2_plus1
            dw kw_peek, tmp2_plus1, off_hi
            dw kw_mul16, byte_counter, 2, tmp3
            dw kw_add16, fill_ptr, tmp3, tmp4
            dw kw_poke, tmp4, off_lo
            dw kw_add16, tmp4, 1, tmp4_plus1
            dw kw_poke, tmp4_plus1, off_hi
            dw kw_inc16, byte_counter
            dw kw_goto, prep_fill_init_loop
prep_fill_init_done:
            dw kw_let16, rom_ptr, rom_data
            dw kw_let16, rom_index, 0
prep_fill_loop:
            dw kw_cplt16, rom_index, ROM_SIZE, tmp0
            dw kw_goto_iffalse, tmp0, prep_fill_done
            dw kw_add16, rom_ptr, rom_index, tmp1
            dw kw_peek, tmp1, byte_val
            dw kw_mul16, byte_val, 2, tmp2
            dw kw_add16, fill_ptr, tmp2, tmp3
            dw kw_peek, tmp3, fill_lo
            dw kw_add16, tmp3, 1, tmp3_plus1
            dw kw_peek, tmp3_plus1, fill_hi
            dw kw_add16, addr_list, fill_lo, tmp4
            dw kw_poke, tmp4, rom_index_lo
            dw kw_add16, tmp4, 1, tmp4_plus1
            dw kw_poke, tmp4_plus1, rom_index_hi
            dw kw_add16, fill_lo, 2, fill_new
            dw kw_poke, tmp3, fill_new_lo
            dw kw_add16, tmp3, 1, tmp3_plus1
            dw kw_poke, tmp3_plus1, fill_new_hi
            dw kw_inc16, rom_index
            dw kw_goto, prep_fill_loop
prep_fill_done:
            dw kw_return

; ============================================================
; kw_stop
; ============================================================

kw_stop:
            ld sp, (#BE80)
            ei
            ret

; ============================================================
; Variables
; ============================================================

test_counter:   dw 0
encode_count:   dw 0
decode_count:   dw 0
error_flag:     dw 0

tmp0:           dw 0
tmp1:           dw 0
tmp2:           dw 0
tmp2_plus1:     dw 0
tmp3:           dw 0
tmp3_plus1:     dw 0
tmp4:           dw 0
tmp4_plus1:     dw 0
tmp5:           dw 0
tmp6:           dw 0
tmp6_plus1:     dw 0
tmp7:           dw 0
tmp8:           dw 0
tmp8_plus1:     dw 0

byte_counter:   dw 0
byte_val:       dw 0
cnt_lo:         dw 0
cnt_hi:         dw 0
cnt_new:        dw 0
off_lo:         dw 0
off_hi:         dw 0
fill_lo:        dw 0
fill_hi:        dw 0
fill_new:       dw 0
fill_new_lo:    dw 0
fill_new_hi:    dw 0
running_offset: dw 0
running_offset_plus1: dw 0
offset_ptr:     dw 0
offset_ptr_plus1: dw 0
offset_ptr_plus2: dw 0
offset_ptr_plus3: dw 0
count_ptr:      dw 0
fill_ptr:       dw 0
rom_ptr:        dw 0
rom_index:      dw 0
rom_index_lo:   dw 0
rom_index_hi:   dw 0

enc_src:        dw 0
enc_len:        dw 0
enc_dst:        dw 0
enc_idx:        dw 0
enc_char:       dw 0
enc_off_lo:     dw 0
enc_off_hi:     dw 0
enc_cnt_lo:     dw 0
enc_cnt_hi:     dw 0
enc_pos:        dw 0
enc_index:      dw 0
enc_adr_lo:     dw 0
enc_adr_hi:     dw 0

dec_rom:        dw 0
dec_src:        dw 0
dec_dst:        dw 0
dec_len:        dw 0
dec_idx:        dw 0
dec_adr_lo:     dw 0
dec_adr_hi:     dw 0
dec_byte:       dw 0

; ============================================================
; Data buffers
; ============================================================

rom_data:       ds ROM_SIZE
count_array:    ds 1024
offset_table:   ds 1024
fill_table:     ds 512
addr_list:      ds 16384
offset_table_ptr: dw offset_table
addr_list_ptr:    dw addr_list

test_msg:       db "This is a test message for ZOSCII encode decode speed testing."
                ds MSG_SIZE-64, 0
test_msg_ptr:   dw test_msg

enc_buffer:     ds ENC_SIZE
dec_buffer:     ds MSG_SIZE