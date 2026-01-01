#include "cpu_utils.h"
#include "klog.h"
#include "pit.h"

void CU_halt(void)
{
    //klogf("CPU halted\n");
    for (;;)
        asm volatile("hlt");
}

void sleep_ms(uint64_t ms)
{
    pit_wait(ms);
}
