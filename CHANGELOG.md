# Changelog

All notable changes to the game are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

**When making a pull request**, please add a short description of your changes
under `[Unreleased]`, in the right subsection (`Added`, `Changed`, `Fixed` or
`Removed`, create it if it is missing).

**When releasing a new version**:

1. In a pull request, rename `## [Unreleased]` to the new version and today's
   date (eg: `## [1.1.0] - 2026-10-25`), and add an empty `## [Unreleased]` above
   it.
2. Update the links at the bottom of this file.
3. Set the same version in `project()` of `CMakeLists.txt`
   (eg: `VERSION 1.1.0`).
4. Once it is merged, tag `main` and push the tag
   (`git tag v1.1.0 && git push origin v1.1.0`). The release workflow does the
   checks and creates the GitHub release.

## [Unreleased]

### Changed

- **Build**: Ampersand Engine is fetched with CPM from its GitHub release (`AMPERSAND_ENGINE_VERSION` in `CMakeLists.txt`, currently `0.2.0`) instead of being included as a git subtree.

### Removed

- The `ampersand-engine/` git subtree.

## [1.0.0] - 2026-10-18

First playable version (Part 1).

### Added

- **Gameplay**: ships, enemies and missiles as ECS components and systems in `src/common/`.
- **Server**: up to 4 players, 10 waves, 3 lives per player, score, victory or defeat, new game 5 s after the end. Network on its own thread, game at 60 ticks per second.
- **Protocol**: binary UDP protocol (see the specification), reliable packets with acks and resends, timeout of silent players, packet limit per sender.
- **Client**: animated sprites, one colour per player, explosions, sounds, HUD (connection, health, lives, score, events), scrolling background.
- Built with CMake and CPM on Linux and Windows, CI with lint, build and tests.
- Documentation website: architecture, gameplay, design decisions, contributing, protocol.

[Unreleased]: https://github.com/Boz0o0/R-Type/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Boz0o0/R-Type/releases/tag/v1.0.0
