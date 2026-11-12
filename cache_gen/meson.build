project(
  'cache_gen',
  'cpp',
  version: '0.1.0',
  meson_version: '>=1.1.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'optimization=3',
  ],
)

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

if get_option('strict_flags') and cpp.get_id() == 'gcc'
  add_project_arguments(gcc_strict_flags, language: 'cpp')
endif

inc_dirs = include_directories('src')

core_sources = files(
  'src/gen/gen.cpp',
  'src/gen/core/cache.cpp',
  'src/gen/core/lulea_tree.cpp',
  'src/gen/core/lulea_algo.cpp',
  'src/gen/io/reader.cpp',
  'src/gen/io/bit_file_writer.cpp',
  'src/io/config_parser.cpp',
  'src/io/cli.cpp',
  'src/io/stats_writer.cpp',
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
  'tests/test_stats_writer.cpp',
  'tests/gen/test_cache.cpp',
  'tests/gen/test_lulea_tree.cpp',
  'tests/gen/test_lulea_algo.cpp',
  'tests/gen/test_lulea_config.cpp',
  'tests/gen/test_reader.cpp',
  'tests/gen/test_bit_file_writer.cpp',
) + core_sources

test_exe = executable(
  'test_gen',
  test_sources,
  include_directories: inc_dirs,
  dependencies: gtest_dep,
)

test('cache_gen_tests', test_exe)

lint_script = files('scripts/run_linters.sh')

run_target(
  'lint',
  command: [lint_script],
)

run_target(
  'lint-cppcheck',
  command: [lint_script, '--cppcheck-only'],
)

run_target(
  'lint-clang-tidy',
  command: [lint_script, '--clang-tidy-only'],
)
