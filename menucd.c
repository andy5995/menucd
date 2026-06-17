// SPDX-License-Identifier: Unlicense

#include "config.h"
#include "core.h"

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined HAVE_NCURSESW_MENU_H
#include <ncursesw/menu.h>
#elif defined HAVE_NCURSES_MENU_H
#include <ncurses/menu.h>
#elif defined HAVE_MENU_H
#include <menu.h>
#else
#error "SysV-compatible curses menu header required"
#endif

#if defined HAVE_NCURSESW_CURSES_H
#include <ncursesw/curses.h>
#elif defined HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#elif defined HAVE_NCURSES_H
#include <ncurses.h>
#elif defined HAVE_CURSES_H
#include <curses.h>
#else
#error "curses header required"
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

enum {
  PAIR_BAR = 1,
  PAIR_HELP,
};

typedef enum {
  E_PARENT,
  E_BOOKMARK,
  E_DIR,
  E_INFO,
} etype;

typedef struct {
  etype type;
  char *display; // shown in the menu
  char *path;    // chdir target, or NULL
  int bm_index;  // index into the bookmark array, for E_BOOKMARK
} entry;

static void init_colors(void) {
  if (!has_colors())
    return;

  start_color();
  use_default_colors();
  init_pair(PAIR_BAR, COLOR_WHITE, COLOR_BLUE);
  init_pair(PAIR_HELP, COLOR_YELLOW, COLOR_BLACK);
}

static void end_curses(void) { endwin(); }

// Modal key-binding help; any key dismisses it.
static void show_help(void) {
  static const char *lines[] = {
      "menucd key bindings",
      "",
      "  Up/Down, PgUp/PgDn   move the selection",
      "  Enter                cd into the selected item",
      "  s                    save current dir as a bookmark",
      "  d                    delete the bookmark under cursor",
      "  /                    search / filter as you type",
      "  q                    quit to the selected directory",
      "  x / ESC              cancel (no directory change)",
      "  ?                    this help",
      "",
      "  press any key to return",
  };
  int n = (int)(sizeof lines / sizeof lines[0]);
  int h = n + 2;
  int w = 60;
  int y = (LINES - h) / 2;
  int x = (COLS - w) / 2;
  if (y < 0)
    y = 0;
  if (x < 0)
    x = 0;

  WINDOW *win = newwin(h, w, y, x);
  if (win == NULL)
    return;
  box(win, 0, 0);
  for (int i = 0; i < n; i++)
    mvwprintw(win, i + 1, 2, "%s", lines[i]);
  wrefresh(win);
  wgetch(win);
  delwin(win);
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("menucd %s\n", VERSION);
    return 0;
  }

  // An optional first argument is the directory to start in.
  if (argc > 1 && chdir(argv[1]) != 0) {
    fprintf(stderr, "menucd: %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  set_escdelay(25);
  atexit(end_curses);
  init_colors();
  int color = has_colors();

  char *sf = save_file_path();
  if (sf != NULL)
    migrate_old_save(sf);

  char query[128] = "";
  int search_mode = 0;

  int running = 1;
  while (running) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof cwd) == NULL)
      strcpy(cwd, "?");
    int have_parent = (strcmp(cwd, "/") != 0);

    size_t n_dir = 0;
    char **dirs = read_dirs(&n_dir);
    if (dirs == NULL) {
      fprintf(stderr, "menucd: cannot read current directory\n");
      free(sf);
      return 1;
    }

    size_t n_bm = 0;
    char **bm = load_bookmarks(sf, &n_bm);

    size_t cap_entries = (size_t)have_parent + n_bm + n_dir;
    if (cap_entries == 0)
      cap_entries = 1; // room for the placeholder
    entry *entries = xcalloc(cap_entries, sizeof *entries);

    // ".." is always shown; bookmarks and subdirs are filtered by the
    // active search query (a no-op when query is empty).
    size_t k = 0;
    if (have_parent) {
      entries[k].type = E_PARENT;
      entries[k].display = xstrdup("..");
      entries[k].path = xstrdup("..");
      k++;
    }
    for (size_t i = 0; i < n_bm; i++) {
      if (!ci_contains(bm[i], query))
        continue;
      entries[k].type = E_BOOKMARK;
      entries[k].display = xmalloc(strlen(bm[i]) + 3);
      sprintf(entries[k].display, "* %s", bm[i]);
      entries[k].path = xstrdup(bm[i]);
      entries[k].bm_index = (int)i;
      k++;
    }
    for (size_t i = 0; i < n_dir; i++) {
      if (!ci_contains(dirs[i], query))
        continue;
      entries[k].type = E_DIR;
      entries[k].display = xstrdup(dirs[i]);
      entries[k].path = xstrdup(dirs[i]);
      k++;
    }
    if (k == 0) {
      entries[0].type = E_INFO;
      entries[0].display = xstrdup(query[0] ? "(no matches)" : "(no subdirectories or bookmarks)");
      entries[0].path = NULL;
      k = 1;
    }

    ITEM **items = xcalloc(k + 1, sizeof *items);
    for (size_t i = 0; i < k; i++) {
      items[i] = new_item(entries[i].display, "");
      set_item_userptr(items[i], &entries[i]);
    }
    items[k] = NULL;

    MENU *menu = new_menu(items);

    int top = 2;
    int sub_h = LINES - top - 1;
    if (sub_h < 1)
      sub_h = 1;

    set_menu_format(menu, sub_h, 1);
    set_menu_mark(menu, " > ");
    set_menu_fore(menu, A_REVERSE | A_BOLD);
    set_menu_back(menu, A_BOLD);

    WINDOW *sub = derwin(stdscr, sub_h, COLS, top, 0);
    set_menu_win(menu, stdscr);
    set_menu_sub(menu, sub);

    erase();
    if (color)
      attron(COLOR_PAIR(PAIR_BAR) | A_BOLD);
    mvhline(0, 0, ' ', COLS);
    mvprintw(0, 0, " menucd %s", VERSION);
    if (color)
      attroff(COLOR_PAIR(PAIR_BAR) | A_BOLD);

    attron(A_BOLD);
    mvprintw(1, 0, "PWD: %s", cwd);
    attroff(A_BOLD);

    if (color)
      attron(COLOR_PAIR(PAIR_HELP) | A_BOLD);
    else
      attron(A_REVERSE | A_BOLD);
    mvhline(LINES - 1, 0, ' ', COLS);
    if (search_mode)
      mvprintw(LINES - 1, 0, " Search: %s  (Enter accept, ESC clear)", query);
    else if (query[0])
      mvprintw(LINES - 1, 0, " filter: %s | / edit | Enter cd | s save | d del | q quit | x cancel",
               query);
    else
      mvprintw(LINES - 1, 0, " Enter cd | / search | s save | d del | q quit | x cancel | ? help");
    if (color)
      attroff(COLOR_PAIR(PAIR_HELP) | A_BOLD);
    else
      attroff(A_REVERSE | A_BOLD);

    post_menu(menu);
    refresh();
    wrefresh(sub);

    int rebuild = 0;
    while (running && !rebuild) {
      int c = getch();
      if (search_mode) {
        switch (c) {
        case KEY_DOWN:
          menu_driver(menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(menu, REQ_UP_ITEM);
          break;
        case KEY_NPAGE:
          menu_driver(menu, REQ_SCR_DPAGE);
          break;
        case KEY_PPAGE:
          menu_driver(menu, REQ_SCR_UPAGE);
          break;
        case '\n': // accept the filter, return to normal mode
          search_mode = 0;
          rebuild = 1;
          break;
        case 27: // ESC: clear the filter
          query[0] = '\0';
          search_mode = 0;
          rebuild = 1;
          break;
        case KEY_BACKSPACE:
        case 127:
        case 8: {
          size_t ql = strlen(query);
          if (ql > 0) {
            query[ql - 1] = '\0';
            rebuild = 1;
          }
          break;
        }
        default:
          if (c >= 32 && c < 127) {
            size_t ql = strlen(query);
            if (ql + 1 < sizeof query) {
              query[ql] = (char)c;
              query[ql + 1] = '\0';
              rebuild = 1;
            }
          }
          break;
        }
      } else {
        switch (c) {
        case KEY_DOWN:
          menu_driver(menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(menu, REQ_UP_ITEM);
          break;
        case KEY_NPAGE:
          menu_driver(menu, REQ_SCR_DPAGE);
          break;
        case KEY_PPAGE:
          menu_driver(menu, REQ_SCR_UPAGE);
          break;
        case KEY_BACKSPACE: // go up, like selecting ".."
        case 127:
        case 8:
          if (have_parent) {
            if (chdir("..") != 0)
              beep();
            else
              rebuild = 1;
          }
          break;
        case '/':
          search_mode = 1;
          rebuild = 1;
          break;
        case '?':
          show_help();
          rebuild = 1;
          break;
        case '\n': {
          ITEM *cur = current_item(menu);
          entry *en = cur ? item_userptr(cur) : NULL;
          if (en != NULL && en->path != NULL) {
            if (chdir(en->path) != 0)
              beep();
            else
              rebuild = 1;
          }
          break;
        }
        case 's':
          save_bookmark(sf, cwd);
          rebuild = 1;
          break;
        case 'd': {
          ITEM *cur = current_item(menu);
          entry *en = cur ? item_userptr(cur) : NULL;
          if (en != NULL && en->type == E_BOOKMARK) {
            if (color)
              attron(COLOR_PAIR(PAIR_HELP) | A_BOLD);
            else
              attron(A_REVERSE | A_BOLD);
            mvhline(LINES - 1, 0, ' ', COLS);
            mvprintw(LINES - 1, 0, " Delete bookmark %s ?  (y/n)", en->path);
            if (color)
              attroff(COLOR_PAIR(PAIR_HELP) | A_BOLD);
            else
              attroff(A_REVERSE | A_BOLD);
            refresh();

            int a = getch();
            if (a == 'y' || a == 'Y') {
              free(bm[en->bm_index]);
              for (size_t j = (size_t)en->bm_index + 1; j < n_bm; j++)
                bm[j - 1] = bm[j];
              n_bm--;
              rewrite_bookmarks(sf, bm, n_bm);
            }
            rebuild = 1;
          }
          break;
        }
        case 'q':
          quit_to_pwd();
          running = 0;
          break;
        case 27: // ESC
        case 'x':
          running = 0;
          break;
        }
      }
      if (running && !rebuild)
        wrefresh(sub);
    }

    unpost_menu(menu);
    free_menu(menu);
    delwin(sub);
    for (size_t i = 0; i < k; i++)
      free_item(items[i]);
    free(items);
    for (size_t i = 0; i < k; i++) {
      free(entries[i].display);
      free(entries[i].path);
    }
    free(entries);
    for (size_t i = 0; i < n_bm; i++)
      free(bm[i]);
    free(bm);
    for (size_t i = 0; i < n_dir; i++)
      free(dirs[i]);
    free(dirs);
  }

  endwin();
  free(sf);
  return 0;
}
