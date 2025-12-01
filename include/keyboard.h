#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

void keyboard_init(void);
bool keyboard_poll(char *out_ch);

#endif
