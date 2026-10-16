# Modules

The engine is split into modules, each built as its own static library. A game
only links the modules it uses, and a module that is turned off is not built
(nor its dependencies downloaded).
For why we split it this way, see [Design decisions](../decisions.md#separation-of-modules).

| Module  | CMake target        | Option                     | Needs           | Sources        | Tests            |
| ------- | ------------------- | -------------------------- | --------------- | -------------- | ---------------- |
| Core    | `ampersand_core`    | `AMPERSAND_MODULE_CORE`    |                 | `src/core/`    | `tests/core/`    |
| Physics | `ampersand_physics` | `AMPERSAND_MODULE_PHYSICS` | core            | `src/physics/` | `tests/physics/` |
| Render  | `ampersand_render`  | `AMPERSAND_MODULE_RENDER`  | core, input, SFML | `src/render/`  | `tests/render/`  |
| Input   | `ampersand_input`   | `AMPERSAND_MODULE_INPUT`   | core            | `src/input/`   | `tests/input/`   |
| Audio   | `ampersand_audio`   | `AMPERSAND_MODULE_AUDIO`   | core, SFML      | `src/audio/`   | `tests/audio/`   |
| Net     | `ampersand_net`     | `AMPERSAND_MODULE_NET`     | core, Asio      | `src/net/`     | `tests/net/`     |

Every option is `ON` by default. Turn one off at configure time:

```sh
cmake -B build -DAMPERSAND_MODULE_RENDER=OFF
```

Linking a target also links the modules it needs: `ampersand_render` brings
`ampersand_input` and `ampersand_core`.

The headers of a module are in `include/ampersand/<module>/`, with its
`components/`, `resources/` and `systems/` in folders of the same name.
