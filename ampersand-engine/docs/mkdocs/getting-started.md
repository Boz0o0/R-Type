# Getting started

## Requirements

- CMake 3.24+
- A C++20 compiler (GCC 10+, Clang 12+ or MSVC 2019 16.10+)
- Git: the dependencies are downloaded at configure time with
  [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)

| Dependency                                     | Version | Used by                 |
| ---------------------------------------------- | ------- | ----------------------- |
| [SFML](https://www.sfml-dev.org/)               | 3.0.2   | render, audio           |
| [Asio](https://think-async.com/Asio/) (standalone) | 1.30.2  | net (hidden, see [Net](architecture/net.md)) |
| [GoogleTest](https://github.com/google/googletest) | 1.15.2  | tests only              |

A dependency is only downloaded when a module that needs it is enabled.

## Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The tests are only built when the engine is the top-level project
(`AMPERSAND_BUILD_TESTS`), so a game that includes the engine does not build
them.

!!! tip "Faster configure"
    Add `-DCPM_SOURCE_CACHE=$HOME/.cache/CPM` to keep the downloaded
    dependencies between build folders.

## Choosing the modules

Every module is on by default. Turn one off with its option:

```sh
cmake -B build -DAMPERSAND_MODULE_RENDER=OFF -DAMPERSAND_MODULE_AUDIO=OFF
```

`core` is needed by every module, and `render` needs `input`: CMake stops with
an error if one is missing. The list of the modules and their options is in
[Modules](architecture/modules.md).

## Using the engine in a game

!!! tip "FetchContent"
    The engine will soon be released on GitHub, so you will be able to use it with
    [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html).

Put the engine in the game's repository, then add it and link the modules the
game uses:

```cmake
add_subdirectory(ampersand-engine)

target_link_libraries(my_game PRIVATE ampersand_render ampersand_physics)
```

Linking a module also gives its include folder and the modules it depends on.
Every header is under `ampersand/<module>/`:

```cpp
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
```
