// SPDX-License-Identifier: Unlicense

#define _GNU_SOURCE

#include "core.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void data_dir_xdg(void) {
  setenv("XDG_DATA_HOME", "/tmp/xdgtest", 1);
  char *d = data_dir();
  assert(d != NULL);
  assert(strcmp(d, "/tmp/xdgtest/menucd") == 0);
  free(d);
}

static void data_dir_home_fallback(void) {
  unsetenv("XDG_DATA_HOME");
  setenv("HOME", "/home/tester", 1);
  char *d = data_dir();
  assert(d != NULL);
  assert(strcmp(d, "/home/tester/.local/share/menucd") == 0);
  free(d);
}

static void data_dir_relative_xdg_ignored(void) {
  // XDG spec: a relative XDG_DATA_HOME must be ignored; fall back to HOME.
  setenv("XDG_DATA_HOME", "relative/path", 1);
  setenv("HOME", "/home/tester", 1);
  char *d = data_dir();
  assert(strcmp(d, "/home/tester/.local/share/menucd") == 0);
  free(d);
  unsetenv("XDG_DATA_HOME");
}

static void runtime_dir_path(void) {
  char buf[256];
  setenv("XDG_RUNTIME_DIR", "/run/u", 1);
  cd_exit_path(buf, sizeof buf);
  assert(strcmp(buf, "/run/u/menucd.cd.exit") == 0);

  unsetenv("XDG_RUNTIME_DIR");
  cd_exit_path(buf, sizeof buf);
  assert(strcmp(buf, "/tmp/menucd.cd.exit") == 0);
}

static void bookmark_roundtrip(void) {
  const char *p = "/tmp/menucd_unit_bm";
  unlink(p);
  save_bookmark(p, "/a");
  save_bookmark(p, "/b with space");

  size_t n = 0;
  char **bm = load_bookmarks(p, &n);
  assert(n == 2);
  assert(strcmp(bm[0], "/a") == 0);
  assert(strcmp(bm[1], "/b with space") == 0);

  for (size_t i = 0; i < n; i++)
    free(bm[i]);
  free(bm);
  unlink(p);
}

static void ci_match(void) {
  assert(ci_contains("Documents", "doc")); // case-insensitive
  assert(ci_contains("Documents", "MENT"));
  assert(ci_contains("anything", "")); // empty needle matches
  assert(!ci_contains("abc", "xyz"));
  assert(!ci_contains("ab", "abc")); // needle longer than haystack
}

static void load_missing_is_empty(void) {
  size_t n = 99;
  char **bm = load_bookmarks("/tmp/menucd_does_not_exist_xyz", &n);
  assert(bm != NULL);
  assert(n == 0);
  free(bm);
}

static const struct {
  const char *name;
  void (*fn)(void);
} tests[] = {
    {"data_dir_xdg", data_dir_xdg},
    {"data_dir_home_fallback", data_dir_home_fallback},
    {"data_dir_relative_xdg_ignored", data_dir_relative_xdg_ignored},
    {"runtime_dir_path", runtime_dir_path},
    {"ci_match", ci_match},
    {"bookmark_roundtrip", bookmark_roundtrip},
    {"load_missing_is_empty", load_missing_is_empty},
};

int main(void) {
  size_t n = sizeof tests / sizeof tests[0];
  for (size_t i = 0; i < n; i++) {
    printf("test_%s\n", tests[i].name);
    tests[i].fn();
  }
  printf("%zu tests passed\n", n);
  return 0;
}
