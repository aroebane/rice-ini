test('test-ini', executable('test-ini', 'test-ini.c', include_directories: inc,
  c_args: lib_args, link_with: rice_ini))

if get_option('default_library') != 'static'
  test('test-exports', find_program('sh'), args: [files('check-exports.sh'), rice_ini])
endif

if add_languages('cpp', required: false, native: false)
  test('test-cxx98', executable('cxx98', 'cxx98.cpp', include_directories: inc,
    cpp_args: ['-std=gnu++98'], link_with: rice_ini))
endif
