#include <stddef.h>

size_t strlen(const char *str) {
    size_t len = 0;
    if (!str) {
        return 0;
    }
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *lhs, const char *rhs) {
    size_t i = 0;
    while (lhs[i] != '\0' && rhs[i] != '\0') {
        if (lhs[i] != rhs[i]) {
            return (unsigned char)lhs[i] - (unsigned char)rhs[i];
        }
        i++;
    }
    return (unsigned char)lhs[i] - (unsigned char)rhs[i];
}

int strncmp(const char *lhs, const char *rhs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (lhs[i] != rhs[i]) {
            return (unsigned char)lhs[i] - (unsigned char)rhs[i];
        }
        if (lhs[i] == '\0') {
            return 0;
        }
    }
    return 0;
}
