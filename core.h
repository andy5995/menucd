// SPDX-License-Identifier: Unlicense

#pragma once

#include <stddef.h>

// Allocation wrappers that abort (with a strerror message) when malloc
// returns NULL, so callers never branch on OOM.
void *xmalloc_at(size_t size, const char *func, int line);
void *xcalloc_at(size_t n, size_t size, const char *func, int line);
void *xrealloc_at(void *ptr, size_t size, const char *func, int line);
char *xstrdup_at(const char *s, const char *func, int line);

#define xmalloc(size) xmalloc_at((size), __func__, __LINE__)
#define xcalloc(n, size) xcalloc_at((n), (size), __func__, __LINE__)
#define xrealloc(ptr, size) xrealloc_at((ptr), (size), __func__, __LINE__)
#define xstrdup(s) xstrdup_at((s), __func__, __LINE__)

// Create every component of path (like mkdir -p), mode 0700.
int mkdir_p(const char *path);

// $XDG_DATA_HOME/menucd (default ~/.local/share/menucd), malloc'd; NULL
// when neither XDG_DATA_HOME nor HOME is usable.
char *data_dir(void);

// $XDG_DATA_HOME/menucd/bookmarks, creating the directory. NULL on no HOME.
char *save_file_path(void);

// One-time move of the legacy ~/.menucd-save into the XDG location.
void migrate_old_save(const char *new_path);

// $XDG_RUNTIME_DIR/menucd.cd.exit, else /tmp/menucd.cd.exit.
void cd_exit_path(char *buf, size_t n);

// Case-insensitive substring match; an empty needle matches everything.
int ci_contains(const char *hay, const char *needle);

// Sorted, malloc'd array of the current directory's subdirectory names.
char **read_dirs(size_t *n_out);

// Saved bookmarks, one malloc'd line each (blanks skipped). Never NULL.
char **load_bookmarks(const char *path, size_t *n_out);
void save_bookmark(const char *path, const char *dir);
void rewrite_bookmarks(const char *path, char **bm, size_t n);

// Write the current directory to the cd-exit file for the wrapper.
void quit_to_pwd(void);
