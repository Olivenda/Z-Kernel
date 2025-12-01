#ifndef STIVALE2_H
#define STIVALE2_H

#include <stdint.h>

#define STIVALE2_BOOTLOADER_BRAND_SIZE 64
#define STIVALE2_BOOTLOADER_VERSION_SIZE 64
#define STIVALE2_MMAP_USABLE 1

struct stivale2_tag {
    uint64_t identifier;
    uint64_t next;
} __attribute__((packed));

struct stivale2_struct {
    uint64_t bootloader_brand;
    uint64_t bootloader_version;
    uint64_t tags;
} __attribute__((packed));

struct stivale2_mmap_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t unused;
} __attribute__((packed));

struct stivale2_mmap_tag {
    struct stivale2_tag tag;
    uint64_t entries;
    struct stivale2_mmap_entry memmap[];
} __attribute__((packed));

struct stivale2_framebuffer_tag {
    struct stivale2_tag tag;
    uint64_t framebuffer_addr;
    uint16_t framebuffer_width;
    uint16_t framebuffer_height;
    uint16_t framebuffer_pitch;
    uint16_t framebuffer_bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
} __attribute__((packed));

#endif
