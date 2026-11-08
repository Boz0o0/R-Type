# Design decisions

The technical choices of R-Type, compared with what we could have used
instead. The choices of the engine (ECS, modules, Lua) are in the
[engine decisions](https://boz0o0.github.io/Ampersand-Engine/decisions/).

## Comparative study

| Need | Choice | Alternatives | Why |
| --- | --- | --- | --- |
| Language | C++20 | C++17 | imposed C++; C++20 gives designated initializers, `std::span` and ranges, used everywhere |
| Build | CMake | Meson, Xmake | known by the team, supported by every IDE and CI |
| Dependencies | CPM | Conan, vcpkg | one file in `cmake/`, one call per dependency with its version; Conan needs Python, vcpkg its own tool |
| Graphics, audio, input | SFML (in the engine) | SDL, Raylib | known by the whole team; the others meant learning a new library |
| Sockets | Asio standalone (in the engine) | POSIX sockets + Winsock, SFML Network | the same code on Linux and Windows, without Boost |
| Transport | UDP only | TCP for the reliable packets | imposed for the game; a few reliable packets with ACKs avoid a second connection, and TCP would block every packet behind a lost one |
| Encoding | binary, field by field, big-endian | JSON, `memcpy` of structs, Protobuf | small and fast to check; `memcpy` depends on the compiler (padding, endianness); Protobuf is a big dependency for about 25 packets |
| Server threads | network, lobby, passwords, one per party | one game thread + one per client, a thread pool | a party is the old single game, unchanged; with UDP every packet arrives on one socket anyway; a pool only pays off with hundreds of parties (16 at most) |
| Lag | prediction of our ship, smoothing of the others | interpolation in the past, lockstep | the controls answer at once; the others absorb late snapshots without adding delay; lockstep waits for the slowest player |
| Game content | JSON prefabs and Lua scripts (engine) | C++ only | changed without recompiling, shared by the client and the server |
| Storage | SQLite | a JSON file, PostgreSQL or MySQL | one C file built with CPM, safe if the server stops during a write; a server database needs a service to install |
| Passwords | Argon2id | bcrypt, PBKDF2, SHA-256 | first choice of OWASP, slow and memory-hard on purpose; SHA-256 can be tried billions of times per second |
| Crypto library | Monocypher | libsodium, OpenSSL | one small audited C file; the others are a lot bigger to build on Linux and Windows |
| Documentation | MkDocs Material + Doxygen | Sphinx, GitBook, a wiki | Markdown in the repository, search and diagrams, published by the CI |

## Algorithms and data structures

| Problem | Choice | Why |
| --- | --- | --- |
| Deaths during a tick | a `Deaths` list, destroyed at the end of the tick | no system destroys an entity another one is using |
| Packets between threads | a list behind a mutex, swapped once per tick, 4096 max | the lock is held only for a push or a swap; a flood cannot fill the memory |
| Clients and their party | a hash map (IP + port -> client), only touched by the lobby | no lock; the parties send their changes through a queue |
| Accounts | a SQLite table, username indexed | finding a username does not read every account |
| Failed logins | a hash map (IP -> attempts this minute), 4096 IPs max | spoofed IPs cannot make it grow without end |
| Code of a private room | a hash map (code -> party), 6 random digits | digits are typed the same way on every keyboard layout |
| Time | fixed tick of 1/60 s, at most 10 late ticks caught up | the same game on every CPU, no burst after a pause |
| Players | an array of 4 sessions, the index is the player id | a free id is an empty slot |

## Accounts

Every query is a prepared statement, so a username cannot change the query
(SQL injection), and usernames ignore the letter case (`Alice` = `alice`).

A password is never stored: only its Argon2id hash (19 MiB, 2 passes) and a
random salt of 16 bytes. Hashes are compared in constant time, and an unknown
username is checked against a fake hash, so it takes the same time and gets
the same answer as a wrong password. A hash takes tens of milliseconds, so it
runs on its own thread (`AuthWorker`, 64 waiting at most): the lobby keeps
answering the others.

## Security

| Threat | Handled | How |
| --- | --- | --- |
| Malformed or too big packet | yes | every field is checked, a bad packet is dropped |
| Flood of packets | yes | 500 packets per second per sender, 4096 waiting at most |
| Packet from an unknown address | yes | only `LOGIN` and `REGISTER` are accepted |
| Stolen database, many tries | yes | Argon2id hashes only, 5 failed logins per IP per minute |
| Finding which usernames exist | yes | the same answer, in the same time |
| SQL injection | yes | prepared statements only |
| Too many clients or rooms | yes | 256 clients, 16 rooms, silent clients dropped after 5 s |
| Amplification attack | yes | answers to unknown addresses are smaller than the requests |
| Bug in one tick | yes | logged, the server keeps running |
| Someone reads the packets | no | nothing is encrypted, not even the password |
| Fake sender address | no | UDP does not check who sends a packet |

To follow it over time, the server logs every dropped packet and failed tick,
the CI runs clang-tidy and the tests on every pull request, and every
dependency has a fixed version.
