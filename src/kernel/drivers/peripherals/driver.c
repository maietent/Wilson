#include "driver.h"
#include "keyboard.h"
#include "mouse.h"
#include "pit.h"
#include "syscall.h"

void load_drivers(void)
{
    syscall_init();
    mouse_init();
    keyboard_init();
    pit_init(1000);
}
