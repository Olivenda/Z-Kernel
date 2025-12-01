#include "kconfig_common.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p) memcpy(p, s, len);
    return p;
}

static void append_option(option_list *list, option_t *opt) {
    option_t **new_items = realloc(list->items, sizeof(option_t*) * (list->count + 1));
    if (!new_items) return;
    list->items = new_items;
    list->items[list->count++] = opt;
}

static option_t *find_option(kconfig_t *kc, const char *name) {
    for (int i = 0; i < kc->options.count; ++i) {
        option_t *opt = kc->options.items[i];
        if (opt->name && strcmp(opt->name, name) == 0)
            return opt;
        if (opt->type == OPT_CHOICE) {
            for (int j = 0; j < opt->children.count; ++j) {
                option_t *child = opt->children.items[j];
                if (child->name && strcmp(child->name, name) == 0)
                    return child;
            }
        }
    }
    return NULL;
}

static void trim(char *line) {
    size_t len = strlen(line);
    while (len && (line[len-1] == '\n' || line[len-1] == '\r')) {
        line[--len] = '\0';
    }
    size_t start = 0;
    while (isspace((unsigned char)line[start])) start++;
    if (start) memmove(line, line + start, len - start + 1);
}

static bool starts_with(const char *s, const char *pref) {
    return strncmp(s, pref, strlen(pref)) == 0;
}

static char *dup_token(const char *start) {
    while (isspace((unsigned char)*start)) start++;
    const char *end = start;
    while (*end && !isspace((unsigned char)*end)) end++;
    size_t len = end - start;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char *dup_quoted(const char *start) {
    while (isspace((unsigned char)*start)) start++;
    if (*start == '"') start++;
    const char *end = start;
    while (*end && *end != '"') end++;
    size_t len = end - start;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

int parse_kconfig(const char *path, kconfig_t *kc, char *err_buf, size_t err_len) {
    memset(kc, 0, sizeof(*kc));
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(err_buf, err_len, "unable to open %s: %s", path, strerror(errno));
        return -1;
    }
    char line[512];
    option_t *current = NULL;
    option_t *current_choice = NULL;
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!line[0] || starts_with(line, "#"))
            continue;
        if (starts_with(line, "mainmenu") || starts_with(line, "menu"))
            continue;
        if (starts_with(line, "choice")) {
            current_choice = calloc(1, sizeof(option_t));
            current_choice->type = OPT_CHOICE;
            append_option(&kc->options, current_choice);
            current = NULL;
            continue;
        }
        if (starts_with(line, "endchoice")) {
            current_choice = NULL;
            current = NULL;
            continue;
        }
        if (starts_with(line, "config")) {
            char *name = dup_token(line + strlen("config"));
            option_t *opt = calloc(1, sizeof(option_t));
            opt->name = name;
            opt->type = OPT_BOOL; /* default until specified */
            if (current_choice) {
                opt->type = OPT_CHOICE_OPT;
                opt->parent = current_choice;
                append_option(&current_choice->children, opt);
            } else {
                append_option(&kc->options, opt);
            }
            current = opt;
            continue;
        }
        if (!current && !current_choice)
            continue;
        if (starts_with(line, "prompt")) {
            char *p = dup_quoted(line + strlen("prompt"));
            if (current_choice)
                current_choice->prompt = p;
            else if (current)
                current->prompt = p;
            continue;
        }
        if (starts_with(line, "bool")) {
            current->type = current->type == OPT_CHOICE_OPT ? OPT_CHOICE_OPT : OPT_BOOL;
            if (!current->prompt)
                current->prompt = dup_quoted(line + strlen("bool"));
            continue;
        }
        if (starts_with(line, "string")) {
            current->type = OPT_STRING;
            if (!current->prompt)
                current->prompt = dup_quoted(line + strlen("string"));
            continue;
        }
        if (starts_with(line, "default")) {
            if (current_choice && !current) {
                kc->default_choice = dup_token(line + strlen("default"));
            } else if (current) {
                if (current->type == OPT_STRING)
                    current->def = dup_quoted(line + strlen("default"));
                else
                    current->def = dup_token(line + strlen("default"));
            }
            continue;
        }
    }
    fclose(f);
    return 0;
}

void free_kconfig(kconfig_t *kc) {
    for (int i = 0; i < kc->options.count; ++i) {
        option_t *opt = kc->options.items[i];
        free(opt->name);
        free(opt->prompt);
        free(opt->help);
        free(opt->def);
        free(opt->str_val);
        for (int j = 0; j < opt->children.count; ++j)
            free(opt->children.items[j]);
        free(opt->children.items);
        free(opt);
    }
    free(kc->options.items);
    free(kc->default_choice);
}

void apply_defaults(kconfig_t *kc) {
    for (int i = 0; i < kc->options.count; ++i) {
        option_t *opt = kc->options.items[i];
        if (opt->type == OPT_BOOL)
            opt->bool_val = opt->def ? (strcmp(opt->def, "y") == 0 || strcmp(opt->def, "1") == 0) : false;
        else if (opt->type == OPT_STRING)
            opt->str_val = opt->def ? xstrdup(opt->def) : xstrdup("");
        else if (opt->type == OPT_CHOICE) {
            /* set selection based on default_choice */
            if (kc->default_choice) {
                for (int j = 0; j < opt->children.count; ++j) {
                    option_t *child = opt->children.items[j];
                    child->bool_val = (strcmp(child->name, kc->default_choice) == 0);
                }
            } else if (opt->children.count) {
                opt->children.items[0]->bool_val = true;
            }
        }
    }
}

static void ensure_dir(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
}

void load_config_values(kconfig_t *kc, const char *config_path) {
    FILE *f = fopen(config_path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (starts_with(line, "#") || !strstr(line, "CONFIG_"))
            continue;
        trim(line);
        if (!starts_with(line, "CONFIG_"))
            continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *name = line + strlen("CONFIG_");
        char *val = eq + 1;
        option_t *opt = find_option(kc, name);
        if (!opt) continue;
        if (opt->type == OPT_BOOL || opt->type == OPT_CHOICE_OPT) {
            bool enabled = (strcmp(val, "y") == 0 || strcmp(val, "1") == 0);
            opt->bool_val = enabled;
            if (opt->parent && enabled) {
                for (int i = 0; i < opt->parent->children.count; ++i)
                    opt->parent->children.items[i]->bool_val = false;
                opt->bool_val = true;
            }
        } else if (opt->type == OPT_STRING) {
            if (*val == '"') val++;
            size_t len = strlen(val);
            if (len && val[len-1] == '"') val[len-1] = '\0';
            free(opt->str_val);
            opt->str_val = xstrdup(val);
        }
    }
    fclose(f);
}

static void write_bool(FILE *f, const option_t *opt) {
    if (opt->bool_val)
        fprintf(f, "CONFIG_%s=y\n", opt->name);
    else
        fprintf(f, "# CONFIG_%s is not set\n", opt->name);
}

static void write_string(FILE *f, const option_t *opt) {
    fprintf(f, "CONFIG_%s=\"%s\"\n", opt->name, opt->str_val ? opt->str_val : "");
}

int save_config(const kconfig_t *kc, const char *config_path, const char *auto_conf_path, const char *auto_header_path, const char *config_mk_path) {
    ensure_dir(auto_conf_path);
    ensure_dir(auto_header_path);
    ensure_dir(config_mk_path);

    FILE *cfg = fopen(config_path, "w");
    FILE *auto_conf = fopen(auto_conf_path, "w");
    FILE *auto_hdr = fopen(auto_header_path, "w");
    FILE *cfg_mk = fopen(config_mk_path, "w");
    if (!cfg || !auto_conf || !auto_hdr || !cfg_mk) {
        if (cfg) fclose(cfg);
        if (auto_conf) fclose(auto_conf);
        if (auto_hdr) fclose(auto_hdr);
        if (cfg_mk) fclose(cfg_mk);
        return -1;
    }
    fprintf(auto_hdr, "/* Auto-generated configuration header */\n");
    fprintf(auto_hdr, "#pragma once\n\n");

    for (int i = 0; i < kc->options.count; ++i) {
        const option_t *opt = kc->options.items[i];
        if (opt->type == OPT_BOOL) {
            write_bool(cfg, opt);
            write_bool(auto_conf, opt);
            fprintf(cfg_mk, "CONFIG_%s=%d\n", opt->name, opt->bool_val ? 1 : 0);
            if (opt->bool_val)
                fprintf(auto_hdr, "#define CONFIG_%s 1\n", opt->name);
        } else if (opt->type == OPT_STRING) {
            write_string(cfg, opt);
            write_string(auto_conf, opt);
            fprintf(cfg_mk, "CONFIG_%s=\"%s\"\n", opt->name, opt->str_val ? opt->str_val : "");
            fprintf(auto_hdr, "#define CONFIG_%s \"%s\"\n", opt->name, opt->str_val ? opt->str_val : "");
        } else if (opt->type == OPT_CHOICE) {
            for (int j = 0; j < opt->children.count; ++j) {
                const option_t *child = opt->children.items[j];
                write_bool(cfg, child);
                write_bool(auto_conf, child);
                fprintf(cfg_mk, "CONFIG_%s=%d\n", child->name, child->bool_val ? 1 : 0);
                if (child->bool_val)
                    fprintf(auto_hdr, "#define CONFIG_%s 1\n", child->name);
            }
        }
    }

    fclose(cfg);
    fclose(auto_conf);
    fclose(auto_hdr);
    fclose(cfg_mk);
    return 0;
}
