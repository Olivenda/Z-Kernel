#include "kconfig_common.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *env_or_default(const char *name, const char *def) {
    const char *v = getenv(name);
    return v ? v : def;
}

static void draw_help(int rows) {
    mvprintw(rows-3, 0, "Space: toggle/select  Enter: edit string  S: save  Q: quit");
}

static void render_menu(kconfig_t *kc, int highlight, int rows) {
    clear();
    int idx = 0;
    for (int i = 0; i < kc->options.count; ++i) {
        option_t *opt = kc->options.items[i];
        if (opt->type == OPT_CHOICE) {
            option_t *active = NULL;
            for (int j = 0; j < opt->children.count; ++j) {
                if (opt->children.items[j]->bool_val) {
                    active = opt->children.items[j];
                    break;
                }
            }
            if (idx == highlight) attron(A_REVERSE);
            mvprintw(idx, 0, "(choice) %s: %s", opt->prompt ? opt->prompt : "Choice", active ? active->prompt : "<none>");
            if (idx == highlight) attroff(A_REVERSE);
            idx++;
        } else {
            const char *prefix = "[ ]";
            char value_buf[64] = {0};
            if (opt->type == OPT_BOOL)
                prefix = opt->bool_val ? "[*]" : "[ ]";
            if (opt->type == OPT_STRING)
                snprintf(value_buf, sizeof(value_buf), " = %s", opt->str_val ? opt->str_val : "");
            if (idx == highlight) attron(A_REVERSE);
            mvprintw(idx, 0, "%s %s%s", prefix, opt->prompt ? opt->prompt : opt->name, value_buf);
            if (idx == highlight) attroff(A_REVERSE);
            idx++;
        }
    }
    draw_help(rows);
    refresh();
}

static int option_count(const kconfig_t *kc) {
    return kc->options.count;
}

static option_t *option_at(kconfig_t *kc, int idx) {
    if (idx < 0 || idx >= kc->options.count) return NULL;
    return kc->options.items[idx];
}

static void cycle_choice(option_t *choice) {
    if (!choice || choice->type != OPT_CHOICE || choice->children.count == 0) return;
    int current = 0;
    for (int i = 0; i < choice->children.count; ++i) {
        if (choice->children.items[i]->bool_val) {
            current = i;
            break;
        }
    }
    int next = (current + 1) % choice->children.count;
    for (int i = 0; i < choice->children.count; ++i)
        choice->children.items[i]->bool_val = false;
    choice->children.items[next]->bool_val = true;
}

static void edit_string(option_t *opt) {
    echo();
    curs_set(1);
    char buf[256];
    mvprintw(LINES-2, 0, "Enter value for %s: ", opt->prompt ? opt->prompt : opt->name);
    getnstr(buf, sizeof(buf)-1);
    noecho();
    curs_set(0);
    free(opt->str_val);
    opt->str_val = strdup(buf);
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

    apply_defaults(&kc);
    load_config_values(&kc, config_path);

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    int highlight = 0;
    int running = 1;
    while (running) {
        render_menu(&kc, highlight, LINES);
        int c = getch();
        switch (c) {
        case KEY_UP:
            if (highlight > 0) highlight--;
            break;
        case KEY_DOWN:
            if (highlight < option_count(&kc) - 1) highlight++;
            break;
        case 'q':
        case 'Q':
            running = 0;
            break;
        case 's':
        case 'S':
            save_config(&kc, config_path, auto_conf_path, auto_header_path, config_mk_path);
            mvprintw(LINES-4, 0, "Saved to %s", config_path);
            refresh();
            break;
        case ' ':
        case KEY_RIGHT: {
            option_t *opt = option_at(&kc, highlight);
            if (!opt) break;
            if (opt->type == OPT_BOOL)
                opt->bool_val = !opt->bool_val;
            else if (opt->type == OPT_CHOICE)
                cycle_choice(opt);
            break;
        }
        case '\n': {
            option_t *opt = option_at(&kc, highlight);
            if (opt && opt->type == OPT_STRING)
                edit_string(opt);
            else if (opt && opt->type == OPT_CHOICE)
                cycle_choice(opt);
            break;
        }
        default:
            break;
        }
    }

    endwin();

    save_config(&kc, config_path, auto_conf_path, auto_header_path, config_mk_path);
    free_kconfig(&kc);
    printf("Configuration saved to %s\n", config_path);
    return 0;
}
