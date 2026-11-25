# Tasks

- [x] Fix unresolved Python import warnings in VS Code for the shared fixture generator. The imports resolve at runtime through `table_gen`, but `# noqa: E402` leaves the editor diagnostics in place.
- [x] Move `CacheEngineFactory` into an anonymous namespace in `test_lulea_cache_real.cpp`. Update `test_golden_model_real.cpp` to prepare its cache images without the shared factory header.
- [x] Update the namespace of `ConfigParser` in `cache_gen/src/io/config_parser.hpp` and adjust its implementation and callers.
- [ ] Move `table_gen` generated outputs (`converted/`, `sampled/`) from `build/` to `tables_build/` and update all scripts, Meson rules, and docs referencing `build/converted` and `build/sampled` (`table_gen/engine/config.py`, `libs_cpp/lulea_verify/meson.build`, `stats/cache_stats/collect_stats.py`).
- [ ] Decide on `clang-tidy` / `lint` scope (tests, `libs_cpp/lulea_verify`, `golden_model`) and resolve existing `clang-tidy` errors (`cache_gen/src/gen/lulea_data/cache_set.cpp`, `libs_cpp/lulea_verify/`, and test suites).
- [ ] Fix `-Wsign-conversion` / `-Wconversion` warnings in `cache_gen/tests/gen/test_gen.cpp`, `test_reader.cpp`, and `test_lulea_tree.cpp` under `-Dstrict_flags=true -Dwerror=true`.
