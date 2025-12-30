#include "mouse.h"
#include "idt.h"
#include "ports.h"
#include "klog.h"
#include "shell.h"
#include "framebuffer.h"

#define IDT_PS2_MOUSE 44
#define MOUSE_PORT 0x60
#define MOUSE_CMD_PORT 0x64

#define PIC1_COMMAND 0x20
#define PIC2_COMMAND 0xA0
#define PIC_EOI      0x20

#define MOUSE_WRITE  0xD4

#define CURSOR_WIDTH 32
#define CURSOR_HEIGHT 32

static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];
static int cursor_x = 0;
static int cursor_y = 0;

extern void mouse_handler_stub(void);

static void mouse_wait_input_clear(void)
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        if (!(inb(MOUSE_CMD_PORT) & 0x02))
            return;
    }
}

static void mouse_wait_output_full(void)
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        if (inb(MOUSE_CMD_PORT) & 0x01)
            return;
    }
}

static void mouse_write(uint8_t data)
{
    mouse_wait_input_clear();
    outb(MOUSE_CMD_PORT, MOUSE_WRITE);
    mouse_wait_input_clear();
    outb(MOUSE_PORT, data);
}

static uint8_t mouse_read(void)
{
    mouse_wait_output_full();
    return inb(MOUSE_PORT);
}

static inline void mouse_eoi(void)
{
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void mouse_handler_c(void)
{
    uint8_t status = inb(MOUSE_CMD_PORT);

    if (!(status & 0x20))
    {
        mouse_eoi();
        return;
    }

    uint8_t data = inb(MOUSE_PORT);

    switch (mouse_cycle)
    {
        case 0:
            if (data & 0x08)
            {
                mouse_bytes[0] = data;
                mouse_cycle = 1;
            }
            break;

        case 1:
            mouse_bytes[1] = data;
            mouse_cycle = 2;
            break;

        case 2:
        {
            mouse_bytes[2] = data;
            mouse_cycle = 0;

            int8_t dx = mouse_bytes[1];
            int8_t dy = mouse_bytes[2];

            cursor_x += dx;
            cursor_y -= dy;

            int fb_width = get_fb_width();
            int fb_height = get_fb_height();

            if (cursor_x < 0) cursor_x = 0;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_x > fb_width - (CURSOR_WIDTH - 16)) cursor_x = fb_width - ((CURSOR_WIDTH / 2) - 4);
            if (cursor_y > fb_height - CURSOR_HEIGHT) cursor_y = fb_height - CURSOR_HEIGHT;

            if (dx || dy)
            {
                s_handle_mouse(cursor_x, cursor_y);
                //klogf("Cursor: X=%d Y=%d\n", cursor_x, cursor_y);
            }

            break;
        }
    }

    mouse_eoi();
}

void mouse_init(void)
{
    mouse_wait_input_clear();
    outb(MOUSE_CMD_PORT, 0xA8);

    mouse_wait_input_clear();
    outb(MOUSE_CMD_PORT, 0x20);
    mouse_wait_output_full();
    uint8_t status = inb(MOUSE_PORT);

    status |= 0x02;
    status &= ~0x20;

    mouse_wait_input_clear();
    outb(MOUSE_CMD_PORT, 0x60);
    mouse_wait_input_clear();
    outb(MOUSE_PORT, status);

    mouse_write(0xF4);
    mouse_read();

    idt_set_entry(IDT_PS2_MOUSE, (uint64_t)mouse_handler_stub, 0x08, 0x8E);
}
