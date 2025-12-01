#ifndef KCONFIG_COMMON_H
#define KCONFIG_COMMON_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OPT_BOOL,
    OPT_STRING,
    OPT_CHOICE,
    OPT_CHOICE_OPT,
} option_type;

typedef struct option option_t;

typedef struct {
    option_t **items;
    int count;
} option_list;

struct option {
    option_type type;
    char *name;
    char *prompt;
    char *help;
    char *def;
    bool bool_val;
    char *str_val;
    option_t *parent;      /* for choice options */
    option_list children;  /* for choice parent */
};

typedef struct {
    option_list options;
    char *default_choice; /* name of default option for the current choice during parse */
} kconfig_t;

int parse_kconfig(const char *path, kconfig_t *kc, char *err_buf, size_t err_len);
void free_kconfig(kconfig_t *kc);
void apply_defaults(kconfig_t *kc);
void load_config_values(kconfig_t *kc, const char *config_path);
int save_config(const kconfig_t *kc, const char *config_path, const char *auto_conf_path, const char *auto_header_path, const char *config_mk_path);

#endif
