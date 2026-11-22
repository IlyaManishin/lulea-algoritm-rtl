project(
  'golden_model',
  'c',
  default_options: ['c_std=c11', 'warning_level=2', 'buildtype=release'],
)

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
test('popcount', popcount_test)

python = import('python').find_installation()
test(
  'pipeline',
  python,
  args: [files('tests/test_pipeline.py'), model_library],
)
