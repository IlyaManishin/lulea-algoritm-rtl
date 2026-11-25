project(
  'lulea-algoritm-rtl',
  'c', 'cpp',
  version: '0.1.0',
  meson_version: '>=1.1.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++20',
    'warning_level=3',
    'buildtype=debugoptimized',
  ],
)

gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')
python_module = import('python')

cpp = meson.get_compiler('cpp')

gcc_strict_flags = [
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

strict_cpp_args = []
if get_option('strict_flags') and cpp.get_id() == 'gcc'
  strict_cpp_args = gcc_strict_flags
endif

subdir('libs_cpp')
subdir('cache_gen')
subdir('golden_model')

cppcheck_prog = find_program('cppcheck', required: false)
run_clang_tidy_prog = find_program('run-clang-tidy', required: false)

lint_scopes = {
  'libs-cpp': 'libs_cpp',
  'cache-gen': 'cache_gen',
}

cppcheck_base_args = [
  '--project=' + (meson.project_build_root() / 'compile_commands.json'),
  '--enable=all',
  '--inconclusive',
  '--library=posix',
  '--platform=unix64',
  '--check-level=exhaustive',
  '--suppress=missingIncludeSystem',
  '--suppress=unusedFunction',
  '--suppress=checkersReport',
  '--error-exitcode=1',
]

clang_tidy_base_args = [
  '-p', meson.project_build_root(),
  '-config-file=' + (meson.project_source_root() / '.clang-tidy'),
  '-removed-arg=-fanalyzer',
  '-quiet',
]

all_lint_targets = []
cppcheck_lint_targets = []
clang_tidy_lint_targets = []

foreach scope_name, scope_dir : lint_scopes
  scope_targets = []
  if cppcheck_prog.found()
    t_cppcheck = run_target(
      'lint-' + scope_name + '-cppcheck',
      command: [cppcheck_prog, cppcheck_base_args, '--file-filter=*/' + scope_dir + '/*'],
    )
    scope_targets += t_cppcheck
    cppcheck_lint_targets += t_cppcheck
  endif

  if run_clang_tidy_prog.found()
    t_tidy = run_target(
      'lint-' + scope_name + '-clang-tidy',
      command: [run_clang_tidy_prog, clang_tidy_base_args, '^' + (meson.project_source_root() / scope_dir) + '/.*'],
    )
    scope_targets += t_tidy
    clang_tidy_lint_targets += t_tidy
  endif

  if scope_targets.length() > 0
    all_lint_targets += alias_target('lint-' + scope_name, scope_targets)
  endif
endforeach

if all_lint_targets.length() > 0
  alias_target('lint', all_lint_targets)
endif
if cppcheck_lint_targets.length() > 0
  alias_target('lint-cppcheck', cppcheck_lint_targets)
endif
if clang_tidy_lint_targets.length() > 0
  alias_target('lint-clang-tidy', clang_tidy_lint_targets)
endif

