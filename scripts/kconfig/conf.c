#include "kconfig_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *env_or_default(const char *name, const char *def) {
    const char *v = getenv(name);
    return v ? v : def;
}

int main(int argc, char **argv) {
    const char *kconfig_file = argc > 1 ? argv[argc-1] : "Kconfig";
    const char *config_path = env_or_default("KCONFIG_CONFIG", ".config");
    const char *auto_conf_path = env_or_default("KCONFIG_AUTOCONFIG", "include/config/auto.conf");
    const char *auto_header_path = env_or_default("KCONFIG_AUTOHEADER", "include/generated/autoconf.h");
    const char *config_mk_path = env_or_default("KCONFIG_MK", "config.mk");

    kconfig_t kc;
    char err[128];
    if (parse_kconfig(kconfig_file, &kc, err, sizeof(err)) != 0) {
        fprintf(stderr, "Kconfig parse error: %s\n", err);
        return 1;
    }

    int defconfig = 0;
    int syncconfig = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--defconfig") == 0)
            defconfig = 1;
        if (strcmp(argv[i], "--syncconfig") == 0)
            syncconfig = 1;
    }

    apply_defaults(&kc);
    if (!defconfig)
        load_config_values(&kc, config_path);

    if (save_config(&kc, config_path, auto_conf_path, auto_header_path, config_mk_path) != 0) {
        fprintf(stderr, "Failed to write configuration outputs\n");
        free_kconfig(&kc);
        return 1;
    }

    if (syncconfig)
        printf("Configuration synchronized to %s\n", config_path);
    else if (defconfig)
        printf("Default configuration written to %s\n", config_path);

    free_kconfig(&kc);
    return 0;
}
