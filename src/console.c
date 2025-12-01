#include <stdbool.h>
#include <stdint.h>
#include <generated/autoconf.h>
#include "console.h"
#include "memory.h"
#include "serial.h"

static volatile uint16_t *vga = (uint16_t *)0xB8000;
static uint16_t vga_row = 0;
static uint16_t vga_col = 0;
static uint16_t vga_color = 0x0700;
static bool serial_enabled = false;
static const struct stivale2_framebuffer_tag *framebuffer = 0;

static uint64_t parse_hex(const char *str, uint64_t fallback) {
    if (!str || !*str) {
        return fallback;
    }

    uint64_t value = 0;
    const char *p = str;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    while (*p) {
        char c = *p++;
        uint8_t digit;
        if (c >= '0' && c <= '9') {
            digit = (uint8_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (uint8_t)(10 + c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            digit = (uint8_t)(10 + c - 'A');
        } else {
            return fallback;
        }
        value = (value << 4) | digit;
    }

    return value;
}

static void vga_newline(void) {
    vga_col = 0;
    if (++vga_row >= 25) {
        vga_row = 24;
    }
}

static void vga_putc(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    const size_t idx = vga_row * 80 + vga_col;
    vga[idx] = (uint16_t)c | vga_color;
    if (++vga_col >= 80) {
        vga_newline();
    }
}

static void framebuffer_clear(void) {
    if (!framebuffer) {
        return;
    }
    uint32_t width = framebuffer->framebuffer_width;
    uint32_t height = framebuffer->framebuffer_height;
    uint32_t pitch = framebuffer->framebuffer_pitch;
    uint8_t bpp = framebuffer->framebuffer_bpp;

    if (bpp != 32) {
        return; /* keep code simple */
    }

    uint32_t color = (uint32_t)parse_hex(CONFIG_FRAMEBUFFER_BG_COLOR, 0x000000);
    uint32_t *fb = (uint32_t *)(uintptr_t)framebuffer->framebuffer_addr;
    for (uint32_t y = 0; y < height; y++) {
        uint32_t *row = (uint32_t *)((uintptr_t)fb + y * pitch);
        for (uint32_t x = 0; x < width; x++) {
            row[x] = color;
        }
    }
}

static void framebuffer_test_pattern(void) {
    if (!framebuffer) {
        return;
    }

    uint32_t width = framebuffer->framebuffer_width;
    uint32_t height = framebuffer->framebuffer_height;
    uint32_t pitch = framebuffer->framebuffer_pitch;
    uint8_t bpp = framebuffer->framebuffer_bpp;

    if (bpp != 32) {
        return;
    }

    uint32_t *fb = (uint32_t *)(uintptr_t)framebuffer->framebuffer_addr;
    for (uint32_t y = 0; y < height; y++) {
        uint32_t *row = (uint32_t *)((uintptr_t)fb + y * pitch);
        for (uint32_t x = 0; x < width; x++) {
            uint8_t r = (uint8_t)((x * 255) / (width ? width : 1));
            uint8_t g = (uint8_t)((y * 255) / (height ? height : 1));
            uint8_t b = (uint8_t)(((x ^ y) & 0xFF));
            row[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }
}

static const struct stivale2_tag *find_tag(struct stivale2_struct *info, uint64_t id) {
    uint64_t current = info ? info->tags : 0;
    while (current) {
        const struct stivale2_tag *tag = (const struct stivale2_tag *)current;
        if (tag->identifier == id) {
            return tag;
        }
        current = tag->next;
    }
    return 0;
}

void console_init(struct stivale2_struct *boot_info) {
    const uint64_t fb_id = 0x506461d2950408faULL;
    framebuffer = (const struct stivale2_framebuffer_tag *)find_tag(boot_info, fb_id);

    vga_color = (uint16_t)parse_hex(CONFIG_VGA_COLOR, 0x0700) & 0xFFFF;

#ifdef CONFIG_FRAMEBUFFER_ENABLE
    framebuffer_clear();
#ifdef CONFIG_FRAMEBUFFER_TEST_PATTERN
    framebuffer_test_pattern();
#endif
#endif

#ifdef CONFIG_ENABLE_SERIAL_DEBUG
    serial_enabled = serial_init();
#endif
}

void console_putc(char c) {
    vga_putc(c);
#ifdef CONFIG_ENABLE_SERIAL_DEBUG
    if (serial_enabled) {
        serial_write(c);
    }
#endif
}

void console_write(const char *s) {
    while (*s) {
        console_putc(*s++);
    }
}

static void kprint_hex(uint64_t value) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (uint8_t)(value & 0xF);
        buf[i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    }
    console_write("0x");
    console_write(buf);
}

void kprint(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            console_putc(*p);
            continue;
        }
        p++;
        switch (*p) {
        case 's': {
            const char *s = va_arg(args, const char *);
            console_write(s ? s : "(null)");
            break;
        }
        case 'x': {
            uint64_t v = va_arg(args, uint64_t);
            kprint_hex(v);
            break;
        }
        case '%':
            console_putc('%');
            break;
        default:
            console_putc('?');
            break;
        }
    }

    va_end(args);
}
