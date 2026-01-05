#include "driver.h"
#include "keyboard.h"
#include "mouse.h"
#include "syscall.h"
#include "ata.h"
#include "pit.h"
#include "fat32.h"

void load_drivers(void)
{
    syscall_init();
    mouse_init();
    keyboard_init();
    pit_init(1000);
    ata_init();
    fat32_init();
}
