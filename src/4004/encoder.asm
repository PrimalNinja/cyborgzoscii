; ============================================
; ZOSCII ENCODER for Intel 4004 - UNTESTED
; Input: Message in INPUT_BUFFER (RAM)
; Output: Address pairs in OUTPUT_BUFFER (RAM)
; Uses: Sequential search with 8-bit instance counter
; ============================================

; Memory map:
; RAM 0x40-0x7F: Input buffer (64 bytes max)
; RAM 0x80-0xFF: Output buffer (128 bytes = 64 address pairs)
; RAM 0x30-0x31: LFSR state for PRNG

; Register allocation:
; R0,R1: Temporary (used by FIN)
; R2,R3: ROM pointer (P1) - 12-bit address
; R4,R5: Input RAM pointer (P2) - 8-bit
; R6,R7: Output RAM pointer (P3) - 8-bit
; R8,R9: Instance counter (8-bit: high, low)
; R10,R11: Target instance (8-bit: high, low)
; R12: Message length counter
; R13: Target character to find
; R14,R15: Scratch

; ============================================
; MAIN ENCODER ENTRY POINT
; ============================================
ENCODE_SETUP:
    ; Initialize message length (must be ≤64)
    FIM P6, 0x40     ; Load 0x40 into R12:R13
    LD R12           ; Get high nibble (4)
    XCH R12          ; R12 = 4 (represents 64 in loop logic)
    
    ; Initialize input pointer (P2 = R4:R5)
    FIM P2, 0x40     ; Input buffer at 0x40
    
    ; Initialize output pointer (P3 = R6:R7)
    FIM P3, 0x80     ; Output buffer at 0x80
    
    ; Initialize PRNG seed
    JMS PRNG_INIT

; ============================================
; MAIN ENCODING LOOP
; ============================================
ENCODE_LOOP:
    ; Get next character from input buffer
    SRC P2           ; Select RAM at R4:R5
    RDM              ; Read character
    XCH R13          ; R13 = target character
    
    ; Get random instance number (0-255)
    JMS PRNG_NEXT    ; Returns 8-bit value in R10:R11
    
    ; Initialize ROM search pointer
    FIM P1, 0x00     ; Start at ROM address 0x000
    
    ; Initialize instance counter to 0
    LDM 0
    XCH R8           ; R8 = 0 (high nibble)
    XCH R9           ; R9 = 0 (low nibble)

; ============================================
; SEQUENTIAL SEARCH LOOP
; ============================================
SEARCH_LOOP:
    ; Read byte from ROM at current address
    SRC P1           ; Set ROM address from R2:R3
    FIN P0           ; Fetch ROM byte into R0:R1
    LD R1            ; Get the data byte (low nibble)
    
    ; Compare with target character
    CLC              ; Clear carry
    SUB R13          ; ACC = ROM[addr] - target
    JCN NZ, NOT_MATCH ; If not zero, not a match
    
    ; Found a match - check if it's the Nth instance
    LD R9            ; Get instance counter low
    CLC
    SUB R11          ; Compare with target instance low
    JCN NZ, NOT_NTH  ; Low nibbles don't match
    
    LD R8            ; Get instance counter high
    CLC
    SUB R10          ; Compare with target instance high
    JCN Z, FOUND_IT  ; Both nibbles match - this is the one!
    
NOT_NTH:
    ; Increment instance counter (8-bit)
    ISZ R9, SEARCH_LOOP ; Inc low nibble, continue if not zero
    ISZ R8, SEARCH_LOOP ; Inc high nibble (carry)
    ; If we overflow 255 instances, wrap to 0 and continue
    JUN SEARCH_LOOP

NOT_MATCH:
    ; Advance ROM pointer (12-bit increment)
    ISZ R2, SEARCH_LOOP ; Inc low nibble
    JCN C, INC_MID      ; If carried out
    JUN SEARCH_LOOP

INC_MID:
    ISZ R3, SEARCH_LOOP ; Inc middle nibble
    ; Note: 4004 has 4K ROM (0x000-0xFFF)
    ; If we exceed 4K, we've failed (should not happen)
    ; Production code should handle this error
    JUN SEARCH_LOOP

; ============================================
; FOUND TARGET - STORE ADDRESS
; ============================================
FOUND_IT:
    ; Store 12-bit ROM address as two bytes
    ; Low byte: R2 (low nibble of address)
    SRC P3           ; Select output RAM
    LD R2            ; Get address low nibble
    WRM              ; Write to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT1    ; Inc low nibble
    ISZ R7, CONT1    ; Inc high on carry
    
CONT1:
    ; High byte: R3 (middle nibble of address)
    SRC P3           ; Update selection (pointer changed)
    LD R3            ; Get address middle nibble
    WRM              ; Write to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT2
    ISZ R7, CONT2
    
CONT2:
    ; Increment input pointer with carry
    ISZ R4, CONT3
    ISZ R5, CONT3
    
CONT3:
    ; Decrement message counter and loop
    ISZ R12, ENCODE_LOOP ; 4004 ISZ skips if result is ZERO
    
    BBL 0            ; Return success

; ============================================
; PRNG: 8-bit Linear Feedback Shift Register
; Uses Galois LFSR with polynomial x^8+x^6+x^5+x^4+1
; Taps at bits 6,5,4
; ============================================
PRNG_INIT:
    ; Initialize LFSR state in RAM 0x30-0x31
    FIM P3, 0x30     ; Point to LFSR storage
    SRC P3
    LDM 0x0A         ; Seed high nibble
    WRM
    
    ISZ R6, PRNGI2
PRNGI2:
    SRC P3
    LDM 0x05         ; Seed low nibble (0xA5)
    WRM
    
    BBL 0

PRNG_NEXT:
    ; Read current state
    FIM P3, 0x30
    SRC P3
    RDM              ; Read high nibble
    XCH R14          ; Store in R14
    
    ISZ R6, PRNGN2
PRNGN2:
    SRC P3
    RDM              ; Read low nibble
    XCH R15          ; Store in R15
    
    ; Perform 8 shift iterations for full byte
    LDM 8
    XCH R13          ; R13 = iteration counter
    
PRNG_SHIFT:
    ; Get LSB of R15
    LD R15
    RAR              ; Rotate right through carry
    JCN C, PRNG_TAP  ; If LSB was 1, apply taps
    
    ; Shift right (R14:R15 >> 1)
    LD R14
    RAR              ; Shift high nibble right
    XCH R14
    LD R15
    RAR              ; Shift low nibble right
    XCH R15
    JUN PRNG_CONT
    
PRNG_TAP:
    ; Apply XOR taps (0x60 = bits 6,5)
    LD R14
    RAR              ; Shift high nibble
    XCH R14
    CLC
    LDM 6            ; Tap bits
    ADD R14
    XCH R14          ; XOR high nibble
    
    LD R15
    RAR
    XCH R15

PRNG_CONT:
    ISZ R13, PRNG_SHIFT
    
    ; Store updated state
    FIM P3, 0x30
    SRC P3
    LD R14
    WRM
    ISZ R6, PRNGN3
PRNGN3:
    SRC P3
    LD R15
    WRM
    
    ; Return value in R10:R11
    LD R14
    XCH R10
    LD R15
    XCH R11
    
    BBL 0