#pragma once

#include "std.h"

typedef enum {
    INPUT_KEY,
    INPUT_MOUSE_MOVE,
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    union {
        struct {
            char key_char;
            uint8_t scancode;
        } key;

        struct {
            int x;
            int y;
        } mouse;
    };
} input_event_t;

bool input_push_key(char key_char, uint8_t scancode);
bool input_push_mouse_move(int x, int y);

bool input_pop_event(input_event_t *out);
