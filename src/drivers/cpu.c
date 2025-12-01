#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "console.h"
#include "cpu.h"

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ volatile ("cpuid" : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d) : "a" (leaf), "c" (subleaf));
}

static void detect_vendor(struct cpu_info *info) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);

    info->vendor[0] = (char)(ebx & 0xFF);
    info->vendor[1] = (char)((ebx >> 8) & 0xFF);
    info->vendor[2] = (char)((ebx >> 16) & 0xFF);
    info->vendor[3] = (char)((ebx >> 24) & 0xFF);
    info->vendor[4] = (char)(edx & 0xFF);
    info->vendor[5] = (char)((edx >> 8) & 0xFF);
    info->vendor[6] = (char)((edx >> 16) & 0xFF);
    info->vendor[7] = (char)((edx >> 24) & 0xFF);
    info->vendor[8] = (char)(ecx & 0xFF);
    info->vendor[9] = (char)((ecx >> 8) & 0xFF);
    info->vendor[10] = (char)((ecx >> 16) & 0xFF);
    info->vendor[11] = (char)((ecx >> 24) & 0xFF);
    info->vendor[12] = '\0';

    info->is_intel = (strncmp(info->vendor, "GenuineIntel", 12) == 0);
    info->is_amd = (strncmp(info->vendor, "AuthenticAMD", 12) == 0);
}

static void detect_basic_features(struct cpu_info *info) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    uint32_t family_id = (eax >> 8) & 0xF;
    uint32_t model_id = (eax >> 4) & 0xF;
    uint32_t stepping_id = eax & 0xF;

    uint32_t extended_family = (eax >> 20) & 0xFF;
    uint32_t extended_model = (eax >> 16) & 0xF;

    if (family_id == 0xF) {
        family_id += extended_family;
    }
    if (family_id == 0x6 || family_id == 0xF) {
        model_id |= (extended_model << 4);
    }

    info->family = family_id;
    info->model = model_id;
    info->stepping = stepping_id;

    info->sse = (edx >> 25) & 0x1;
    info->sse2 = (edx >> 26) & 0x1;
    info->sse3 = (ecx >> 0) & 0x1;
    info->avx = (ecx >> 28) & 0x1;
    info->avx2 = false;
}

static void detect_ext_features(struct cpu_info *info) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 7) {
        return;
    }

    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    info->avx2 = (ebx >> 5) & 0x1;
}

static void log_driver_notes(const struct cpu_info *info) {
    if (info->is_intel) {
        kprint("Intel driver hints: APIC + xAPIC ready.\n");
        if (info->avx2) {
            kprint(" - AVX2 present, vector paths enabled.\n");
        } else if (info->avx) {
            kprint(" - AVX present, falling back to AVX1 paths.\n");
        } else {
            kprint(" - Legacy SIMD only, using SSE fast paths.\n");
        }
    } else if (info->is_amd) {
        kprint("AMD driver hints: enable CCX-friendly timers.\n");
        if (info->avx2) {
            kprint(" - Zen-class core detected, wide vectors available.\n");
        } else {
            kprint(" - Older core, keep 128-bit aligned code paths.\n");
        }
    } else {
        kprint("Generic x86_64 CPU detected.\n");
    }
}

void cpu_detect(struct cpu_info *info) {
    if (!info) {
        return;
    }
    memset(info, 0, sizeof(*info));
    detect_vendor(info);
    detect_basic_features(info);
    detect_ext_features(info);
}

void cpu_log(const struct cpu_info *info) {
    if (!info) {
        return;
    }

    kprint("CPU vendor: %s\n", info->vendor);
    kprint("Family %x Model %x Stepping %x\n", (uint64_t)info->family, (uint64_t)info->model, (uint64_t)info->stepping);
    kprint("Features: SSE=%x SSE2=%x SSE3=%x AVX=%x AVX2=%x\n", (uint64_t)info->sse, (uint64_t)info->sse2, (uint64_t)info->sse3, (uint64_t)info->avx, (uint64_t)info->avx2);
    log_driver_notes(info);
}
