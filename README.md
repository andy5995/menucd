[![Linux](https://github.com/andy5995/menucd/actions/workflows/linux.yml/badge.svg)](https://github.com/andy5995/menucd/actions/workflows/linux.yml)
[![MacOS](https://github.com/andy5995/menucd/actions/workflows/macos.yml/badge.svg)](https://github.com/andy5995/menucd/actions/workflows/macos.yml)
[![BSD](https://github.com/andy5995/menucd/actions/workflows/bsd.yml/badge.svg)](https://github.com/andy5995/menucd/actions/workflows/bsd.yml)
[![run shellcheck](https://github.com/andy5995/menucd/actions/workflows/shellcheck.yml/badge.svg)](https://github.com/andy5995/menucd/actions/workflows/shellcheck.yml)

# menucd
Directory browser and changer for the command line.

`menucd` shows a curses menu of the current directory's subdirectories plus your
saved bookmarks. Move with the arrow keys, filter as you type, save the current
directory as a bookmark — then pick one and your shell lands in it. It does only
that: browse, bookmark, and `cd`. For copy/move/delete, use a file manager.

## Requirements

* ncurses, including the menu library (`libmenuw` / `libmenu`)

To build: a C compiler and [meson](https://mesonbuild.com/).

Runs on Linux, the BSDs, and macOS. It won't run from the native Windows console
(no ncurses there) without WSL, MSYS2, or a similar environment.

## Build and install

```sh
meson setup _build
meson compile -C _build
meson install -C _build   # optional — installs the menucd binary
```

## Usage

`menucd` runs as a small wrapper shell function so it can change your
*current* shell's directory. A child process can't do that on its own, so the
binary hands the chosen directory back through a file that the function reads.

After installing the `menucd` binary (e.g. `meson install`), add this function
to your `~/.profile`, `~/.bashrc`, `~/.zshrc`, etc:

```sh
menucd() {
  cdfile="${XDG_RUNTIME_DIR:-/tmp}/menucd.cd.exit"
  rm -f "$cdfile"
  command menucd "$@"
  ret=$?
  if [ -r "$cdfile" ]; then
    cd "$(cat "$cdfile")" || return
    rm -f "$cdfile"
  fi
  return $ret
}
```

Reload your shell config:

    source ~/.profile

Then run `menucd`. Running the binary directly works, but it can't change your
shell's directory without this wrapper.

You can pass a starting directory as the first argument — `menucd ~/projects`
opens the menu there instead of the current directory.

## Key bindings

| Key | Action |
| --- | --- |
| Up / Down, PgUp / PgDn | move the selection |
| Backspace | go up to the parent directory (same as selecting `..`) |
| Enter | `cd` into the highlighted directory or bookmark |
| `s` | save the current directory as a bookmark |
| `d` | delete the bookmark under the cursor (asks first) |
| `/` | search — filter the list as you type (Enter accepts, ESC clears) |
| `q` | quit, returning your shell to the selected directory |
| `x` / ESC | cancel — quit without changing directory |
| `?` | show this key-binding help inside the program |

Bookmarks are stored in `$XDG_DATA_HOME/menucd/bookmarks` (default
`~/.local/share/menucd/bookmarks`).

