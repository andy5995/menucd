project(
  'menucd',
  'c',
  version: '0.3.0',
  meson_version: '>= 0.59.0',
  default_options: [
    'c_std=gnu99',
    'warning_level=3',
    'pkg_config_path=/opt/homebrew/opt/ncurses/lib/pkgconfig,/usr/local/opt/ncurses/lib/pkgconfig',
  ],
)

cc = meson.get_compiler('c')

extra_flags = [
  '-fno-common',
  '-fstack-protector-strong',
  '-Wformat-security',
  '-Wshadow',
]
add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())

check_headers = [
  ['ncursesw/menu.h', 'HAVE_NCURSESW_MENU_H'],
  ['ncurses/menu.h', 'HAVE_NCURSES_MENU_H'],
  ['menu.h', 'HAVE_MENU_H'],
  ['ncursesw/curses.h', 'HAVE_NCURSESW_CURSES_H'],
  ['ncurses/curses.h', 'HAVE_NCURSES_CURSES_H'],
  ['ncurses.h', 'HAVE_NCURSES_H'],
  ['curses.h', 'HAVE_CURSES_H'],
]

foreach h : check_headers
  if cc.has_header(h[0])
    conf.set(h[1], 1)
  endif
endforeach

dep_curses = dependency('ncursesw', required: false)
if not dep_curses.found()
  dep_curses = dependency('curses')
endif

dep_menu = dependency('menuw', required: false)
if not dep_menu.found()
  dep_menu = dependency('menu', required: false)
  if not dep_menu.found()
    dep_menu = cc.find_library('menuw', required: false)
    if not dep_menu.found()
      dep_menu = cc.find_library('menu')
    endif
  endif
endif

config_h = configure_file(output: 'config.h', configuration: conf)

core_lib = static_library(
  'menucd_core',
  'core.c',
)

menucd_exe = executable(
  'menucd',
  'menucd.c',
  config_h,
  link_with: core_lib,
  dependencies: [dep_curses, dep_menu],
  install: true,
)

test('version', menucd_exe, args: ['-v'])

unit_tests = ['core']
foreach t : unit_tests
  exe = executable('test_' + t, 'test_' + t + '.c', link_with: core_lib)
  test(t, exe)
endforeach
