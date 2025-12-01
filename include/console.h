#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "stivale2.h"

void console_init(struct stivale2_struct *boot_info);
void console_putc(char c);
void console_write(const char *s);
void kprint(const char *fmt, ...);

#endif
