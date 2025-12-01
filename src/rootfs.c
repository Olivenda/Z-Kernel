#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "console.h"
#include "rootfs.h"

static struct rootfs_entry entries[] = {
    { "/etc/motd", "Welcome to Z-Kernel!\n", 0 },
    { "/drivers/intel.txt", "Intel microcode placeholder: load APIC + xAPIC paths.\n", 0 },
    { "/drivers/amd.txt", "AMD microcode placeholder: prefer CCX-stable timers.\n", 0 },
    { "/init", "#!/bin/sh\necho Bootstrapping tiny rootfs...\n", 0 },
};

void rootfs_init(void) {
    const size_t count = sizeof(entries) / sizeof(entries[0]);
    for (size_t i = 0; i < count; i++) {
        entries[i].size = strlen(entries[i].data);
    }
}

const struct rootfs_entry *rootfs_entries(size_t *count) {
    if (count) {
        *count = sizeof(entries) / sizeof(entries[0]);
    }
    return entries;
}

static const struct rootfs_entry *rootfs_find_entry(const char *path) {
    if (!path) {
        return NULL;
    }

    size_t count = 0;
    const struct rootfs_entry *list = rootfs_entries(&count);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(list[i].path, path) == 0) {
            return &list[i];
        }
    }
    return NULL;
}

bool rootfs_read(const char *path, const char **data, size_t *size) {
    const struct rootfs_entry *entry = rootfs_find_entry(path);
    if (!entry) {
        return false;
    }
    if (data) {
        *data = entry->data;
    }
    if (size) {
        *size = entry->size;
    }
    return true;
}

void rootfs_log(void) {
    size_t count = 0;
    const struct rootfs_entry *list = rootfs_entries(&count);
    kprint("rootfs entries (%x):\n", (uint64_t)count);
    for (size_t i = 0; i < count; i++) {
        kprint(" - %s (%x bytes)\n", list[i].path, (uint64_t)list[i].size);
    }
}
