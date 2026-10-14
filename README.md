<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/mkdocs/assets/ampersand-logo-light.svg">
    <img src="docs/mkdocs/assets/ampersand-logo-dark.svg" alt="Ampersand Engine logo" width="120">
  </picture>
</p>

<h1 align="center">Ampersand Engine</h1>

<p align="center">
  A modular C++20 game engine built around a sparse set ECS.
</p>

<p align="center">
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/build-check.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/build-check.yml/badge.svg" alt="Build Check"></a>
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/unit-tests.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/unit-tests.yml/badge.svg" alt="Unit Tests"></a>
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/format-lint-check.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/format-lint-check.yml/badge.svg" alt="Format & Lint Check"></a>
  <a href="https://boz0o0.github.io/Ampersand-Engine/"><img src="https://img.shields.io/badge/docs-online-blue" alt="Documentation"></a>
</p>

---

## Documentation

The full documentation can be found **[here](https://boz0o0.github.io/Ampersand-Engine/)**.

## Modules

The engine is split into libraries so that you only compile what you need.
Every module depends on `core`.

| Module  | CMake target        | Option                     |
| ------- | ------------------- | -------------------------- |
| Core    | `ampersand_core`    | `AMPERSAND_MODULE_CORE`    |
| Audio   | `ampersand_audio`   | `AMPERSAND_MODULE_AUDIO`   |
| Input   | `ampersand_input`   | `AMPERSAND_MODULE_INPUT`   |
| Network | `ampersand_net`     | `AMPERSAND_MODULE_NET`     |
| Physics | `ampersand_physics` | `AMPERSAND_MODULE_PHYSICS` |
| Render  | `ampersand_render`  | `AMPERSAND_MODULE_RENDER`  |

All modules are enabled by default.

Unit tests are triggered by `AMPERSAND_BUILD_TESTS`, which is `ON` only when the engine is the top-level project.

## Getting started

### Requirements

- CMake 3.24+
- A C++20 compiler (GCC 10+, Clang 12+ or MSVC 2019 16.10+)
- Git (dependencies are fetched with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake))

### Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

See [Getting started](https://boz0o0.github.io/Ampersand-Engine/getting-started/) for module options, build output and more.

## Project structure

```
.
├── cmake/        # CMake helpers (CPM)
├── docs/         # MkDocs documentation
├── src/          # Engine modules (core, audio, input, net, physics, render)
└── tests/        # Unit tests, one folder per module
```
