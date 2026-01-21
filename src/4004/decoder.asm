; ============================================
; ZOSCII DECODER for Intel 4004 - UNTESTED
; Input: Address pairs in OUTPUT_BUFFER (RAM)
; Output: Plaintext in INPUT_BUFFER (RAM)
; The "airtight" hardware indirection implementation
; ============================================

; Memory map:
; RAM 0x40-0x7F: Output plaintext buffer (64 bytes)
; RAM 0x80-0xFF: Input address buffer (128 bytes = 64 pairs)

; Register allocation:
; R0,R1: Temporary (used by FIN)
; R2,R3: ROM pointer (P1) - reconstructed 12-bit address
; R4,R5: Input address pointer (P2) - 8-bit
; R6,R7: Output plaintext pointer (P3) - 8-bit
; R12: Pair counter (number of address pairs to decode)
; R13,R14,R15: Scratch

; ============================================
; MAIN DECODER ENTRY POINT
; ============================================
DECODE_SETUP:
    ; Initialize pair counter (64 pairs max)
    FIM P6, 0x40     ; Load 0x40 into R12:R13
    LD R12           ; Get high nibble (4)
    XCH R12          ; R12 = 4 (represents 64 pairs)
    
    ; Initialize input pointer (address buffer)
    FIM P2, 0x80     ; Input addresses at 0x80
    
    ; Initialize output pointer (plaintext buffer)
    FIM P3, 0x40     ; Output plaintext at 0x40

; ============================================
; MAIN DECODING LOOP
; ============================================
DECODE_LOOP:
    ; Read address LOW nibble from RAM
    SRC P2           ; Select input address buffer
    RDM              ; Read low nibble
    XCH R2           ; Store in ROM pointer low
    
    ; Increment input pointer with carry
    ISZ R4, CONT1
    ISZ R5, CONT1
    
CONT1:
    ; Read address MID nibble from RAM
    SRC P2           ; Re-select (pointer changed)
    RDM              ; Read middle nibble
    XCH R3           ; Store in ROM pointer mid
    
    ; Note: 4004 has 12-bit addressing (3 nibbles)
    ; We're only storing/reading 2 nibbles (8 bits)
    ; This limits us to 256 ROM addresses
    ; For full 4K ROM, need to store 3 nibbles
    
    ; Increment input pointer with carry
    ISZ R4, CONT2
    ISZ R5, CONT2

CONT2:
    ; ========================================
    ; CORE HARDWARE INDIRECTION
    ; This is the cryptographic primitive:
    ; ROM[address] cannot be predicted without
    ; physical access to the ROM contents
    ; ========================================
    SRC P1           ; Set ROM address from R2:R3
    FIN P0           ; Hardware fetch: ROM[addr] → R0:R1
    
    ; The byte we want is in R1 (low nibble of ROM word)
    LD R1            ; Get the plaintext character
    
    ; Store plaintext byte to output buffer
    SRC P3           ; Select output buffer
    WRM              ; Write character to RAM
    
    ; Increment output pointer with carry
    ISZ R6, CONT3
    ISZ R7, CONT3

CONT3:
    ; Decrement pair counter and loop
    ; ISZ on 4004 skips next instruction if result is ZERO
    ISZ R12, DECODE_LOOP
    
    BBL 0            ; Return success

; ============================================
; ENHANCED VERSION: Full 12-bit address support
; (Comment out above, uncomment this for 4K ROM)
; ============================================
;DECODE_LOOP_12BIT:
;    ; Read address LOW nibble
;    SRC P2
;    RDM
;    XCH R2
;    ISZ R4, DL1
;    ISZ R5, DL1
;    
;DL1:
;    ; Read address MID nibble  
;    SRC P2
;    RDM
;    XCH R3
;    ISZ R4, DL2
;    ISZ R5, DL2
;
;DL2:
;    ; Read address HIGH nibble (only lower 4 bits used)
;    SRC P2
;    RDM
;    XCH R14          ; Temporarily store high nibble
;    ISZ R4, DL3
;    ISZ R5, DL3
;
;DL3:
;    ; The 4004 SRC command only uses R2:R3 for addressing
;    ; To support 12-bit (4K), need custom ROM access
;    ; This would require bank-switching or external logic
;    
;    ; Standard approach: Use R2:R3 (8-bit = 256 addresses)
;    SRC P1
;    FIN P0
;    LD R1
;    
;    SRC P3
;    WRM
;    ISZ R6, DL4
;    ISZ R7, DL4
;
;DL4:
;    ISZ R12, DECODE_LOOP_12BIT
;    BBL 0

; ============================================
; ERROR HANDLING (Optional)
; ============================================
DECODE_ERROR:
    ; If pointer overflow or other error
    LDM 0x0F         ; Error code
    BBL 0x0F         ; Return with error

; ============================================
; UTILITY: Clear output buffer before decode
; ============================================
CLEAR_OUTPUT:
    FIM P3, 0x40     ; Output buffer start
    LDM 0x04         ; Counter high nibble (64 bytes)
    XCH R12
    
CLR_LOOP:
    SRC P3
    LDM 0            ; Zero
    WRM              ; Write to RAM
    ISZ R6, CLR_LOOP
    ISZ R7, CLR2
CLR2:
    ISZ R12, CLR_LOOP
    BBL 0

; ============================================
; NOTES ON 4004 ARCHITECTURE
; ============================================
; 1. ROM addressing is 12-bit (0x000-0xFFF = 4096 bytes)
;    but SRC only uses R2:R3 (8 bits = 256 addresses)
;    
; 2. FIN instruction: Fetches 8-bit word from ROM
;    into register pair (R0:R1, R2:R3, etc.)
;    Format: [R(n)] = high nibble, [R(n+1)] = low nibble
;
; 3. For full 4K ROM access, need external banking logic
;    or store addresses as 3 nibbles (12 bits)
;
; 4. The security comes from hardware indirection:
;    Without reading the actual ROM chip, an adversary
;    cannot determine ROM[addr] from addr alone
;
; 5. This implementation uses 8-bit addresses (256 slots)
;    which is sufficient for most character sets
; ============================================