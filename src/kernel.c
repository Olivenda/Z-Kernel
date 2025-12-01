/* Minimal 64-bit kernel using Stivale2. */

#include <stdint.h>
#include <generated/autoconf.h>
#include "console.h"
#include "cpu.h"
#include "keyboard.h"
#include "memory.h"
#include "rootfs.h"
#include "stivale2.h"

static void scan_memory(void) {
    const struct stivale2_mmap_tag *tag = memory_get_mmap();
    if (!tag) {
        kprint("No memory map found.\n");
        return;
    }

    kprint("Memory map entries: %x\n", (uint64_t)tag->entries);
    for (uint64_t i = 0; i < tag->entries; i++) {
        const struct stivale2_mmap_entry *entry = &tag->memmap[i];
        if (entry->type != STIVALE2_MMAP_USABLE) {
            continue;
        }
        kprint(" - base %x length %x\n", entry->base, entry->length);
    }
}

static void heap_demo(void) {
    void *block = bump_alloc(4096, 16);
    if (!block) {
        kprint("Bump allocator out of memory\n");
        return;
    }
    memset(block, 0xAA, 4096);
    kprint("Allocated 4KiB at %x\n", (uint64_t)(uintptr_t)block);
}

static void print_boot_banner(void) {
    console_write("\n==============================\n");
    console_write("      Welcome to Z-Kernel\n");
    console_write("==============================\n");
    console_write("Booting with:");
#ifdef CONFIG_ENABLE_SERIAL_DEBUG
    console_write(" serial-debug");
#endif
#ifdef CONFIG_FRAMEBUFFER_ENABLE
    console_write(" framebuffer");
#endif
#ifdef CONFIG_ENABLE_KEYBOARD_ECHO
    console_write(" keyboard-echo");
#endif
    console_write("\n\n");
}

void kernel_main(struct stivale2_struct *boot_info) {
    console_init(boot_info);
    memory_init(boot_info);

#ifdef CONFIG_BOOT_BANNER
    print_boot_banner();
#endif

#ifdef CONFIG_HELLO
#ifdef CONFIG_LANG_DE
    const char *msg = "Hallo Welt";
#else
    const char *msg = "Hello World";
#endif
    console_write(msg);
    console_write("\n");
#endif

    kprint("Z-Kernel ready.\n");
    struct cpu_info cpu = {0};
    cpu_detect(&cpu);
    cpu_log(&cpu);
#ifdef CONFIG_LOG_MEMORY_MAP
    scan_memory();
#endif
#ifdef CONFIG_HEAP_DEMO
    heap_demo();
#endif
#ifdef CONFIG_LOG_ROOTFS
    rootfs_init();
    rootfs_log();
#endif

#ifdef CONFIG_ENABLE_KEYBOARD_ECHO
    keyboard_init();
    kprint("Keyboard polling active. Type to echo...\n");
#endif

    for (;;) {
#ifdef CONFIG_ENABLE_KEYBOARD_ECHO
        char c;
        if (keyboard_poll(&c)) {
            console_putc(c);
        }
#endif
        __asm__ volatile ("hlt");
    }
}
