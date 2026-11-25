lulea_common_include = include_directories('include')
lulea_common_library = static_library(
  'lulea_common',
  'src/reader.cpp',
  include_directories: lulea_common_include,
  cpp_args: strict_cpp_args,
)

lulea_common_dep = declare_dependency(
  include_directories: lulea_common_include,
  link_with: lulea_common_library,
)
