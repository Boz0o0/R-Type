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

## Server: one thread per party
The subject asks for a multithreaded server. We use a network thread, a lobby thread, a password thread, and one thread per party. They only share the socket (behind a mutex) and bounded queues of packets: the lobby sends each packet to the queue of its party, and the parties tell the lobby who joined or left through another queue. Details: [Architecture](architecture.md#server).

A thread per party keeps each party exactly like the single game we had before: its own tick loop, its own `World`, nothing shared. A pool of threads running every party in turn would only pay off with hundreds of parties; there are 16 at most, so that the server cannot be made to start threads without end.

We also looked at one game thread plus one thread per client. With UDP it does not help: all the clients send to the same socket, so all the packets arrive in one place anyway. One thread per client would only add threads (more players, more threads) to do the same work.

## Algorithms and data structures

| Problem                         | Choice                                                        | Why                                                          |
| ------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------ |
| Entities and components         | sparse set ECS, one row per entity                            | see the [engine decisions](https://boz0o0.github.io/Ampersand-Engine/decisions/) |
| Deaths during a tick            | a `Deaths` list, destroyed at the end of the tick             | no system destroys an entity another one is using            |
| Packets between threads         | a list behind a mutex, swapped once per tick, 4096 max        | the lock is held only for a push or a swap; a flood cannot fill the memory |
| Clients and their party         | a hash map (IP + port -> name, party, account), only touched by the lobby | no lock needed; the parties send their changes through a queue |
| Accounts                        | a SQLite table (id, username, salt, hash), username indexed | finding a username does not read every account |
| Failed logins                   | a hash map (IP -> attempts, start of the minute), 4096 IPs at most | spoofed IPs cannot make it grow without end |
| Code of a private room          | a hash map (code -> party); `ROOM_CREATE` draws 6 digits until one is free | digits can be typed the same way on every keyboard layout |
| Time                            | fixed tick of 1/60 s, at most 10 late ticks caught up         | same game on every CPU; no burst of ticks after a pause      |
| Collisions                      | every pair is tested (engine)                                 | simplest, and fast enough for a few hundred entities         |
| Players                         | an array of 4 sessions, the index is the player id            | a free id is an empty slot                                   |

## Storage: SQLite
The player accounts are kept in a SQLite file (`rtype.db`, in the folder the server runs from). SQLite is one C file, built with CPM like the other dependencies, so nothing has to be installed, and the accounts are safe if the server stops in the middle of a write. A text file (JSON) would have to be read and written whole, and we would have to handle a write cut in the middle ourselves. A server database (PostgreSQL, MySQL) would need a service to install and run, for a few accounts.

Every query is a prepared statement: the username is never put in the SQL text, so it cannot change the query (SQL injection). The username is `UNIQUE COLLATE NOCASE`, so `Alice` and `alice` cannot be two accounts.

Nothing else is saved for now: no score, no save.

## Passwords: Argon2id with Monocypher
A password is never stored, only its hash and a random salt of 16 bytes per account. We use **Argon2id**, the winner of the Password Hashing Competition and the one OWASP advises first: it is slow on purpose and needs memory (19 MiB, 2 passes, the OWASP minimum), so trying millions of passwords on a stolen database costs a lot, even on a graphics card. A fast hash (SHA-256) can be tried billions of times per second, and without salt, two players with the same password would have the same hash.

We take Argon2 from **Monocypher**: like SQLite, it is one C file, small, audited, and it also has what encrypting the packets would need later (X25519, XChaCha20-Poly1305). libsodium does the same but is a lot bigger to build with CPM on Linux and Windows, and OpenSSL even more.

- The hashes are compared in constant time (`crypto_verify32`), so the time taken does not tell how many bytes are right.
- An unknown username is checked against a fake hash, so it takes the same time and gets the same answer (`bad credentials`) as a wrong password: the server does not tell which usernames exist.
- A hash takes tens of milliseconds, so it runs on a thread of its own (`AuthWorker`): the lobby keeps answering the other clients. At most 64 wait, and an IP gets 5 failed logins per minute, so someone who sends LOGIN in a loop cannot keep the thread busy.

## Security

| Threat                         | Handled | How                                                  |
| ------------------------------ | ------- | ---------------------------------------------------- |
| Malformed packet               | yes     | every field is checked, a bad packet is dropped      |
| Datagram too big               | yes     | more than 1200 bytes: dropped                        |
| Flood of packets               | yes     | at most 4096 packets wait for the game, more are dropped |
| Packet from an unknown address | yes     | only `LOGIN` and `REGISTER` are accepted             |
| Stolen database                | yes     | no password stored: Argon2id hash and salt only      |
| Trying many passwords          | yes     | slow hash, 5 failed logins per IP per minute         |
| Finding which usernames exist  | yes     | same answer and same time for an unknown username    |
| SQL injection                  | yes     | prepared statements only                             |
| Logins that block the server   | yes     | passwords checked on their own thread, 64 waiting at most |
| Too many clients or rooms      | yes     | 256 clients and 16 rooms at most, silent clients of the lobby dropped after 5 s |
| Room list used to flood someone | yes    | at most one `ROOM_LIST_REPLY` every 250 ms per client |
| Bug in one tick                | yes     | logged, and the server keeps running                 |
| Someone reads the packets      | no      | nothing is encrypted, **not even the password** in `LOGIN` and `REGISTER` |
| Fake sender address            | no      | UDP does not check who sends a packet                |
| Someone plays with another's account | partly | needs the password; one account plays once at a time, but someone who fakes the IP + port of a logged-in player can send packets as them |

To follow it over time: the server logs every dropped packet and every failed tick, the CI runs clang-tidy and the tests on every pull request, and every dependency has a fixed version, so updating one is a choice we make.

## Accessibility
For the game to be as accessible as possible, we plan:

- colors that colorblind players can tell apart, and/or different shapes;
- big enough and readable text;
- controls that each player can change;
- simple menus and no fast flashes (or the possibility to disable this).

## Documentation: MkDocs + Doxygen
MkDocs (Material theme) to have the documentation as a website with a menu, a search bar and diagrams, published by the CI, as the subject requires (it's modern !).
Doxygen to have the documentation of the code itself. 
