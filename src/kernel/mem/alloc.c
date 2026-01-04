#include "alloc.h"
#include "irq.h"
#include "memory.h"

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

#define ALIGN 8
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(ALIGN - 1))

static block_t *heap_head = NULL;
static uintptr_t heap_end;

static void split_block(block_t *block, size_t size)
{
    if (block->size <= size + sizeof(block_t))
        return;

    block_t *new_block = (block_t *)((uintptr_t)block + sizeof(block_t) + size);
    new_block->size = block->size - size - sizeof(block_t);
    new_block->free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

static void coalesce()
{
    block_t *curr = heap_head;
    while (curr && curr->next)
    {
        if (curr->free && curr->next->free) {

            curr->size += sizeof(block_t) + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}

void init_heap(uintptr_t start, size_t size)
{
    uint32_t flags = irq_save();

    heap_head = (block_t *)start;
    heap_head->size = size - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = NULL;

    heap_end = start + size;

    irq_restore(flags);
}

void *kmalloc(size_t size)
{
    if (!size)
        return NULL;

    size = ALIGN_UP(size);
    uint32_t flags = irq_save();

    block_t *curr = heap_head;
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            split_block(curr, size);
            curr->free = 0;
            irq_restore(flags);
            return (void *)((uintptr_t)curr + sizeof(block_t));
        }
        curr = curr->next;
    }

    irq_restore(flags);
    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    uint32_t flags = irq_save();

    block_t *block = (block_t *)((uintptr_t)ptr - sizeof(block_t));
    block->free = 1;
    coalesce();

    irq_restore(flags);
}

void *krealloc(void *ptr, size_t size)
{
    if (!ptr)
        return kmalloc(size);

    if (size == 0)
    {
        kfree(ptr);
        return NULL;
    }

    block_t *block = (block_t *)((uintptr_t)ptr - sizeof(block_t));
    size = ALIGN_UP(size);

    if (block->size >= size)
        return ptr;

    void *new_ptr = kmalloc(size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, block->size);
    kfree(ptr);

    return new_ptr;
}
