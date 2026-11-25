inc_dirs = include_directories('src', 'src/gen')

core_sources = files(
  'src/gen/gen.cpp',
  'src/gen/lulea_data/route.cpp',
  'src/gen/lulea_data/cache_set.cpp',
  'src/gen/lulea_algo/lulea_tree.cpp',
  'src/gen/lulea_algo/lulea_algo.cpp',
  'src/gen/io/bit_file_writer.cpp',
)

cache_gen_core = static_library(
  'cache_gen',
  core_sources,
  include_directories: inc_dirs,
  dependencies: lulea_common_dep,
  cpp_args: strict_cpp_args,
)

cache_gen_dep = declare_dependency(
  link_with: cache_gen_core,
  include_directories: include_directories('src/gen'),
  dependencies: lulea_common_dep,
)

cache_gen_test_support_dep = declare_dependency(
  include_directories: include_directories('tests/lulea/lulea_engines'),
  dependencies: cache_gen_dep,
)

io_sources = files('src/io/config_parser.cpp', 'src/io/cli.cpp', 'src/io/stats_writer.cpp')

executable(
  'lulea_builder',
  files('src/main.cpp', 'src/io/progress_bar.cpp') + io_sources,
  include_directories: inc_dirs,
  dependencies: cache_gen_dep,
  cpp_args: strict_cpp_args,
  install: true,
)

test_sources = files(
  'tests/test_config.cpp',
  'tests/test_stats_writer.cpp',
  'tests/gen/test_route.cpp',
  'tests/gen/test_cache_set.cpp',
  'tests/gen/test_lulea_tree.cpp',
  'tests/gen/test_lulea_cache_lookup.cpp',
  'tests/gen/test_reader.cpp',
  'tests/gen/test_bit_file_writer.cpp',
  'tests/gen/test_gen.cpp',
  'tests/lulea/test_lulea_tree_real.cpp',
  'tests/lulea/test_lulea_cache_real.cpp',
  'tests/lulea/lulea_engines/lulea_cache_lookup.cpp',
  'tests/lulea/lulea_engines/lulea_tree_lookup.cpp',
) + io_sources

test_exe = executable(
  'test_gen',
  test_sources,
  include_directories: [inc_dirs, include_directories('tests/lulea')],
  dependencies: [cache_gen_dep, lulea_verify_dep, gtest_main_dep],
  cpp_args: strict_cpp_args,
)

test('cache_gen_tests', test_exe, workdir: verify_workdir, depends: verify_tables, timeout: 300)

lint_script = files('scripts/run_linters.sh')

run_target(
  'lint',
  command: [lint_script, '--build-dir', meson.project_build_root()],
)

run_target(
  'lint-cppcheck',
  command: [lint_script, '--build-dir', meson.project_build_root(), '--cppcheck-only'],
)

run_target(
  'lint-clang-tidy',
  command: [lint_script, '--build-dir', meson.project_build_root(), '--clang-tidy-only'],
)
