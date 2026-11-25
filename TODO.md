# Tasks

- [ ] Fix unresolved Python import warnings in VS Code for the shared fixture generator. The imports resolve at runtime through `table_gen`, but `# noqa: E402` leaves the editor diagnostics in place.
- [ ] Move `CacheEngineFactory` into an anonymous namespace in `test_lulea_cache_real.cpp`. Update `test_golden_model_real.cpp` to prepare its cache images without the shared factory header.
- [ ] Update the namespace of `ConfigParser` in `cache_gen/src/io/config_parser.hpp` and adjust its implementation and callers.
