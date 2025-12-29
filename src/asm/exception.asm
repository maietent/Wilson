global isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
global isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

extern exception_handler

section .text
bits 64

%macro EXC 2
isr%1:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    %if %2 = 0
        mov rsi, 0
    %endif

    mov rdi, %1

    call exception_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq
%endmacro

EXC 0, 0
EXC 1, 0
EXC 2, 0
EXC 3, 0
EXC 4, 0
EXC 5, 0
EXC 6, 0
EXC 7, 0
EXC 8, 1
EXC 9, 0
EXC 10,1
EXC 11,1
EXC 12,1
EXC 13,1
EXC 14,1
EXC 15,0
EXC 16,0
EXC 17,1
EXC 18,0
EXC 19,0
EXC 20,0
EXC 21,0
EXC 22,0
EXC 23,0
EXC 24,0
EXC 25,0
EXC 26,0
EXC 27,0
EXC 28,0
EXC 29,0
EXC 30,0
EXC 31,0
