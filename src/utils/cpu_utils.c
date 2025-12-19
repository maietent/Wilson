#include "cpu_utils.h"

void CU_halt(void)
{
    asm volatile("hlt");
}
