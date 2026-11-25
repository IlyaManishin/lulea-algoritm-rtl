subdir('lulea_common')
subdir('lulea_verify')

alias_target('libs_cpp', lulea_common_library, lulea_verify_library, verify_test)

libs_cpp_lint_targets = []
foreach tool : lint_tools
  libs_cpp_lint_targets += [
    run_target(
      'lint-libs-cpp-' + tool,
      command: lint_command + ['--scope', meson.current_source_dir(), '--tool', tool],
    ),
  ]
endforeach
libs_cpp_lint = alias_target('lint-libs-cpp', libs_cpp_lint_targets)
