model_include = include_directories('include')
model_library = shared_library(
  'lulea_model',
  ['src/lulea_model.c', 'src/cache_images.c'],
  include_directories: model_include,
  override_options: ['buildtype=release', 'warning_level=2'],
)
golden_model_dep = declare_dependency(
  include_directories: model_include,
  link_with: model_library,
)

popcount_test = executable(
  'test_popcount',
  'tests/test_popcount.c',
  include_directories: include_directories('src'),
  override_options: ['buildtype=release', 'warning_level=2'],
)
test('popcount', popcount_test)

python = python_module.find_installation()
test(
  'pipeline',
  python,
  args: [files('tests/test_pipeline.py'), model_library],
)

model_lookup_test = executable(
  'test_golden_model_real',
  ['tests/test_golden_model_real.cpp', 'tests/golden_model_lookup.cpp'],
  dependencies: [golden_model_dep, cache_gen_test_support_dep, lulea_verify_dep, gtest_main_dep],
)

test(
  'golden_model_lookup',
  model_lookup_test,
  workdir: verify_workdir,
  depends: verify_tables,
  timeout: 300,
)
