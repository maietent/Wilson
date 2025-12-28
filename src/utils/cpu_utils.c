#include "cpu_utils.h"
#include "klog.h"

void CU_halt(void)
{
    klogf("CPU halted\n");
    asm volatile("hlt");
}
