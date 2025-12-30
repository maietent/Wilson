#include "keyboard.h"
#include "idt.h"
#include "ports.h"
#include "scancodes.h"
#include "klog.h"
#include "shell.h"

#define IDT_PS2_KEYBOARD 33
#define KEYBOARD_PORT 0x60
#define KEYBOARD_CMD_PORT 0x64
#define INTERRUPT_ACK 0x20

bool shift_pressed = false;

extern void keyboard_handler_stub();

void keyboard_handler_c(void)
{
    if (!(inb(KEYBOARD_CMD_PORT) & 0x01))
    {
        outb(0x20, INTERRUPT_ACK);
        return;
    }

    uint8_t scancode = inb(KEYBOARD_PORT);
    bool released = scancode & 0x80;
    uint8_t key = scancode & 0x7F;

    if (key == 0x2A || key == 0x36)
    {
        shift_pressed = !released;
        outb(0x20, INTERRUPT_ACK);
        return;
    }

    if (!released)
    {
        char key_char = shift_pressed ? scancode_to_char_shift[key] : scancode_to_char[key];
        s_handle_keyboard(key_char, scancode);
    }

    outb(0x20, INTERRUPT_ACK);
}

void keyboard_init()
{
    idt_set_entry(IDT_PS2_KEYBOARD, (uint64_t)keyboard_handler_stub, 0x08, 0x8E);
}
