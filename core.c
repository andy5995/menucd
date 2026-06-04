// SPDX-License-Identifier: Unlicense

#define _GNU_SOURCE

#include "core.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// No endwin() here: the curses front-end registers atexit(endwin), so the
// terminal is restored on this exit() without coupling core to curses.
static void fatal_oom(const char *func, int line) {
  int err = errno;
  fprintf(stderr, "menucd: out of memory at %s:%d: %s\n", func, line, strerror(err));
  exit(EXIT_FAILURE);
}

void *xmalloc_at(size_t size, const char *func, int line) {
  void *p = malloc(size);
  if (p == NULL)
    fatal_oom(func, line);
  return p;
}

void *xcalloc_at(size_t n, size_t size, const char *func, int line) {
  void *p = calloc(n, size);
  if (p == NULL)
    fatal_oom(func, line);
  return p;
}

void *xrealloc_at(void *ptr, size_t size, const char *func, int line) {
  void *p = realloc(ptr, size);
  if (p == NULL)
    fatal_oom(func, line);
  return p;
}

char *xstrdup_at(const char *s, const char *func, int line) {
  char *p = strdup(s);
  if (p == NULL)
    fatal_oom(func, line);
  return p;
}

int ci_contains(const char *hay, const char *needle) {
  if (needle[0] == '\0')
    return 1;
  size_t nl = strlen(needle);
  for (const char *p = hay; *p != '\0'; p++)
    if (strncasecmp(p, needle, nl) == 0)
      return 1;
  return 0;
}

static int cmp_name(const void *a, const void *b) {
  return strcmp(*(const char *const *)a, *(const char *const *)b);
}

char **read_dirs(size_t *n_out) {
  DIR *d = opendir(".");
  if (d == NULL)
    return NULL;

  size_t cap = 16;
  size_t n = 0;
  char **names = xmalloc(cap * sizeof *names);

  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
      continue;

    struct stat st;
    if (stat(e->d_name, &st) != 0 || !S_ISDIR(st.st_mode))
      continue;

    if (n == cap) {
      cap *= 2;
      names = xrealloc(names, cap * sizeof *names);
    }
    names[n++] = xstrdup(e->d_name);
  }
  closedir(d);

  qsort(names, n, sizeof *names, cmp_name);
  *n_out = n;
  return names;
}

int mkdir_p(const char *path) {
  char *tmp = xstrdup(path);
  for (char *p = tmp + 1; *p != '\0'; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
      }
      *p = '/';
    }
  }
  int rc = (mkdir(tmp, 0700) != 0 && errno != EEXIST) ? -1 : 0;
  free(tmp);
  return rc;
}

char *data_dir(void) {
  const char *xdg = getenv("XDG_DATA_HOME");
  char *base;
  if (xdg != NULL && xdg[0] == '/') {
    base = xstrdup(xdg);
  } else {
    const char *home = getenv("HOME");
    if (home == NULL)
      return NULL;
    base = xmalloc(strlen(home) + strlen("/.local/share") + 1);
    sprintf(base, "%s/.local/share", home);
  }

  char *dir = xmalloc(strlen(base) + strlen("/menucd") + 1);
  sprintf(dir, "%s/menucd", base);
  free(base);
  return dir;
}

char *save_file_path(void) {
  char *dir = data_dir();
  if (dir == NULL)
    return NULL;

  mkdir_p(dir);
  char *p = xmalloc(strlen(dir) + strlen("/bookmarks") + 1);
  sprintf(p, "%s/bookmarks", dir);
  free(dir);
  return p;
}

void migrate_old_save(const char *new_path) {
  const char *home = getenv("HOME");
  if (home == NULL)
    return;

  char *old = xmalloc(strlen(home) + strlen("/.menucd-save") + 1);
  sprintf(old, "%s/.menucd-save", home);

  struct stat st;
  if (stat(new_path, &st) != 0 && stat(old, &st) == 0)
    rename(old, new_path);

  free(old);
}

char **load_bookmarks(const char *path, size_t *n_out) {
  *n_out = 0;
  if (path == NULL)
    return xcalloc(1, sizeof(char *));

  FILE *f = fopen(path, "r");
  if (f == NULL)
    return xcalloc(1, sizeof(char *));

  size_t cap = 8;
  size_t n = 0;
  char **a = xmalloc(cap * sizeof *a);
  char *line = NULL;
  size_t len = 0;
  ssize_t r;
  while ((r = getline(&line, &len, f)) != -1) {
    if (r > 0 && line[r - 1] == '\n')
      line[--r] = '\0';
    if (r == 0)
      continue;

    if (n == cap) {
      cap *= 2;
      a = xrealloc(a, cap * sizeof *a);
    }
    a[n++] = xstrdup(line);
  }
  free(line);
  fclose(f);

  *n_out = n;
  return a;
}

void save_bookmark(const char *path, const char *dir) {
  if (path == NULL)
    return;

  FILE *f = fopen(path, "a");
  if (f == NULL)
    return;

  fprintf(f, "%s\n", dir);
  fclose(f);
}

void rewrite_bookmarks(const char *path, char **bm, size_t n) {
  if (path == NULL)
    return;

  FILE *f = fopen(path, "w");
  if (f == NULL)
    return;

  for (size_t i = 0; i < n; i++)
    fprintf(f, "%s\n", bm[i]);
  fclose(f);
}

void cd_exit_path(char *buf, size_t n) {
  const char *rt = getenv("XDG_RUNTIME_DIR");
  if (rt != NULL && rt[0] == '/')
    snprintf(buf, n, "%s/menucd.cd.exit", rt);
  else
    snprintf(buf, n, "/tmp/menucd.cd.exit");
}

void quit_to_pwd(void) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof cwd) == NULL)
    return;

  char path[PATH_MAX];
  cd_exit_path(path, sizeof path);

  FILE *f = fopen(path, "w");
  if (f == NULL)
    return;

  fprintf(f, "%s\n", cwd);
  fclose(f);
}
