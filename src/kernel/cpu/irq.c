#include "irq.h"

uint64_t irq_save(void)
{
    uint64_t flags;
    asm volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

void irq_restore(uint64_t flags)
{
    asm volatile(
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}
