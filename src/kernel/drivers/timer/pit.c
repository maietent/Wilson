#include "pit.h"
#include "idt.h"
#include "ports.h"
#include "klog.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_FREQ 1193182ULL
#define PIT_IRQ 0
#define INTERRUPT_ACK 0x20

extern void pit_handler_stub(void);

volatile uint64_t pit_ticks = 0;
static uint32_t pit_frequency = 1000;

void pit_handler_c(void)
{
    pit_ticks++;

    outb(0x20, INTERRUPT_ACK);
}

void pit_init(uint32_t frequency)
{
    if (frequency == 0) return;

    pit_frequency = frequency;

    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / frequency);

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    idt_set_entry(32 + PIT_IRQ, (uint64_t)pit_handler_stub, 0x08, 0x8E);
}

uint64_t pit_get_ticks(void)
{
    return pit_ticks;
}

void pit_sleep_ms(uint64_t ms)
{
    uint64_t start = pit_ticks;
    uint64_t wait_ticks = (ms * pit_frequency) / 1000;
    while ((pit_ticks - start) < wait_ticks) {
        asm volatile ("hlt");
    }
}
