project(
  'lulea-algoritm-rtl',
  'c', 'cpp',
  version: '0.1.0',
  meson_version: '>=1.1.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++20',
    'warning_level=3',
    'optimization=3',
  ],
)

gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')
python_module = import('python')

cpp = meson.get_compiler('cpp')

gcc_strict_flags = [
  '-fanalyzer',
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-Werror',
  '-Wshadow',
  '-Wconversion',
  '-Wsign-conversion',
  '-Wnull-dereference',
  '-Wdouble-promotion',
  '-Wformat=2',
  '-Wimplicit-fallthrough',
  '-Wcast-align',
  '-Wstrict-overflow=5',
]

strict_cpp_args = []
if get_option('strict_flags') and cpp.get_id() == 'gcc'
  strict_cpp_args = gcc_strict_flags
endif

subdir('libs_cpp/lulea_common')
subdir('libs_cpp/lulea_verify')
subdir('cache_gen')
subdir('golden_model')
