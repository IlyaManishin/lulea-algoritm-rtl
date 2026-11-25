model_include = include_directories('include')
model_library = shared_library(
  'lulea_model',
  ['src/lulea_model.c', 'src/cache_images.c'],
  include_directories: model_include,
)
golden_model_dep = declare_dependency(
  include_directories: model_include,
  link_with: model_library,
)

popcount_test = executable(
  'test_popcount',
  'tests/test_popcount.c',
  include_directories: include_directories('src'),
)
test('popcount', popcount_test, suite: 'golden_model')

python = python_module.find_installation()
test(
  'pipeline',
  python,
  args: [files('tests/test_pipeline.py'), model_library],
  suite: 'golden_model',
)

model_lookup_test = executable(
  'test_golden_model_real',
  ['tests/test_golden_model_real.cpp', 'tests/golden_model_lookup.cpp'],
  dependencies: [golden_model_dep, cache_gen_dep, lulea_verify_dep, gtest_main_dep],
  cpp_args: strict_cpp_args,
)

test(
  'golden_model_lookup',
  model_lookup_test,
  suite: 'golden_model',
  workdir: verify_workdir,
  depends: verify_tables,
  timeout: 300,
)

alias_target('golden_model', model_library, popcount_test, model_lookup_test)
