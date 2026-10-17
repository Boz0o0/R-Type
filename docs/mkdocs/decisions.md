# Design decisions

This page explains the main technical choices of R-Type, and why.
The choices of the engine (ECS, modules) are in the [engine decisions](https://boz0o0.github.io/Ampersand-Engine/decisions/).

## Language: C++20
The subject imposes C++. We use C++20.

## Build and dependencies: CMake + CPM
We already used CMake, so we took the package manager that is only CMake: CPM is one file in `cmake/`, and a dependency is one `CPMAddPackage` call with its version. Nothing has to be installed on the system. Conan needs Python and vcpkg needs its own tool, and both keep the dependencies in another file.

## Graphics, audio and input: SFML
We use SFML by habit: we used CSFML in first year and SFML last year, so we already knew it and could go fast. We did check some other libraries some of us used in the past, Raylib for example (Yann used Raylib for the Zappy), but as most of us knew SFML, it was the easy choice.

## Network: UDP + Asio
The subject imposes UDP for the game. For the sockets, Asio was recommended to us (the subject names it too), so we looked at it first, and it works on Linux and Windows with the same code. With the system sockets, we would write it twice (POSIX and Winsock). We use the standalone version (no Boost), hidden inside the engine `net` module: the game only sees `UdpSocket`.

## Protocol: binary, field by field
The protocol is binary: packets are small and fast to check. A text format (JSON) would be bigger and slower to read. Every value is written one by one, in big-endian, never with a `memcpy` of a struct, so the bytes are the same on every compiler and machine. Details: [Protocol](protocol/specification.md).

## Server: two threads
The subject asks for a multithreaded server. We use two threads: the network thread and the game thread. They only share the socket (behind a mutex) and a bounded queue of packets. Details: [Architecture](architecture.md#server).

We also looked at one game thread plus one thread per client. With UDP it does not help: all the clients send to the same socket, so all the packets arrive in one place anyway. One thread per client would only add threads (more players, more threads) to do the same work.

## Algorithms and data structures

| Problem                         | Choice                                                        | Why                                                          |
| ------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------ |
| Entities and components         | sparse set ECS, one row per entity                            | see the [engine decisions](https://boz0o0.github.io/Ampersand-Engine/decisions/) |
| Deaths during a tick            | a `Deaths` list, destroyed at the end of the tick             | no system destroys an entity another one is using            |
| Packets between the two threads | a list behind a mutex, swapped once per tick, 4096 max        | the lock is held only for a push or a swap; a flood cannot fill the memory |
| Time                            | fixed tick of 1/60 s, at most 10 late ticks caught up         | same game on every CPU; no burst of ticks after a pause      |
| Collisions                      | every pair is tested (engine)                                 | simplest, and fast enough for a few hundred entities         |
| Players                         | an array of 4 sessions, the index is the player id            | a free id is an empty slot                                   |

## Storage
Nothing is saved for now: no score, no save.

## Security

| Threat                         | Handled | How                                                  |
| ------------------------------ | ------- | ---------------------------------------------------- |
| Malformed packet               | yes     | every field is checked, a bad packet is dropped      |
| Datagram too big               | yes     | more than 1200 bytes: dropped                        |
| Flood of packets               | yes     | at most 4096 packets wait for the game, more are dropped |
| Packet from an unknown address | yes     | only `CONNECT_REQUEST` is accepted                   |
| Bug in one tick                | yes     | logged, and the server keeps running                 |
| Someone reads the packets      | no      | nothing is encrypted                                 |
| Fake sender address            | no      | UDP does not check who sends a packet                |
| Any name                       | no      | anyone can join with any name                        |

To follow it over time: the server logs every dropped packet and every failed tick, the CI runs clang-tidy and the tests on every pull request, and every dependency has a fixed version, so updating one is a choice we make.

## Documentation: MkDocs + Doxygen
MkDocs (Material theme) to have the documentation as a website with a menu, a search bar and diagrams, published by the CI, as the subject requires (it's modern !).
Doxygen to have the documentation of the code itself. 
