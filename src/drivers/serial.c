#include "serial.h"
#include "io.h"

#define COM1_PORT 0x3F8

bool serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    return true;
}

static int serial_is_ready(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_write(char c) {
    while (!serial_is_ready()) {
    }
    outb(COM1_PORT, (uint8_t)c);
}
