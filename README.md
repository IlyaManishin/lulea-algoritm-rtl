# Luleå RTL

Run Meson from the repository root. It uses C11, C++20, Ninja, GoogleTest, and
Python 3.12 or newer with `python-dotenv` for the shared fixtures.
The default build includes debug symbols and optimization.

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

| Component | Build target | Test suite | Lint target |
| --- | --- | --- | --- |
| [Cache generator](cache_gen/README.md) | `cache_gen` | `cache_gen` | `lint-cache-gen` |
| [C++ libraries](libs_cpp/lulea_verify/README.md) | `libs_cpp` | `libs_cpp` | `lint-libs-cpp` |
| [Golden model](golden_model/README.md) | `golden_model` | `golden_model` | |

```sh
meson compile -C build cache_gen
meson test -C build --suite cache_gen --print-errorlogs
meson compile -C build lint-cache-gen
meson compile -C build lint --ninja-args=-k,0
```

The last command runs both lint targets and keeps checking if one fails.
`lint-cppcheck` and `lint-clang-tidy` each run one tool across both components.
Use `lulea_builder` or `lulea_model` to build just the CLI or the C library.
