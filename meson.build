project('rice-ini', ['c'],
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=1.0',
  default_options: ['c_std=c99', 'warning_level=3', 'werror=true'])

inc = include_directories('include')

# A vendored copy (static library) exports nothing; the shared library exports the API.
if get_option('default_library') == 'static'
  lib_args = ['-DRICEINI_STATIC']
else
  lib_args = ['-DRICEINI_BUILDING']
endif

rice_ini = library('rice-ini', 'src/ini.c',
  include_directories: inc, c_args: lib_args,
  version: meson.project_version(), soversion: '0',
  gnu_symbol_visibility: 'hidden', install: true)

install_subdir('include/rice-ini', install_dir: get_option('includedir'))

pkg = import('pkgconfig')
pkg.generate(rice_ini, name: 'rice-ini',
  description: 'INI reader', subdirs: [])

rice_ini_dep = declare_dependency(include_directories: inc, link_with: rice_ini,
  compile_args: get_option('default_library') == 'static' ? ['-DRICEINI_STATIC'] : [])

if get_option('tests')
  subdir('tests')
endif
