#include "limine.h"
#include "cpu_utils.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
};

void limine_init(void)
{
    if(!LIMINE_BASE_REVISION_SUPPORTED) CU_halt();
    if(framebuffer_request.response == NULL) CU_halt();
    if(framebuffer_request.response->framebuffer_count < 1) CU_halt();
}

struct limine_framebuffer *get_framebuffer(void)
{
    return framebuffer_request.response->framebuffers[0];
}
