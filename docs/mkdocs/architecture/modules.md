# Modules

The engine is split into modules, each built as its own static library.
For why we split it this way, see [Design decisions](../decisions.md#separation-of-modules).

The render module depends on input because its window event stream includes
keyboard events defined by the input module.
For the renderer API and usage, see [Render](rendering.md).

| Module   | CMake target        | Option                     | Sources        | Tests            |
| -------- | ------------------- | -------------------------- | -------------- | ---------------- |
| Core     | `ampersand_core`    | `AMPERSAND_MODULE_CORE`    | `src/core/`    | `tests/core/`    |
| Audio    | `ampersand_audio`   | `AMPERSAND_MODULE_AUDIO`   | `src/audio/`   | `tests/audio/`   |
| Input    | `ampersand_input`   | `AMPERSAND_MODULE_INPUT`   | `src/input/`   | `tests/input/`   |
| Network  | `ampersand_net`     | `AMPERSAND_MODULE_NET`     | `src/net/`     | `tests/net/`     |
| Physics  | `ampersand_physics` | `AMPERSAND_MODULE_PHYSICS` | `src/physics/` | `tests/physics/` |
| Render   | `ampersand_render`  | `AMPERSAND_MODULE_RENDER`  | `src/render/`  | `tests/render/`  |
