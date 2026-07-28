; sdot_asm - dot product kernel in x86-64
; float sdot_asm(int n, float *A, float *B)
; n comes in thru ecx, A in rdx, B in r8 (win x64 convention)
; answer goes back to C thru xmm0

section .data

section .text

default rel
bits 64
global sdot_asm

sdot_asm:
    xorps xmm0, xmm0            ; clear xmm0, this holds the running sum
    test ecx, ecx               ; edge case: n is 0 or negative
    jle FINISH                  ; nothing to do, just return 0

    movsxd rcx, ecx             ; make n 64-bit so it works as a loop bound
    mov rax, 0                  ; rax is our counter i, start at 0

L1: movss xmm1, [rdx + rax*4]   ; grab A[i] (*4 since floats are 4 bytes)
    mulss xmm1, [r8 + rax*4]    ; multiply it with B[i]
    addss xmm0, xmm1            ; add the product to the sum
    inc rax                     ; i++
    cmp rax, rcx                ; are we done yet?
    jl L1                       ; nope, keep going while i < n

FINISH:
    ret                         ; sum is already in xmm0 so just return