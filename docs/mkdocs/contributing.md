# Contributing

## Workflow

1. Create a ticket (a GitHub issue) for the work.
2. Create the branch from the ticket ("Create a branch" in the issue), so the branch is linked to it, for example `42-doc-rtype-and-correct-binaries-naming`.
3. Open a pull request to `main`. It is merged when a member of the team approved it and the CI is green.

The CI builds, lints and tests the project on Linux and Windows for every pull request.

Each pull request adds a short line under `[Unreleased]` in `CHANGELOG.md` (how to release a version is at the top of that file).

## Commits

We follow [Conventional Commits](https://www.conventionalcommits.org/), for example `feat(client): turn held keys into the input mask`.

## The engine

`ampersand-engine/` is a git subtree of the [engine repository](https://github.com/Boz0o0/Ampersand-Engine). Change the engine in its own repository, then pull it here:

```sh
git subtree pull --prefix=ampersand-engine git@github.com:Boz0o0/Ampersand-Engine.git main --squash
```

## Code style

- **Format**: clang-format, Google style (`.clang-format`).
- **Lint**: clang-tidy (`.clang-tidy`), every warning is an error.
- Both run in the CI, and before each commit with [pre-commit](https://pre-commit.com/): `pre-commit install`.

| What                    | Style         | Example                        |
| ----------------------- | ------------- | ------------------------------ |
| Classes, structs, enums | `PascalCase`  | `ShootingSystem`, `EntityKind` |
| Functions, variables    | `camelCase`   | `createMissile`, `deltaTime`   |
| Private members         | `_camelCase`, used with `this->` | `this->_world` |
| Struct fields           | `camelCase`   | `Weapon::cooldown`             |
| Constants               | `UPPER_CASE`  | `MAX_DATAGRAM_SIZE`            |

- Headers go in `include/rtype/<part>/`, sources in `src/<part>/`, with `#pragma once`.
- Public classes and functions are documented in their header, with Doxygen comments (`/** @brief ... */`).
- The gameplay (`src/common/`) never includes the protocol nor the network (see [Architecture](architecture.md#rules)).

## Tests

GoogleTest, one file per class: `tests/<part>/tests_<name>.cpp`. A test name says what it checks: `TEST(ShootingSystem, FiresAtOnceThenEveryCooldown)`.

```sh
ctest --test-dir build --output-on-failure
```
