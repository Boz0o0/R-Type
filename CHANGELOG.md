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

### Added

- **Server**: several parties at once. Each party has its own thread and tick loop; a new client goes to the first party with a free place, or to a new party when they are all full (16 at most). Empty parties are closed.
- **Protocol**: party codes (protocol version 2). `CONNECT_REQUEST` carries an optional `partyCode`; players with the same code join the same private party, which players without the code never join. New reject reasons `party full` and `invalid party code`.
- **Client**: optional fifth argument, the party code: `r-type_client [host] [port] [name] [party code]`.
- **Client**: controllers: the left stick moves, A shoots and Start quits.
- **Client**: smooth movement. Our own ship is predicted: it moves as soon as a key is pressed, then slides to the server's position once both have stopped. The other entities slide toward each snapshot instead of jumping to it.
- **Common**: `ShipMovement`, the speed, diagonals and screen edges of a ship, shared by the server and the client's prediction.
- **Server**: a lobby. A new client enters the lobby, sees the public rooms, and creates one (public, or private with a code of 6 digits made by the server) or joins one (from the list, with a code, or with a quick match). It can leave its room to go back to the lobby. Clients of the lobby are dropped after 5 s of silence; 256 clients at most.
- **Protocol**: lobby packets (protocol version 3): `ROOM_LIST`, `ROOM_CREATE`, `ROOM_JOIN`, `ROOM_LEAVE` from the client, `ROOM_LIST_REPLY`, `ROOM_JOINED`, `ROOM_REJECT`, `ROOM_LEFT` from the server, and the reject reason `room not found`. `ROOM_CREATE` and `ROOM_JOIN` carry a `requestId`, sent back in `ROOM_JOINED` and `ROOM_REJECT`.
- **Protocol**: late packets of the lobby are handled. A late copy of `CONNECT_REQUEST` (less than 5 s after `CONNECT_ACCEPT`) no longer takes a player out of its room; answers to an older room request are ignored; a room request with a newer `requestId` takes the player out of the room it is still in on the server; the client drops the game packets and the `ROOM_LEFT` older (header sequence) than the `ROOM_JOINED` of its room, and goes back to the lobby on a `ROOM_LEFT` it did not ask for.
- **Server**: a client of the lobby that sends `CONNECT_REQUEST` again has its name checked and updated; a room left empty frees its place before the next requests of the same step; on shutdown, every client gets `DISCONNECT(server shutdown)`, even one on its way back to the lobby.
- **Client**: the lobby screen (quick match, create a public or a private room, join with a code typed with the digits, list of the public rooms). The HUD shows the room, or its code. In a room, Escape (or Start) goes back to the lobby.
- **Client**: the look of the game is data, in `game/` (read by the engine scene module): `world/config.json` (window, sound, camera, input actions), `world/scenes/game.json` (the star field) and `world/prefabs/` (ships, enemy, missiles, explosion, star-field layers). Changing a sprite, a sound or a key only means editing JSON.
- **Client**: Lua scripting: the star field scrolls with `game/scripts/scroll.lua`. A broken script is printed and turned off, the game goes on.
- **Tests**: every file of `game/` is loaded in the CI with fake window and sound backends, so a missing field, file or key fails there.

### Changed

- **Protocol**: version 3. `CONNECT_REQUEST` only carries the name and puts the client in the lobby; the party code moved to `ROOM_JOIN`. `CONNECT_ACCEPT` only carries the tick rate and is no longer reliable; the player id now comes in `ROOM_JOINED`.
- **Client**: with a party code on the command line, the client joins that private room as soon as it is in the lobby. Without one, it shows the lobby instead of joining any public party.
- **Build**: Ampersand Engine is fetched with CPM from its GitHub release (`AMPERSAND_ENGINE_VERSION` in `CMakeLists.txt`) instead of being included as a git subtree.
- **Build**: Ampersand Engine goes from 0.2.0 to 0.5.1.
- **Client**: ported to the new engine: keys are `KeyCode`s read through the `InputManager` getters, and the draw order is the `layer` of the `Sprite` and `Text` instead of the `z` of the `Transform`.
- **Client**: the loop is the engine `Scheduler`, timed by a `FrameTimer`. The network work is a `NetworkSystem`, the lobby a `LobbySystem`, quitting or leaving the room a `QuitSystem`, and explosions are destroyed through the engine `CommandQueue`.
- **Client**: the controls are engine input actions (`Horizontal` and `Vertical` axes, `Fire` and `Quit` buttons). Holding two opposite arrows now cancels out.

### Removed

- The `ampersand-engine/` git subtree.
- **Client**: `AssetLoader`, `EntityLooks`, `Starfield`, `ScrollingSystem` and the `Scrolling` component, replaced by the `game/` files. `assets/` moved to `game/assets/`.

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
