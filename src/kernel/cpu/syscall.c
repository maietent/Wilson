#include "syscall.h"
#include "idt.h"
#include "klog.h"

#define IDT_SYSCALL 0x80

extern void syscall_handler_stub();

void syscall_handler_c(uint64_t rax, uint64_t rbx, uint64_t rcx)
{
    klogf("syscall called\n");
    switch (rax)
    {
        case 1:
            klogf("%c", (char)rbx);
            break;
        default:
            klogf("default syscall\n");
            break;
    }
}

void syscall_init(void)
{
    idt_set_entry(IDT_SYSCALL, (uint64_t)syscall_handler_stub, 0x08, 0x8E);
}
