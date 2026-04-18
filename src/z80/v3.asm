; ============================================================
; ZOSCII Speed Test - NTC Version
; SP is instruction pointer
; Program is sequence of dw keyword addresses
; Each keyword ends with ret
; org at #2000
; (c) 2025 Cyborg Unicorn Pty Ltd - MIT License
; ============================================================
;
; NTC RULES
;   SP = instruction pointer. No call. No push.
;   Keywords pop args from thread via pop, end with ret.
;   Helpers use jp with return addr stored in memory.
;   IY = software stack pointer for gosub/return.
;
; ARGUMENT ORDER
;   Pop gets args left-to-right as written after keyword.
;   dw kw_add16, var1, var2, result
;     first pop  = var1 address
;     second pop = var2 address
;     third pop  = result address
;
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
            ld iy, software_stack_top
            ld sp, main_thread
            ret

; ============================================================
; Main Thread (NTC)
; ============================================================

main_thread:
            dw kw_gosub, sub_prepare_rom
IF TEST_ENCODE_ONLY = 1
            dw kw_gosub, sub_test_encode_loop
ENDIF
IF TEST_DECODE_ONLY = 1
            dw kw_gosub, sub_test_decode_loop
ENDIF
IF TEST_BOTH = 1
            dw kw_gosub, sub_test_encode_loop
            dw kw_gosub, sub_test_decode_loop
ENDIF
            dw kw_stop

; ============================================================
; Test 1000 encodes (NTC sub-thread)
; ============================================================

sub_test_encode_loop:
            dw kw_let16, encode_count, 0
encode_loop_start:
            dw kw_cplt16, encode_count, 1000, tmp0
            dw kw_goto_iffalse, tmp0, encode_loop_done
            dw kw_gosub, sub_encode
            dw kw_inc16, encode_count
            dw kw_goto, encode_loop_start
encode_loop_done:
            dw kw_return

; ============================================================
; Test 1000 decodes (NTC sub-thread)
; ============================================================

sub_test_decode_loop:
            dw kw_let16, decode_count, 0
decode_loop_start:
            dw kw_cplt16, decode_count, 1000, tmp0
            dw kw_goto_iffalse, tmp0, decode_loop_done
            dw kw_gosub, sub_decode
            dw kw_inc16, decode_count
            dw kw_goto, decode_loop_start
decode_loop_done:
            dw kw_return

; ============================================================
; ZOSCII Encode (NTC sub-thread)
;
; For each byte in test message
;   look up offset table entry (offset + count)
;   pick random address from the address list
;   store the 16-bit ROM address into enc_buffer
; ============================================================

sub_encode:
            dw kw_ld_ptr, enc_src, test_msg_ptr
            dw kw_let16, enc_len, MSG_SIZE
            dw kw_ld_ptr, enc_dst, enc_buffer_ptr
            dw kw_let16, enc_idx, 0
enc_loop:
            dw kw_cplt16, enc_idx, enc_len, tmp0
            dw kw_goto_iffalse, tmp0, enc_done
            ; get source byte
            dw kw_add16, enc_src, enc_idx, tmp1
            dw kw_peek, tmp1, enc_char
            ; offset table is 4 bytes per entry (offset word, count word)
            dw kw_mul16, enc_char, 4, tmp2
            dw kw_add16, offset_table_ptr, tmp2, tmp3
            ; read offset (16-bit)
            dw kw_peek16, tmp3, enc_off
            ; read count (16-bit at tmp3+2)
            dw kw_add16, tmp3, 2, tmp4
            dw kw_peek16, tmp4, enc_cnt
            ; if count is zero, bail
            dw kw_goto_iffalse, enc_cnt, enc_bail
            ; pick random index in [0, count)
            dw kw_random, enc_cnt, enc_pos
            ; calculate address list entry
            dw kw_add16, enc_off, enc_pos, enc_index
            dw kw_mul16, enc_index, 2, tmp5
            dw kw_add16, addr_list_ptr, tmp5, tmp6
            ; read the ROM address (16-bit)
            dw kw_peek16, tmp6, enc_adr
            ; store into enc_buffer at enc_idx * 2
            dw kw_mul16, enc_idx, 2, tmp7
            dw kw_add16, enc_dst, tmp7, tmp8
            dw kw_poke16, tmp8, enc_adr
            ; next byte
            dw kw_inc16, enc_idx
            dw kw_goto, enc_loop
enc_bail:
enc_done:
            dw kw_return

; ============================================================
; ZOSCII Decode (NTC sub-thread)
;
; For each 16-bit address in enc_buffer
;   look up the byte at that ROM address
;   store byte into dec_buffer
; ============================================================

sub_decode:
            dw kw_ld_ptr, dec_rom, rom_data_ptr
            dw kw_ld_ptr, dec_src, enc_buffer_ptr
            dw kw_ld_ptr, dec_dst, dec_buffer_ptr
            dw kw_let16, dec_len, MSG_SIZE
            dw kw_let16, dec_idx, 0
dec_loop:
            dw kw_cplt16, dec_idx, dec_len, tmp0
            dw kw_goto_iffalse, tmp0, dec_done
            ; read 16-bit address from enc_buffer
            dw kw_mul16, dec_idx, 2, tmp1
            dw kw_add16, dec_src, tmp1, tmp2
            dw kw_peek16, tmp2, dec_adr
            ; look up byte in ROM
            dw kw_add16, dec_rom, dec_adr, tmp3
            dw kw_peek, tmp3, dec_byte
            ; store decoded byte
            dw kw_add16, dec_dst, dec_idx, tmp4
            dw kw_poke, tmp4, dec_byte
            ; next
            dw kw_inc16, dec_idx
            dw kw_goto, dec_loop
dec_done:
            dw kw_return

; ============================================================
; ROM Preparation (NTC sub-thread)
;
; Phase 1 - zero the count array (256 entries x 4 bytes)
; Phase 2 - count byte occurrences in ROM
; Phase 3 - build offset table (running offset + count per byte)
; Phase 4 - init fill table (copy offsets from offset table)
; Phase 5 - populate addr_list with ROM indices
; ============================================================

sub_prepare_rom:
            ; --- Phase 1 - zero count array ---
            dw kw_let16, byte_counter, 0
prep_init_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_init_done
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, count_array_ptr, tmp1, tmp2
            dw kw_poke16, tmp2, 0
            dw kw_inc16, byte_counter
            dw kw_goto, prep_init_loop
prep_init_done:

            ; --- Phase 2 - count byte occurrences ---
            dw kw_let16, rom_index, 0
prep_count_loop:
            dw kw_cplt16, rom_index, ROM_SIZE, tmp0
            dw kw_goto_iffalse, tmp0, prep_count_done
            dw kw_add16, rom_data_ptr, rom_index, tmp1
            dw kw_peek, tmp1, byte_val
            dw kw_mul16, byte_val, 4, tmp2
            dw kw_add16, count_array_ptr, tmp2, tmp3
            dw kw_peek16, tmp3, cnt_val
            dw kw_add16, cnt_val, 1, cnt_val
            dw kw_poke16, tmp3, cnt_val
            dw kw_inc16, rom_index
            dw kw_goto, prep_count_loop
prep_count_done:

            ; --- Phase 3 - build offset table ---
            dw kw_let16, running_offset, 0
            dw kw_let16, byte_counter, 0
prep_off_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_off_done
            ; write running offset to offset_table[byte_counter]
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, offset_table_ptr, tmp1, tmp2
            dw kw_poke16, tmp2, running_offset
            ; get count for this byte from count_array
            dw kw_add16, count_array_ptr, tmp1, tmp3
            dw kw_peek16, tmp3, cnt_val
            ; write count to offset_table[byte_counter]+2
            dw kw_add16, tmp2, 2, tmp4
            dw kw_poke16, tmp4, cnt_val
            ; advance running offset by count
            dw kw_add16, running_offset, cnt_val, running_offset
            dw kw_inc16, byte_counter
            dw kw_goto, prep_off_loop
prep_off_done:

            ; --- Phase 4 - init fill table ---
            dw kw_let16, byte_counter, 0
prep_fill_init_loop:
            dw kw_cplt16, byte_counter, 256, tmp0
            dw kw_goto_iffalse, tmp0, prep_fill_init_done
            ; read offset from offset_table
            dw kw_mul16, byte_counter, 4, tmp1
            dw kw_add16, offset_table_ptr, tmp1, tmp2
            dw kw_peek16, tmp2, off_val
            ; write to fill_table
            dw kw_mul16, byte_counter, 2, tmp3
            dw kw_add16, fill_table_ptr, tmp3, tmp4
            dw kw_poke16, tmp4, off_val
            dw kw_inc16, byte_counter
            dw kw_goto, prep_fill_init_loop
prep_fill_init_done:

            ; --- Phase 5 - populate addr_list ---
            dw kw_let16, rom_index, 0
prep_fill_loop:
            dw kw_cplt16, rom_index, ROM_SIZE, tmp0
            dw kw_goto_iffalse, tmp0, prep_fill_done
            ; get byte value at rom[rom_index]
            dw kw_add16, rom_data_ptr, rom_index, tmp1
            dw kw_peek, tmp1, byte_val
            ; get fill position for this byte
            dw kw_mul16, byte_val, 2, tmp2
            dw kw_add16, fill_table_ptr, tmp2, tmp3
            dw kw_peek16, tmp3, fill_pos
            ; addr_list[fill_pos] = rom_index (16-bit)
            dw kw_mul16, fill_pos, 2, tmp4
            dw kw_add16, addr_list_ptr, tmp4, tmp5
            dw kw_poke16, tmp5, rom_index
            ; fill_pos++
            dw kw_add16, fill_pos, 1, fill_pos
            dw kw_poke16, tmp3, fill_pos
            dw kw_inc16, rom_index
            dw kw_goto, prep_fill_loop
prep_fill_done:
            dw kw_return

; ============================================================
; NTC Primitives
; ============================================================

; --- Flow control ---

kw_stop:
            ld sp, (#BE80)
            ei
            ret

; kw_goto - 1 arg (destination)
kw_goto:
            pop hl              ; HL = destination
            ld sp, hl
            ret

; kw_goto_iftrue - 2 args (condition_var, destination)
; jump if condition var is nonzero
kw_goto_iftrue:
            pop hl              ; HL = condition var address
            pop de              ; DE = destination
            ld a, (hl)
            inc hl
            or (hl)
            jr z, goit_skip     ; zero = false, skip jump
            ld sp, de           ; nonzero = true, jump
goit_skip:
            ret

; kw_goto_iffalse - 2 args (condition_var, destination)
; jump if condition var is zero
kw_goto_iffalse:
            pop hl              ; HL = condition var address
            pop de              ; DE = destination
            ld a, (hl)
            inc hl
            or (hl)
            jr nz, goif_skip    ; nonzero = true, skip jump
            ld sp, de           ; zero = false, jump
goif_skip:
            ret

; kw_gosub - 1 arg (sub-thread address)
; saves return point on IY software stack, enters sub-thread
kw_gosub:
            pop bc              ; BC = sub-thread address
            ld hl, 0
            add hl, sp          ; HL = return address (next in calling thread)
            ld (iy+0), l
            ld (iy-1), h
            dec iy
            dec iy
            ld sp, bc
            ret

; kw_return - 0 args
; pops return point from IY software stack
kw_return:
            inc iy
            inc iy
            ld l, (iy+0)
            ld h, (iy-1)
            ld sp, hl
            ret

; --- Variables ---

; kw_let16 - 2 args (dest_var, immediate_value)
kw_let16:
            pop hl              ; HL = dest var address
            pop de              ; DE = immediate 16-bit value
            ld (hl), e
            inc hl
            ld (hl), d
            ret

; kw_inc16 - 1 arg (var)
kw_inc16:
            pop hl              ; HL = var address
            ld a, (hl)
            inc a
            ld (hl), a
            jr nz, inc16_done
            inc hl
            inc (hl)
inc16_done:
            ret

; --- Arithmetic ---
; All 3-arg pattern (var1, var2, result)

; kw_add16 - result = var1 + var2
kw_add16:
            pop de              ; DE = addr of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a             ; HL = value of var1
            pop de              ; DE = addr of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a             ; DE = value of var2
            add hl, de          ; HL = var1 + var2
            pop de              ; DE = addr of result
            ex de, hl           ; HL = result addr, DE = sum
            ld (hl), e
            inc hl
            ld (hl), d
            ret

; kw_mul16 - result = var1 * var2
; multiply inlined, no call
kw_mul16:
            pop de              ; DE = addr of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a             ; HL = value of var1 (multiplicand)
            pop de              ; DE = addr of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a             ; DE = value of var2 (multiplier)
            ; save SP so we can pop result addr later
            ld (mul_result_sp), sp
            ; inline 16-bit multiply DE * second_value -> HL
            ; multiplier is in DE, multiplicand saved to memory
            ld (mul_mcand), hl  ; save multiplicand
            ex de, hl           ; HL = multiplier value
            ld (mul_mplier), hl
            ld de, 0            ; DE = accumulator (result)
            ld a, 16
mul16_loop:
            ld hl, (mul_mplier)
            srl h
            rr l                ; shift multiplier right, LSB to carry
            ld (mul_mplier), hl
            jr nc, mul16_noadd
            ld hl, (mul_mcand)
            add hl, de          ; add multiplicand to result
            ex de, hl           ; DE = updated result
mul16_noadd:
            ld hl, (mul_mcand)
            add hl, hl          ; shift multiplicand left
            ld (mul_mcand), hl
            dec a
            jr nz, mul16_loop
            ; DE = product
            ld sp, (mul_result_sp)
            pop hl              ; HL = addr of result var
            ld (hl), e
            inc hl
            ld (hl), d
            ret

; kw_or16 - result = var1 | var2
kw_or16:
            pop de              ; DE = addr of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a             ; HL = value of var1
            pop de              ; DE = addr of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a             ; DE = value of var2
            ld a, l
            or e
            ld l, a
            ld a, h
            or d
            ld h, a             ; HL = var1 | var2
            pop de              ; DE = addr of result
            ex de, hl
            ld (hl), e
            inc hl
            ld (hl), d
            ret

; kw_cplt16 - compare less-than
; result = (var1 < var2) ? 1  0
kw_cplt16:
            pop de              ; DE = addr of var1
            ld a, (de)
            inc de
            ld h, (de)
            ld l, a             ; HL = value of var1
            pop de              ; DE = addr of var2
            ld a, (de)
            inc de
            ld d, (de)
            ld e, a             ; DE = value of var2
            or a
            sbc hl, de          ; carry set if var1 < var2
            pop hl              ; HL = addr of result
            jr c, cplt_true
            xor a
            ld (hl), a
            inc hl
            ld (hl), a
            ret
cplt_true:
            ld (hl), 1
            inc hl
            ld (hl), 0
            ret

; --- Memory access ---

; kw_peek - 2 args (src_ptr_var, dest_var)
; reads single byte at address stored in src_ptr_var
; zero-extends to 16-bit in dest_var
kw_peek:
            pop hl              ; HL = addr of src pointer var
            ld e, (hl)
            inc hl
            ld d, (hl)          ; DE = pointer value
            ld a, (de)          ; A = byte at that address
            pop hl              ; HL = addr of dest var
            ld (hl), a
            inc hl
            ld (hl), 0          ; zero-extend
            ret

; kw_peek16 - 2 args (src_ptr_var, dest_var)
; reads 16-bit word at address stored in src_ptr_var
kw_peek16:
            pop hl              ; HL = addr of src pointer var
            ld e, (hl)
            inc hl
            ld d, (hl)          ; DE = pointer value
            ld a, (de)          ; A = low byte
            inc de
            ld b, (de)          ; B = high byte
            pop hl              ; HL = addr of dest var
            ld (hl), a
            inc hl
            ld (hl), b
            ret

; kw_poke - 2 args (dest_ptr_var, src_val_var)
; writes low byte from src_val_var to address in dest_ptr_var
kw_poke:
            pop hl              ; HL = addr of dest pointer var
            ld e, (hl)
            inc hl
            ld d, (hl)          ; DE = destination address
            pop hl              ; HL = addr of source value var
            ld a, (hl)          ; A = byte to write
            ld (de), a
            ret

; kw_poke16 - 2 args (dest_ptr_var, src_val_var)
; writes 16-bit word from src_val_var to address in dest_ptr_var
kw_poke16:
            pop hl              ; HL = addr of dest pointer var
            ld e, (hl)
            inc hl
            ld d, (hl)          ; DE = destination address
            pop hl              ; HL = addr of source value var
            ld a, (hl)
            ld (de), a          ; write low byte
            inc hl
            inc de
            ld a, (hl)
            ld (de), a          ; write high byte
            ret

; kw_ld_ptr - 2 args (dest_var, src_ptr_var)
; copies 16-bit value from src to dest
kw_ld_ptr:
            pop hl              ; HL = addr of dest var
            pop de              ; DE = addr of src var
            ld a, (de)
            ld (hl), a
            inc hl
            inc de
            ld a, (de)
            ld (hl), a
            ret

; --- Random number ---

; kw_random - 2 args (range_var, result_var)
; result = random number in [0, range-1]
; RNG and modulo fully inlined, no call/push
kw_random:
            pop hl              ; HL = addr of range var
            ld e, (hl)
            inc hl
            ld d, (hl)          ; DE = range value
            ; save range and SP
            ld (rng_range), de
            ld (rng_result_sp), sp
            ; check for zero range
            ld a, d
            or e
            jr z, rng_zero
            ; --- inline xorshift RNG ---
            ld hl, (rng_state)
            ld a, h
            xor l
            ld h, a
            add hl, hl
            ld a, h
            xor l
            ld h, a
            ld (rng_state), hl
            ; HL = random 16-bit value
            ; reload range
            ld de, (rng_range)
            ex de, hl           ; DE = random, HL = range
            ; --- inline DE mod HL (restoring division) ---
            ld b, h
            ld c, l             ; BC = divisor (range)
            ld hl, 0            ; HL = partial remainder
            ld a, 16
rng_div_loop:
            sla e
            rl d                ; shift dividend left, MSB to carry
            adc hl, hl          ; shift carry into remainder
            or a
            sbc hl, bc          ; try subtract divisor
            jr nc, rng_div_ok   ; no borrow, subtraction is good
            add hl, bc          ; borrow, restore remainder
rng_div_ok:
            dec a
            jr nz, rng_div_loop
            ; HL = remainder (random mod range)
            ex de, hl           ; DE = remainder
            jr rng_store
rng_zero:
            ld de, 0
rng_store:
            ld sp, (rng_result_sp)
            pop hl              ; HL = addr of result var
            ld (hl), e
            inc hl
            ld (hl), d
            ret

; ============================================================
; Scratch space for keywords that cant use the stack
; ============================================================

mul_result_sp:  dw 0
mul_mcand:      dw 0
mul_mplier:     dw 0
rng_range:      dw 0
rng_result_sp:  dw 0
rng_state:      dw #1234

; ============================================================
; Variables
; ============================================================

encode_count:   dw 0
decode_count:   dw 0

tmp0:           dw 0
tmp1:           dw 0
tmp2:           dw 0
tmp3:           dw 0
tmp4:           dw 0
tmp5:           dw 0
tmp6:           dw 0
tmp7:           dw 0
tmp8:           dw 0

byte_counter:   dw 0
byte_val:       dw 0
cnt_val:        dw 0
off_val:        dw 0
fill_pos:       dw 0
running_offset: dw 0
rom_index:      dw 0

enc_src:        dw 0
enc_len:        dw 0
enc_dst:        dw 0
enc_idx:        dw 0
enc_char:       dw 0
enc_off:        dw 0
enc_cnt:        dw 0
enc_pos:        dw 0
enc_index:      dw 0
enc_adr:        dw 0

dec_rom:        dw 0
dec_src:        dw 0
dec_dst:        dw 0
dec_len:        dw 0
dec_idx:        dw 0
dec_adr:        dw 0
dec_byte:       dw 0

; ============================================================
; Pointer variables (contain addresses of data buffers)
; ============================================================

rom_data_ptr:       dw rom_data
count_array_ptr:    dw count_array
offset_table_ptr:   dw offset_table
fill_table_ptr:     dw fill_table
addr_list_ptr:      dw addr_list
test_msg_ptr:       dw test_msg
enc_buffer_ptr:     dw enc_buffer
dec_buffer_ptr:     dw dec_buffer

; ============================================================
; Data
; ============================================================

test_msg:       db "This is a test message for ZOSCII encode decode speed testing."
                ds MSG_SIZE-64, 0

; ============================================================
; Software stack for gosub/return (grows downward)
; ============================================================

software_stack:     ds 128
software_stack_top:

; ============================================================
; Data buffers (large, at end)
; ============================================================

rom_data:       ds ROM_SIZE         ; 8192 bytes
count_array:    ds 1024             ; 256 entries x 4 bytes
offset_table:   ds 1024             ; 256 entries x 4 bytes (offset + count)
fill_table:     ds 512              ; 256 entries x 2 bytes
addr_list:      ds 16384            ; up to 8192 entries x 2 bytes
enc_buffer:     ds ENC_SIZE         ; 64 x 2 = 128 bytes
dec_buffer:     ds MSG_SIZE         ; 64 bytes