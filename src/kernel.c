/* Minimal 64-bit kernel in C.
   Provides kernel_main which writes "Hello World" (or "Hallo Welt") to VGA text buffer.
*/

#include <stdint.h>
#include <generated/autoconf.h>

void kernel_main(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void write_string(const char *s) {
    volatile uint16_t *video = (volatile uint16_t*)0xB8000;
    const char *p = s;
    int i = 0;
    while (*p) {
        uint8_t ch = (uint8_t)*p++;
        video[i++] = (uint16_t)ch | (uint16_t)0x0700; /* white on black */
    }
}

void kernel_main(void) {
#ifdef CONFIG_LANG_DE
    const char *msg = "Hallo Welt";
#else
    const char *msg = "Hello World";
#endif

#ifdef CONFIG_HELLO
    write_string(msg);
#endif
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
