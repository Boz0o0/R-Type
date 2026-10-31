<h1 align="center">R-Type</h1>

<p align="center">
  <a href="https://github.com/Boz0o0/R-Type/actions/workflows/ci.yml"><img src="https://github.com/Boz0o0/R-Type/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://boz0o0.github.io/R-Type/"><img src="https://img.shields.io/badge/docs-online-blue" alt="Documentation"></a>
</p>

---

## Documentation

- **R-Type:** [boz0o0.github.io/R-Type](https://boz0o0.github.io/R-Type/)
- **Ampersand Engine:** [boz0o0.github.io/Ampersand-Engine](https://boz0o0.github.io/Ampersand-Engine/)

## Getting started

### Requirements

- CMake 3.24+
- A C++20 compiler (GCC, Clang or MSVC)
- Git (dependencies are fetched with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake))
- On Linux, the system libraries SFML needs:

```sh
sudo apt-get install libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev
```

Supported platforms: Linux and Windows (both built and tested by the CI).

### Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The executables are placed in `build/bin/`: `r-type_client` and `r-type_server`.

To build only one of them, turn the other off with `-DRTYPE_CLIENT=OFF` or `-DRTYPE_SERVER=OFF`.

### Run

Start the server, then one client per player (4 players per party, 16 parties at once):

```sh
./build/bin/r-type_server [port]                              # default port: 4242
./build/bin/r-type_client [host] [port] [name] [party code]   # default: 127.0.0.1 4242 Player
```

Without a party code, a client joins any public party with a free place. To
play with your friends only, give everyone the same code (1 to 8 letters and
digits, e.g. `./build/bin/r-type_client 127.0.0.1 4242 Alice FRIENDS`): you
all join the same private party.

Controls: the arrow keys move and Space shoots, Escape quits. A controller
works too: left stick, A to shoot, Start to quit.

Stop the server with `Ctrl+C`: it tells every client before closing.

## Project structure

```
.
├── assets/             # Client sprites, star-field, font and sounds (sources in CREDITS.md)
├── cmake/              # CMake helpers (CPM, also used to fetch Ampersand Engine)
├── docs/
│   ├── doxygen/        # Doxygen output (generated, not committed)
│   └── mkdocs/         # MkDocs documentation
├── include/rtype/      # Headers (client, common, server)
├── src/
│   ├── client/         # Game client
│   ├── common/         # Code shared by the client and the server
│   └── server/         # Game server
└── tests/              # Unit tests, one folder per part
```

## Authors

- Enzo Laugel
- Maxime Entz
- Francis Nguyen
- Yann Toison-Chabane

## License

All rights reserved, see [LICENSE](LICENSE).
