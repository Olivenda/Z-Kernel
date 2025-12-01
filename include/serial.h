#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>

bool serial_init(void);
void serial_write(char c);

#endif
