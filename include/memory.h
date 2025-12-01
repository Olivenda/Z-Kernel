#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "stivale2.h"

void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);

void memory_init(struct stivale2_struct *boot_info);
void *bump_alloc(size_t size, size_t align);
const struct stivale2_mmap_tag *memory_get_mmap(void);

#endif
