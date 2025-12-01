#include "keyboard.h"
#include "io.h"
#include "console.h"

#define PS2_DATA 0x60
#define PS2_STATUS 0x64

static const char scancode_set1[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '};

void keyboard_init(void) {
    /* No special init required for basic polling */
    (void)0;
}

bool keyboard_poll(char *out_ch) {
    if (!(inb(PS2_STATUS) & 1)) {
        return false;
    }

    uint8_t sc = inb(PS2_DATA);
    if (sc & 0x80) {
        return false; /* ignore key releases */
    }

    if (sc < sizeof(scancode_set1)) {
        char c = scancode_set1[sc];
        if (c && out_ch) {
            *out_ch = c;
            return true;
        }
    }
    return false;
}
