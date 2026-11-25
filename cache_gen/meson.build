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
  'cache_gen_core',
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

io_sources = files('src/io/config_parser.cpp', 'src/io/cli.cpp', 'src/io/stats_writer.cpp')

cache_gen_exe = executable(
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
  'tests/lulea/lulea_engine/lulea_cache_lookup.cpp',
  'tests/lulea/lulea_engine/lulea_tree_lookup.cpp',
) + io_sources

test_exe = executable(
  'test_gen',
  test_sources,
  include_directories: [inc_dirs, include_directories('tests/lulea')],
  dependencies: [cache_gen_dep, lulea_verify_dep, gtest_main_dep],
  cpp_args: strict_cpp_args,
)

test('cache_gen_tests', test_exe, suite: 'cache_gen', workdir: verify_workdir, depends: verify_tables, timeout: 300)

alias_target('cache_gen', cache_gen_exe, test_exe)

cache_gen_lint_targets = []
foreach tool : lint_tools
  cache_gen_lint_targets += [
    run_target(
      'lint-cache-gen-' + tool,
      command: lint_command + ['--scope', meson.current_source_dir(), '--tool', tool],
    ),
  ]
endforeach
cache_gen_lint = alias_target('lint-cache-gen', cache_gen_lint_targets)
