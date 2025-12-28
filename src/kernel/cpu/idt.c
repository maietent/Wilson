#include "idt.h"
#include "ports.h"

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

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
}

bool idt_is_set(int num)
{
    return idt[num].base_low != 0 || idt[num].base_high != 0 || idt[num].flags != 0;
}

void idt_init(void)
{
    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (uint64_t)&idt;

    for (int i = 0; i < 256; i++)
    {
        idt_set_entry(i, 0, 0, 0);
    }

    pic_remap();
    idt_load();
}
