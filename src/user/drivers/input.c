#include "input.h"

#define INPUT_QUEUE_SIZE 128

static volatile input_event_t input_queue[INPUT_QUEUE_SIZE];
static volatile uint32_t input_head = 0;
static volatile uint32_t input_tail = 0;

static inline bool input_queue_full(uint32_t next)
{
    return next == input_tail;
}

static inline bool input_queue_empty(void)
{
    return input_head == input_tail;
}

bool input_push_key(char key_char, uint8_t scancode)
{
    uint32_t next = (input_head + 1) % INPUT_QUEUE_SIZE;
    if (input_queue_full(next))
        return false;

    input_queue[input_head].type = INPUT_KEY;
    input_queue[input_head].key.key_char = key_char;
    input_queue[input_head].key.scancode = scancode;

    input_head = next;
    return true;
}

bool input_push_mouse_move(int x, int y)
{
    uint32_t next = (input_head + 1) % INPUT_QUEUE_SIZE;
    if (input_queue_full(next))
        return false;

    input_queue[input_head].type = INPUT_MOUSE_MOVE;
    input_queue[input_head].mouse.x = x;
    input_queue[input_head].mouse.y = y;

    input_head = next;
    return true;
}

bool input_pop_event(input_event_t *out)
{
    if (input_queue_empty())
        return false;

    *out = input_queue[input_tail];
    input_tail = (input_tail + 1) % INPUT_QUEUE_SIZE;
    return true;
}
