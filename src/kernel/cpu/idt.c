#include "idt.h"
#include "ports.h"
#include "exception.h"
#include "klog.h"

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_RING0   0x00
#define IDT_FLAG_INT_GATE 0x0E

struct idt_entry idt[256];
struct idt_ptr idt_descriptor;

extern void idt_load(void);

void idt_set_entry(int num, uint64_t base, uint16_t sel, uint8_t flags)
{
    if (num < 0 || num >= 256)
        return;

    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].base_upper = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

void pic_remap()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

bool idt_is_set(int num)
{
    return idt[num].base_low != 0 || idt[num].base_high != 0 || idt[num].flags != 0;
}

void register_exception_handlers() {
    idt_set_entry(0,  (uint64_t)isr0,  0x08, 0x8E);
    idt_set_entry(1,  (uint64_t)isr1,  0x08, 0x8E);
    idt_set_entry(2,  (uint64_t)isr2,  0x08, 0x8E);
    idt_set_entry(3,  (uint64_t)isr3,  0x08, 0x8E);
    idt_set_entry(4,  (uint64_t)isr4,  0x08, 0x8E);
    idt_set_entry(5,  (uint64_t)isr5,  0x08, 0x8E);
    idt_set_entry(6,  (uint64_t)isr6,  0x08, 0x8E);
    idt_set_entry(7,  (uint64_t)isr7,  0x08, 0x8E);
    idt_set_entry(8,  (uint64_t)isr8,  0x08, 0x8E);
    idt_set_entry(9,  (uint64_t)isr9,  0x08, 0x8E);
    idt_set_entry(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_entry(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_entry(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_entry(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_entry(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_entry(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_entry(16, (uint64_t)isr16, 0x08, 0x8E);
    idt_set_entry(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_entry(18, (uint64_t)isr18, 0x08, 0x8E);
    idt_set_entry(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_entry(20, (uint64_t)isr20, 0x08, 0x8E);
    idt_set_entry(21, (uint64_t)isr21, 0x08, 0x8E);
    idt_set_entry(22, (uint64_t)isr22, 0x08, 0x8E);
    idt_set_entry(23, (uint64_t)isr23, 0x08, 0x8E);
    idt_set_entry(24, (uint64_t)isr24, 0x08, 0x8E);
    idt_set_entry(25, (uint64_t)isr25, 0x08, 0x8E);
    idt_set_entry(26, (uint64_t)isr26, 0x08, 0x8E);
    idt_set_entry(27, (uint64_t)isr27, 0x08, 0x8E);
    idt_set_entry(28, (uint64_t)isr28, 0x08, 0x8E);
    idt_set_entry(29, (uint64_t)isr29, 0x08, 0x8E);
    idt_set_entry(30, (uint64_t)isr30, 0x08, 0x8E);
    idt_set_entry(31, (uint64_t)isr31, 0x08, 0x8E);
}

void dummy_handler(int irq) {
    klogf("IRQ %d received.\n", irq);
    outb(0x20, 0x20);
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
}

void idt_init(void)
{
    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (uint64_t)&idt;

    for (int i = 0; i < 256; i++)
    {
        idt_set_entry(i, 0, 0, 0);
    }

    register_exception_handlers();

    pic_remap();
    idt_load();
}
