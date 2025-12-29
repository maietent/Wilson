#include "keyboard.h"
#include "idt.h"
#include "ports.h"
#include "scancodes.h"
#include "klog.h"

#define IDT_PS2_KEYBOARD 33
#define KEYBOARD_PORT 0x60
#define KEYBOARD_CMD_PORT 0x64
#define INTERRUPT_ACK 0x20

extern void keyboard_handler_stub();

static keyboard_handler_fn user_keyboard_handler = NULL;

void keyboard_handler_c(void)
{
    if (inb(KEYBOARD_CMD_PORT) & 0X01)
    {
        uint8_t scancode = inb(KEYBOARD_PORT);

        if (scancode & 0x80)
        {
            outb(0x20, INTERRUPT_ACK);
            return;
        }

        char key_char = scancode_to_char[scancode];

        if (user_keyboard_handler != NULL)
        {
            user_keyboard_handler(key_char, scancode);
        }
    }
    outb(0x20, INTERRUPT_ACK);
}

void keyboard_init(keyboard_handler_fn handler)
{
    user_keyboard_handler = handler;
    idt_set_entry(IDT_PS2_KEYBOARD, (uint64_t)keyboard_handler_stub, 0x08, 0x8E);
}
