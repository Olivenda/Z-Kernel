#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

struct cpu_info {
    char vendor[13];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    bool is_intel;
    bool is_amd;
    bool sse;
    bool sse2;
    bool sse3;
    bool avx;
    bool avx2;
};

void cpu_detect(struct cpu_info *info);
void cpu_log(const struct cpu_info *info);

#endif /* CPU_H */
