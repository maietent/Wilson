#include "pit.h"
#include "idt.h"
#include "ports.h"

#define PIT_CHANNEL_0 0x40
#define PIT_CMD 0x43
#define IDT_PIT_TIMER 32

#define PIT_BASE_FREQ 1193182

static volatile uint64_t ticks = 0;

extern void pit_handler_stub(void);

void pit_handler_c(void) {
    ticks++;
    outb(0x20, 0x20);
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    outb(PIT_CMD, 0x36);

    outb(PIT_CHANNEL_0, divisor & 0xFF);
    outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF);

    idt_set_entry(IDT_PIT_TIMER, (uint64_t)pit_handler_stub, 0x08, 0x8E);
}

uint64_t pit_get_ticks(void) {
    return ticks;
}

void pit_wait(uint32_t wait_ticks) {
    uint64_t target = ticks + wait_ticks;
    while (ticks < target) {
        asm volatile("hlt");
    }
}
