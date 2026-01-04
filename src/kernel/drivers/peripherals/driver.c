#include "driver.h"
#include "keyboard.h"
#include "mouse.h"
#include "pit.h"
#include "syscall.h"
#include "ata.h"

void load_drivers(void)
{
    syscall_init();
    mouse_init();
    keyboard_init();
    pit_init(1000);
    ata_init();
}
