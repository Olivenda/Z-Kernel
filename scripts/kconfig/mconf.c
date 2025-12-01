#include "kconfig_common.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_ROWS 3
#define FOOTER_ROWS 2
#define HELP_ROWS   6

static bool colors_ready = false;

static const char *env_or_default(const char *name, const char *def) {
    const char *v = getenv(name);
    return v ? v : def;
}

static void init_colors(void) {
    if (!has_colors())
        return;

    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);    /* headers */
    init_pair(2, COLOR_BLACK, COLOR_CYAN); /* highlighted row */
    init_pair(3, COLOR_GREEN, -1);   /* enabled */
    init_pair(4, COLOR_RED, -1);     /* disabled */
    init_pair(5, COLOR_YELLOW, -1);  /* hints/help */
    colors_ready = true;
}

static void push_color(short pair) {
    if (colors_ready)
        attron(COLOR_PAIR(pair));
}

static void pop_color(short pair) {
    if (colors_ready)
        attroff(COLOR_PAIR(pair));
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

static void edit_string(option_t *opt, int rows) {
    echo();
    curs_set(1);
    char buf[256];
    move(rows - FOOTER_ROWS - 1, 0);
    clrtoeol();
    mvprintw(rows - FOOTER_ROWS - 1, 0, "Enter value for %s: ", opt->prompt ? opt->prompt : opt->name);
    getnstr(buf, sizeof(buf)-1);
    noecho();
    curs_set(0);
    free(opt->str_val);
    opt->str_val = strdup(buf);
}

static void draw_header(const char *config_path, int cols) {
    push_color(1);
    attron(A_BOLD);
    mvhline(0, 0, ' ', cols);
    const char *title = "Z-Kernel Configuration";
    int start = (cols - (int)strlen(title)) / 2;
    if (start < 0) start = 0;
    mvprintw(0, start, "%s", title);
    attroff(A_BOLD);
    mvhline(1, 0, ' ', cols);
    mvprintw(1, 2, "Config file: %s", config_path);
    mvhline(2, 0, ACS_HLINE, cols);
    pop_color(1);
}

static void draw_footer(int rows, int cols) {
    int y = rows - FOOTER_ROWS;
    mvhline(y, 0, ACS_HLINE, cols);
    push_color(5);
    attron(A_BOLD);
    mvprintw(y + 1, 2, "Arrows: navigate   Space/Enter: toggle or edit   S: save   Q: quit");
    attroff(A_BOLD);
    pop_color(5);
}

static void draw_help(option_t *opt, int rows, int cols) {
    int start = rows - FOOTER_ROWS - HELP_ROWS;
    mvhline(start, 0, ACS_HLINE, cols);
    push_color(5);
    mvprintw(start + 1, 2, "Help for %s", opt && opt->prompt ? opt->prompt : "(no selection)");
    pop_color(5);
    for (int i = 2; i < HELP_ROWS; ++i) {
        move(start + i, 0);
        clrtoeol();
    }
    if (!opt || !opt->help)
        return;

    const char *text = opt->help;
    int y = start + 2;
    int x = 2;
    int width = cols - 4;
    const char *p = text;
    while (*p && y < rows - FOOTER_ROWS - 1) {
        while (*p == '\n') {
            y++;
            x = 2;
            p++;
            if (y >= rows - FOOTER_ROWS - 1)
                break;
        }
        if (y >= rows - FOOTER_ROWS - 1)
            break;
        const char *line_start = p;
        int line_len = 0;
        while (p[line_len] && p[line_len] != '\n' && line_len < width)
            line_len++;
        mvprintw(y, x, "%.*s", line_len, line_start);
        p += line_len;
        if (p[0] && p[0] != '\n') {
            while (*p && *p != '\n') p++; /* skip the rest */
        }
        if (*p == '\n') p++;
        y++;
    }
}

static void format_option_label(option_t *opt, char *buf, size_t len) {
    if (opt->type == OPT_CHOICE) {
        option_t *active = NULL;
        for (int j = 0; j < opt->children.count; ++j) {
            if (opt->children.items[j]->bool_val) {
                active = opt->children.items[j];
                break;
            }
        }
        snprintf(buf, len, "<Select> %s (%s)", opt->prompt ? opt->prompt : "Choice", active && active->prompt ? active->prompt : "none");
        return;
    }

    if (opt->type == OPT_BOOL || opt->type == OPT_CHOICE_OPT) {
        snprintf(buf, len, "[%c] %s", opt->bool_val ? 'X' : ' ', opt->prompt ? opt->prompt : opt->name);
        return;
    }

    if (opt->type == OPT_STRING) {
        snprintf(buf, len, "(str) %s = \"%s\"", opt->prompt ? opt->prompt : opt->name, opt->str_val ? opt->str_val : "");
        return;
    }

    snprintf(buf, len, "%s", opt->prompt ? opt->prompt : opt->name);
}

static void render_menu(kconfig_t *kc, int highlight, int scroll, int rows, int cols, const char *config_path) {
    clear();
    draw_header(config_path, cols);
    draw_footer(rows, cols);

    int list_start = HEADER_ROWS;
    int list_height = rows - HEADER_ROWS - FOOTER_ROWS - HELP_ROWS;
    if (list_height < 3)
        list_height = 3;

    for (int i = 0; i < list_height; ++i) {
        int idx = scroll + i;
        move(list_start + i, 0);
        clrtoeol();
        if (idx >= kc->options.count)
            continue;
        option_t *opt = kc->options.items[idx];
        char label[256];
        format_option_label(opt, label, sizeof(label));
        bool selected = idx == highlight;
        if (selected) {
            push_color(2);
            attron(A_BOLD);
            mvprintw(list_start + i, 0, " > %-*.*s", cols - 3, cols - 3, label);
            attroff(A_BOLD);
            pop_color(2);
        } else {
            short pair = 0;
            if (opt->type == OPT_BOOL || opt->type == OPT_CHOICE_OPT)
                pair = opt->bool_val ? 3 : 4;
            if (pair) push_color(pair);
            mvprintw(list_start + i, 2, "%-*.*s", cols - 2, cols - 2, label);
            if (pair) pop_color(pair);
        }
    }

    option_t *opt = option_at(kc, highlight);
    draw_help(opt, rows, cols);

    refresh();
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
    init_colors();

    int highlight = 0;
    int scroll = 0;
    int running = 1;
    while (running) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        int list_height = rows - HEADER_ROWS - FOOTER_ROWS - HELP_ROWS;
        if (list_height < 3)
            list_height = 3;
        if (highlight < scroll)
            scroll = highlight;
        else if (highlight >= scroll + list_height)
            scroll = highlight - list_height + 1;

        render_menu(&kc, highlight, scroll, rows, cols, config_path);
        int c = getch();
        switch (c) {
        case KEY_UP:
            if (highlight > 0) highlight--;
            break;
        case KEY_DOWN:
            if (highlight < option_count(&kc) - 1) highlight++;
            break;
        case KEY_PPAGE:
            highlight -= list_height;
            if (highlight < 0) highlight = 0;
            break;
        case KEY_NPAGE:
            highlight += list_height;
            if (highlight >= option_count(&kc)) highlight = option_count(&kc) - 1;
            break;
        case 'q':
        case 'Q':
            running = 0;
            break;
        case 's':
        case 'S':
            save_config(&kc, config_path, auto_conf_path, auto_header_path, config_mk_path);
            push_color(5);
            mvprintw(rows - FOOTER_ROWS - 1, 2, "Saved to %s", config_path);
            pop_color(5);
            refresh();
            break;
        case ' ':
        case KEY_RIGHT:
        case '\n': {
            option_t *opt = option_at(&kc, highlight);
            if (!opt) break;
            if (opt->type == OPT_BOOL)
                opt->bool_val = !opt->bool_val;
            else if (opt->type == OPT_CHOICE)
                cycle_choice(opt);
            else if (opt->type == OPT_STRING && c == '\n')
                edit_string(opt, rows);
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
