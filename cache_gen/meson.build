project(
  'cache_gen',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'optimization=3',
  ],
)

inc_dirs = include_directories('include', 'src', 'src/gen', 'src/gen/internal')

core_sources = files(
  'src/gen/gen.cpp',
  'src/gen/internal/reader.cpp',
  'src/parser.cpp',
)

executable(
  'lulea_builder',
  files('src/main.cpp') + core_sources,
  include_directories: inc_dirs,
  install: true,
)

gtest_dep = dependency('gtest_main')

test_sources = files(
  'tests/test_config.cpp',
  'tests/gen/test_reader.cpp',
) + core_sources
test_exe = executable(
  'test_gen',
  test_sources,
  include_directories: inc_dirs,
  dependencies: gtest_dep,
)

test('cache_gen_tests', test_exe)
