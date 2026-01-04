global dummy_stubs_start
extern dummy_handler

section .text
bits 64

%macro DUMMY_STUB 1
global dummy%1
dummy%1:
    push %1          ; push vector number
    jmp dummy_stubs_start
%endmacro

; Generate 256 stubs
%assign i 0
%rep 256
    DUMMY_STUB i
    %assign i i+1
%endrep

; Common entry point called by all stubs
dummy_stubs_start:
    ; Argument: vector number is on stack
    mov rdi, [rsp]   ; first argument in RDI
    add rsp, 8       ; remove vector from stack
    call dummy_handler
    iretq
