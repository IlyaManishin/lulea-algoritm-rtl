fs = import('fs')
verify_workdir = meson.current_build_dir()
verify_include = include_directories('include')

project_python = meson.project_source_root() / 'venv/bin/python3'
if fs.is_file(project_python)
  fixture_python = python_module.find_installation(project_python, modules: ['dotenv'])
else
  fixture_python = python_module.find_installation('python3', modules: ['dotenv'])
endif

fixture_args = [
  '--output-dir', verify_workdir / 'tables',
  '--count', get_option('verify_route_count').to_string(),
  '--num-tables', get_option('verify_table_count').to_string(),
  '--stamp', '@OUTPUT@',
  '--depfile', '@DEPFILE@',
]
fixture_inputs = files(
  '../../table_gen/engine/config.py',
  '../../table_gen/engine/core/gen_types.py',
  '../../table_gen/engine/core/mrt_filter.py',
  '../../table_gen/engine/utils/route_reader.py',
  '../../table_gen/engine/utils/route_writer.py',
)
routes_option = get_option('verify_routes')
if routes_option != ''
  verify_routes = meson.project_source_root() / routes_option
  if not fs.is_file(verify_routes)
    error('Verification route table does not exist: ' + verify_routes)
  endif
else
  verify_routes = meson.project_source_root() / 'build/converted/ris_ripe_net/rrc00_bview_20250601_0000/routes.txt'
endif
if fs.is_file(verify_routes)
  fixture_args += ['--input', verify_routes]
endif

verify_tables = custom_target(
  'lulea_verify_tables',
  output: 'tables.stamp',
  depfile: 'tables.d',
  command: [fixture_python, files('scripts/generate_tables.py')] + fixture_args,
  depend_files: fixture_inputs,
)

lulea_verify_library = static_library(
  'lulea_verify',
  [
    'src/ip_gen.cpp',
    'src/linear_lookup.cpp',
    'src/lookup_verification.cpp',
    'src/utils.cpp',
  ],
  include_directories: verify_include,
  dependencies: [lulea_common_dep, gtest_dep],
  cpp_args: strict_cpp_args,
  link_depends: verify_tables,
)

lulea_verify_dep = declare_dependency(
  include_directories: verify_include,
  link_with: lulea_verify_library,
  dependencies: [lulea_common_dep, gtest_dep],
)

verify_test = executable(
  'test_lulea_verify',
  [
    'tests/test_ip_gen.cpp',
    'tests/test_linear_lookup.cpp',
    'tests/test_route_table_read.cpp',
  ],
  include_directories: include_directories('src'),
  dependencies: [lulea_verify_dep, gtest_main_dep],
  cpp_args: strict_cpp_args,
)

test('lulea_verify_tests', verify_test, workdir: verify_workdir, depends: verify_tables)
