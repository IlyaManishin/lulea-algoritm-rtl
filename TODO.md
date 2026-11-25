# Tasks

- [x] Fix unresolved Python import warnings in VS Code for the shared fixture generator. The imports resolve at runtime through `table_gen`, but `# noqa: E402` leaves the editor diagnostics in place.
- [x] Move `CacheEngineFactory` into an anonymous namespace in `test_lulea_cache_real.cpp`. Update `test_golden_model_real.cpp` to prepare its cache images without the shared factory header.
- [x] Update the namespace of `ConfigParser` in `cache_gen/src/io/config_parser.hpp` and adjust its implementation and callers.
- [x] Store converted and sampled routing tables in `tables_build/` and update their consumers and documentation.
- [ ] Decide on lint coverage for `golden_model` and resolve existing `clang-tidy` errors in `cache_gen/src/`.
- [ ] Fix `-Wsign-conversion` / `-Wconversion` warnings in `cache_gen/tests/gen/test_gen.cpp`, `test_reader.cpp`, and `test_lulea_tree.cpp` under `-Dstrict_flags=true -Dwerror=true`.
