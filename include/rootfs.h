#ifndef ROOTFS_H
#define ROOTFS_H

#include <stddef.h>
#include <stdbool.h>

struct rootfs_entry {
    const char *path;
    const char *data;
    size_t size;
};

void rootfs_init(void);
const struct rootfs_entry *rootfs_entries(size_t *count);
bool rootfs_read(const char *path, const char **data, size_t *size);
void rootfs_log(void);

#endif /* ROOTFS_H */
