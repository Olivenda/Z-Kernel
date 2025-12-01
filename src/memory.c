#include "memory.h"
#include "console.h"

static const struct stivale2_mmap_tag *boot_mmap = 0;

struct allocator_state {
    uint64_t base;
    uint64_t size;
    uint64_t offset;
};

static struct allocator_state bump_state = {0};

void *memset(void *dest, int c, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (unsigned char)c;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }
    return 0;
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

static void select_allocator_region(void) {
    if (!boot_mmap || boot_mmap->entries == 0) {
        return;
    }

    uint64_t best_len = 0;
    uint64_t best_base = 0;

    for (uint64_t i = 0; i < boot_mmap->entries; i++) {
        const struct stivale2_mmap_entry *entry = &boot_mmap->memmap[i];
        if (entry->type != STIVALE2_MMAP_USABLE) {
            continue;
        }
        if (entry->length > best_len && entry->base >= 0x100000) {
            best_len = entry->length;
            best_base = entry->base;
        }
    }

    bump_state.base = best_base;
    bump_state.size = best_len;
    bump_state.offset = 0;
}

void memory_init(struct stivale2_struct *boot_info) {
    const uint64_t mmap_id = 0x2187f79e8612de07ULL;
    boot_mmap = (const struct stivale2_mmap_tag *)find_tag(boot_info, mmap_id);
    select_allocator_region();
}

void *bump_alloc(size_t size, size_t align) {
    if (bump_state.size == 0) {
        return 0;
    }

    uint64_t base = bump_state.base + bump_state.offset;
    if (align) {
        uint64_t mask = align - 1;
        base = (base + mask) & ~mask;
    }

    uint64_t new_offset = (base + size) - bump_state.base;
    if (new_offset > bump_state.size) {
        return 0;
    }

    bump_state.offset = new_offset;
    return (void *)(uintptr_t)base;
}

const struct stivale2_mmap_tag *memory_get_mmap(void) {
    return boot_mmap;
}
