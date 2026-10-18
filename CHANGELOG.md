# Changelog

All notable changes to the engine are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

Everything here is written by hand.

**When making a pull request**, please add a short description of your changes
under `[Unreleased]`, in the right subsection (`Added`, `Changed`, `Fixed` or
`Removed`, create it if it is missing).

**When releasing a new version**:

1. In a pull request, rename `## [Unreleased]` to the new version and today's
   date (eg: `## [0.2.0] - 2026-10-16`), and add an empty `## [Unreleased]` above
   it.
2. Update the links at the bottom of this file.
3. Set the same version in `project()` of `CMakeLists.txt`
   (eg: `VERSION 0.2.0`).
4. Once it is merged, tag `main` and push the tag
   (`git tag v0.2.0 && git push origin v0.2.0`). The release workflow does the checks and creates the GitHub
   release.

## [Unreleased]

## [0.2.0] - 2026-10-18

Binary serialization for the network module.

### Added

- **Net**: `BinaryWriter` and `BinaryReader` for big-endian serialization of integers, finite floats, enums and length-prefixed strings, with bounds and input validation.

### Changed

- README: fixed the organisation section (`include/` folder)
- Getting started: use the engine with FetchContent (`v0.1.0`).

## [0.1.0] - 2026-10-16

First release, with the very base of the engine.

### Added

- **Core**: sparse set ECS (`World`, `Entity`, components, resources, `ISystem`), `Transform` component, `Clock` resource, `Vec2`, `Vec3` and `Rect` math types.
- **Physics**: `Velocity`, `Gravity` and `BoxCollider` components, `GravityForce` and `Collisions` resources, `MovementSystem` and `CollisionSystem`.
- **Render**: `Sprite`, `Text` and `Animation` components, `Renderer`, `Camera` and `ClearColor` resources, `RenderSystem` and `AnimationSystem`, SFML backend behind `IRenderer`.
- **Input**: `InputManager` resource and `InputSystem`, keyboard keys by physical position, mouse buttons and position, focus loss.
- **Audio**: `AudioSource` component, `AudioListener` and `SceneMusic` resources, `AudioSystem`, SFML backend behind `IAudioEngine`.
- **Net**: non-blocking IPv4 `UdpSocket`, with Asio hidden inside the module.
- One CMake option and one static library per module.
- Documentation website.

[Unreleased]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Boz0o0/Ampersand-Engine/releases/tag/v0.1.0
