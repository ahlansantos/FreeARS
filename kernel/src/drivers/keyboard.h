#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void keyboard_set_cursor_cb(void (*cb)(int visible));

void keyboard_readline(char *buf, int max);

#endif