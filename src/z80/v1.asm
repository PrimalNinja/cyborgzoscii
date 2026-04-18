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
            dw kw_goto_iftrue, tmp0, enc_done
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
            dw kw_goto_iffalse, tmp5, enc_ok
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
            dw kw_goto_iftrue, tmp0, dec_done
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

kw_let16:
            pop hl
            pop de
            ex de, hl
            ld (hl), e
            inc hl
            ld (hl), d
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

kw_add16:
            pop hl
            pop de
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a
            pop de
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a
            add hl, de
            pop de
            ex de, hl
            ld (hl), e
            inc hl
            ld (hl), d
            ret

kw_mul16:
            pop hl
            pop de
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a
            pop de
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a
            call mul16_z80
            pop de
            ex de, hl
            ld (hl), e
            inc hl
            ld (hl), d
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

kw_cplt16:
            pop hl
            pop de
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a
            pop de
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a
            or a
            sbc hl, de
            pop de
            ex de, hl
            jr c, lt_true
            xor a
            ld (hl), a
            inc hl
            ld (hl), a
            ret
lt_true:
            ld a, 1
            ld (hl), a
            inc hl
            xor a
            ld (hl), a
            ret

kw_peek:
            pop hl
            ld a, (hl)
            pop hl
            ex de, hl
            ld (hl), a
            inc hl
            xor a
            ld (hl), a
            ret

kw_poke:
            pop hl
            pop de
            ld a, e
            ld (hl), a
            ret

kw_ld_ptr:
            pop hl
            pop de
            ld a, (de)
            ld (hl), a
            inc hl
            inc de
            ld a, (de)
            ld (hl), a
            ret

kw_or16:
            pop hl
            pop de
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a
            pop de
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a
            ld a, l
            or e
            ld l, a
            ld a, h
            or d
            ld h, a
            pop de
            ex de, hl
            ld (hl), e
            inc hl
            ld (hl), d
            ret

kw_random:
            pop hl
            pop de
            push de
            push hl
            call random_range
            pop hl
            ex de, hl
            pop de
            ld (de), l
            inc de
            ld (de), h
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

random_range:
            pop hl
            push hl
            ld a, h
            or l
            jr z, rand_zero
            call random_next
            pop hl
            push de
            push hl
            call divmod_16
            pop hl
            pop de
            ex de, hl
            ret
rand_zero:
            pop hl
            ld de, 0
            ret

divmod_16:
            ld bc, 0
            ld a, 16
div_loop:
            sla e
            rl d
            rl c
            rl b
            push de
            ld hl, 0
            add hl, de
            ex de, hl
            pop hl
            or a
            sbc hl, de
            jr c, div_skip
            ld de, hl
            inc bc
div_skip:
            dec a
            jr nz, div_loop
            ex de, hl
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