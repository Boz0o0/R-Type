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

The full documentation is **[here](https://boz0o0.github.io/Ampersand-Engine/)**.

## Modules

Each module is a static library. A game only builds the modules it uses.

| Module  | CMake target        | Needs             |
| ------- | ------------------- | ----------------- |
| Core    | `ampersand_core`    |                   |
| Physics | `ampersand_physics` | core              |
| Input   | `ampersand_input`   | core              |
| Render  | `ampersand_render`  | core, input, SFML |
| Audio   | `ampersand_audio`   | core, SFML        |
| Net     | `ampersand_net`     | core, Asio        |

All modules are on by default. Turn one off with `AMPERSAND_MODULE_<NAME>=OFF`
(e.g. `AMPERSAND_MODULE_RENDER=OFF`).

## Use it in a game

```cmake
include(FetchContent)
FetchContent_Declare(ampersand
    GIT_REPOSITORY https://github.com/Boz0o0/Ampersand-Engine.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(ampersand)

target_link_libraries(my_game PRIVATE ampersand_render ampersand_physics)
```

```cpp
#include <ampersand/core/ecs/World.hpp>
```

## Build the engine

Needs CMake 3.24+, a C++20 compiler and Git. The dependencies (SFML, Asio,
GoogleTest) are downloaded by CMake.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Project structure

```
.
├── include/ampersand/  # Public headers, one folder per module
├── src/                # Sources, one folder per module
├── tests/              # Unit tests, one folder per module
├── docs/               # Documentation (MkDocs)
├── cmake/              # CPM.cmake (downloads the dependencies)
└── CHANGELOG.md        # Changes of each version
```
