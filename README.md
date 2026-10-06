<h1 align="center">R-Type</h1>

<p align="center">
  <a href="https://github.com/Boz0o0/R-Type/actions/workflows/ci.yml"><img src="https://github.com/Boz0o0/R-Type/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://boz0o0.github.io/R-Type/"><img src="https://img.shields.io/badge/docs-online-blue" alt="Documentation"></a>
</p>

---

## Documentation

- **R-Type:** [boz0o0.github.io/R-Type](https://boz0o0.github.io/R-Type/)
- **Ampersand Engine:** [boz0o0.github.io/Ampersand-Engine](https://boz0o0.github.io/Ampersand-Engine/)

## Getting started

### Requirements

- CMake 3.16+
- A C++17 compiler (GCC, Clang or MSVC)
- Git (dependencies are fetched with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake))

### Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The executables are placed in `build/bin/`: `rtype_client` and `rtype_server`.

To build only one of them, turn the other off with `-DRTYPE_CLIENT=OFF` or `-DRTYPE_SERVER=OFF`.

## Project structure

```
.
├── ampersand-engine/   # Ampersand Engine, included as a git subtree
├── cmake/              # CMake helpers (CPM)
├── docs/               # MkDocs documentation
├── src/
│   ├── client/         # Game client
│   ├── common/         # Code shared by the client and the server
│   └── server/         # Game server
└── tests/              # Unit tests, one folder per part
```
