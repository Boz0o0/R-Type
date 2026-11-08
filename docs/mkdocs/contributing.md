# Contributing

## Workflow

1. Create a ticket (a GitHub issue) for the work.
2. Create the branch from the ticket ("Create a branch" in the issue), so the branch is linked to it, for example `42-doc-rtype-and-correct-binaries-naming`.
3. Open a pull request to `main`. It is merged when a member of the team approved it and the CI is green.

The CI builds, lints and tests the project on Linux and Windows for every pull request.

Each pull request adds a short line under `[Unreleased]` in `CHANGELOG.md` (how to release a version: see [Release](#release)).

## Commits

We follow [Conventional Commits](https://www.conventionalcommits.org/), for example `feat(client): turn held keys into the input mask`.

## The engine

The engine is not in this repository. Change it in the [engine repository](https://github.com/Boz0o0/Ampersand-Engine) and release a new version there.

The engine version the game is built against is `AMPERSAND_ENGINE_VERSION` in the root `CMakeLists.txt`. When configuring, CMake pulls the release tagged `v<AMPERSAND_ENGINE_VERSION>` with CPM. To use a new engine release, bump this version in a pull request.

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

## Release

1. In a pull request, rename `## [Unreleased]` of the [Changelog](changelog.md) to the new version and today's date (eg: `## [1.1.0] - 2026-10-25`), add an empty `## [Unreleased]` above it, and update the links at the bottom of the file.
2. Set the same version in `project()` of `CMakeLists.txt` (eg: `VERSION 1.1.0`).
3. Once it is merged, tag `main` and push the tag (`git tag v1.1.0 && git push origin v1.1.0`). The release workflow does the checks and creates the GitHub release.
