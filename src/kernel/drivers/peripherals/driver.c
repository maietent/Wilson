#include "driver.h"
#include "keyboard.h"
#include "mouse.h"

void load_drivers(void)
{
    mouse_init();
    keyboard_init();
}
