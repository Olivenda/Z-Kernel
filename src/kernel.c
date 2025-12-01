/* Minimal 64-bit kernel using Stivale2. */

#include <stdint.h>
#include <generated/autoconf.h>
#include "console.h"
#include "memory.h"
#include "keyboard.h"
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

void kernel_main(struct stivale2_struct *boot_info) {
    console_init(boot_info);
    memory_init(boot_info);

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
    scan_memory();
    heap_demo();

    keyboard_init();
    kprint("Keyboard polling active. Type to echo...\n");

    for (;;) {
        char c;
        if (keyboard_poll(&c)) {
            console_putc(c);
        }
        __asm__ volatile ("hlt");
    }
}
