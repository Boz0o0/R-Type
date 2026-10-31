# Network Protocol

This page explains how the client and the server talk to each other.
If you want to write your own client, everything you need should be here.

Quick facts:

- Transport: **UDP** only (IPv4)
- Default port: **4242**
- Binary protocol, current version: **3**
- Code: `include/rtype/common/protocol/` and `src/common/protocol/` (see [Implementation](implementation.md))

The server runs the game. The client only sends the player's inputs and draws
what the server tells it. If the client and the server disagree, the server
is right.

## How a game goes

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    C->>S: CONNECT_REQUEST(name)
    S-->>C: CONNECT_ACCEPT (the client is in the lobby)
    C->>S: ROOM_LIST
    S-->>C: ROOM_LIST_REPLY(public rooms)
    C->>S: ROOM_CREATE or ROOM_JOIN
    S-->>C: ROOM_JOINED(roomId, playerId)
    C->>S: ACK
    S-->>C: PLAYER_JOINED, ENTITY_SPAWN (current state)
    S-->>C: GAME_START
    loop every frame
        C->>S: PLAYER_INPUT(keys)
    end
    loop 30 times per second
        S-->>C: WORLD_SNAPSHOT(positions)
    end
    S-->>C: ENTITY_SPAWN / ENTITY_DESTROY / PLAYER_STATE
    S-->>C: GAME_OVER
    C->>S: ROOM_LEAVE
    S-->>C: ROOM_LEFT (back in the lobby)
```

The server has a **lobby** and **rooms** (we also call them parties). A
client first enters the lobby, then plays in one room at a time, and can go
back to the lobby to pick another one.

## Data types

All numbers are sent in **big-endian** (network byte order). There is no
padding between fields: we write them one after the other. Don't send your
C++ structs directly with `memcpy`, it won't work between different
compilers/OS.

| Type | Size | Notes |
| --- | --- | --- |
| `u8` | 1 | unsigned |
| `u16` | 2 | unsigned, big-endian |
| `u32` | 4 | unsigned, big-endian |
| `f32` | 4 | float (IEEE 754), sent like a `u32`. NaN and infinity are not allowed |
| `string` | 1 + n | one `u8` for the length, then n bytes of UTF-8 (no `\0` at the end) |
| `enum` | 1 | a `u8`, only the values listed below are valid |

Coordinates: the world is 1920 x 1080, (0, 0) is the top-left corner, x goes
right and y goes down. Speeds are in units per second. Entities can be
outside the screen (for example Bydos coming from the right).

## Packet header

Every packet starts with the same 12 bytes:

| Offset | Field | Type | What it is |
| --- | --- | --- | --- |
| 0 | `magic` | `u16` | always `0x5254` ("RT") |
| 2 | `version` | `u8` | protocol version, `3` for now |
| 3 | `type` | `u8` | packet type (see list below) |
| 4 | `flags` | `u8` | `0x01` = reliable, the other bits are unused |
| 5 | `reserved` | `u8` | unused, send 0 |
| 6 | `payloadSize` | `u16` | size of the data after the header |
| 8 | `sequence` | `u32` | packet counter of the sender |

Then come `payloadSize` bytes of data, depending on the type.

A full packet is at most **1200 bytes** (header included), so the data part is
at most 1188 bytes. We picked 1200 so packets don't get split on the
internet. One UDP datagram = one packet.

## Packet list

| Type | Name | Who sends it | Reliable | Data size |
| --- | --- | --- | --- | --- |
| `0x01` | [CONNECT_REQUEST](#connect_request-0x01) | client | no | 1 + n |
| `0x02` | [CONNECT_ACCEPT](#connect_accept-0x02) | server | no | 1 |
| `0x03` | [CONNECT_REJECT](#connect_reject-0x03) | server | no | 1 |
| `0x04` | [DISCONNECT](#disconnect-0x04) | both | no | 1 |
| `0x05` | [PING](#ping-0x05) | both | no | 4 |
| `0x06` | [PONG](#pong-0x06) | both | no | 4 |
| `0x07` | [ACK](#ack-0x07) | both | no | 4 |
| `0x10` | [PLAYER_INPUT](#player_input-0x10) | client | no | 5 |
| `0x20` | [GAME_START](#game_start-0x20) | server | yes | 4 |
| `0x21` | [PLAYER_JOINED](#player_joined-0x21) | server | yes | 2 + n |
| `0x22` | [PLAYER_LEFT](#player_left-0x22) | server | yes | 2 |
| `0x23` | [ENTITY_SPAWN](#entity_spawn-0x23) | server | yes | 22 |
| `0x24` | [ENTITY_DESTROY](#entity_destroy-0x24) | server | yes | 5 |
| `0x25` | [WORLD_SNAPSHOT](#world_snapshot-0x25) | server | no | 6 + 20 x n |
| `0x26` | [PLAYER_STATE](#player_state-0x26) | server | yes | 7 |
| `0x27` | [GAME_OVER](#game_over-0x27) | server | yes | 5 |
| `0x40` | [ROOM_LIST](#room_list-0x40) | client | no | 0 |
| `0x41` | [ROOM_CREATE](#room_create-0x41) | client | no | 1 |
| `0x42` | [ROOM_JOIN](#room_join-0x42) | client | no | 5 + n |
| `0x43` | [ROOM_LEAVE](#room_leave-0x43) | client | no | 0 |
| `0x50` | [ROOM_LIST_REPLY](#room_list_reply-0x50) | server | no | 1 + 5 x n |
| `0x51` | [ROOM_JOINED](#room_joined-0x51) | server | yes | 6 + n |
| `0x52` | [ROOM_REJECT](#room_reject-0x52) | server | no | 1 |
| `0x53` | [ROOM_LEFT](#room_left-0x53) | server | no | 0 |

`0x00` and every value not in this list are invalid. The reliable flag is set
on the "yes" packets and only on them.

The ranges: `0x01`-`0x0F` session (both ways), `0x10`-`0x1F` game from the
client, `0x20`-`0x3F` game from the server, `0x40`-`0x4F` lobby from the
client, `0x50`-`0x5F` lobby from the server.

### Enums

**EntityKind** (`rtype::common`, in `Kind.hpp`): `0` ship, `1` enemy, `2` player
missile, `3` enemy missile, `4` power-up, `5` obstacle.

**DeathReason** (`rtype::common`, in `Deaths.hpp`): `0` killed, `1` out of
screen, `2` collected.

**DisconnectReason**: `0` quit, `1` timeout, `2` kicked, `3` server shutdown.

**RejectReason**: `0` server full, `1` game already started, `2` wrong version,
`3` invalid name, `4` party full, `5` invalid party code, `6` room not found.

**RoomVisibility**: `0` public (in the room list, anyone can join it), `1`
private (hidden, joined with its code only).

**GameResult**: `0` victory, `1` defeat.

Player ids go from `0` to `3` (4 players max). `0xFF` means "no player".

### Connection packets

#### CONNECT_REQUEST (0x01)

Sent by the client to enter the lobby.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `playerName` | `string` | 1 to 16 bytes |

#### CONNECT_ACCEPT (0x02)

The client is in the lobby. Not reliable: the client sends CONNECT_REQUEST
again until it gets an answer, and the server answers every one.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `tickRate` | `u8` | server ticks per second (60), never 0 |

#### CONNECT_REJECT (0x03)

The server refused the client (`server full` when it already has 256
clients, `invalid name`, `wrong version`). The client stops trying.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `reason` | `RejectReason` | |

We will never change the header or this packet in future versions, so an old
client can still understand a "wrong version" answer.

#### DISCONNECT (0x04)

The client is leaving, or the server closes the connection.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `reason` | `DisconnectReason` | client sends "quit", server sends "kicked" or "server shutdown" |

#### PING (0x05)

Used to check the other side is still alive and to measure the lag.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `timestampMs` | `u32` | time of the sender in ms |

#### PONG (0x06)

Answer to a PING.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `timestampMs` | `u32` | same value as in the PING. Ping = now - timestampMs |

#### ACK (0x07)

Says "I got your reliable packet".

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `sequence` | `u32` | `sequence` of the packet we got |

### Client packets

#### PLAYER_INPUT (0x10)

The keys pressed by the player during one frame.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `clientTick` | `u32` | frame number of the client, always goes up |
| 4 | `inputMask` | `u8` | one bit per key, see below |

| Bit | Value | Key |
| --- | --- | --- |
| 0 | `0x01` | up |
| 1 | `0x02` | down |
| 2 | `0x04` | left |
| 3 | `0x08` | right |
| 4 | `0x10` | shoot |

The other bits must be 0, otherwise the packet is dropped.

### Server packets

#### GAME_START (0x20)

The game starts.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `serverTick` | `u32` | server tick when the game starts |

#### PLAYER_JOINED (0x21)

A player is in the game.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `playerId` | `u8` | 0 to 3 |
| 1 | `playerName` | `string` | 1 to 16 bytes |

#### PLAYER_LEFT (0x22)

A player left the game. When a client crashes, the others get this with the
reason "timeout".

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `playerId` | `u8` | 0 to 3 |
| 1 | `reason` | `DisconnectReason` | |

#### ENTITY_SPAWN (0x23)

A new entity appears: a ship, a Bydos, a missile, a power-up...

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `entityId` | `u32` | unique id, never reused in the same game |
| 4 | `entityType` | `EntityKind` | tells the client which sprite to use |
| 5 | `ownerId` | `u8` | player who owns it, or `0xFF` |
| 6 | `x` | `f32` | position |
| 10 | `y` | `f32` | |
| 14 | `velocityX` | `f32` | speed |
| 18 | `velocityY` | `f32` | |

The client uses `ownerId` to give each player ship a different color.

#### ENTITY_DESTROY (0x24)

An entity is removed.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `entityId` | `u32` | |
| 4 | `reason` | `DeathReason` | only "killed" shows an explosion |

#### WORLD_SNAPSHOT (0x25)

Positions and speeds of the entities at a given tick.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `serverTick` | `u32` | tick of this snapshot |
| 4 | `count` | `u16` | number of entities, 59 max |
| 6 | `entities` | list | `count` entries of 20 bytes |

Each entry:

| Offset | Field | Type |
| --- | --- | --- |
| 0 | `entityId` | `u32` |
| 4 | `x` | `f32` |
| 8 | `y` | `f32` |
| 12 | `velocityX` | `f32` |
| 16 | `velocityY` | `f32` |

`payloadSize` has to be exactly `6 + 20 x count`. If there are more than 59
entities, the server sends several snapshots with the same `serverTick`.

#### PLAYER_STATE (0x26)

Health, lives or score of a player changed.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `playerId` | `u8` | 0 to 3 |
| 1 | `health` | `u8` | 0 = ship destroyed |
| 2 | `lives` | `u8` | ships left, counting the one on screen (or coming back). 0 = out of the game |
| 3 | `score` | `u32` | 100 points per Bydos shot down |

#### GAME_OVER (0x27)

End of the game.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `result` | `GameResult` | |
| 1 | `finalScore` | `u32` | score of the team |

### Lobby packets

The client sends them only once it got CONNECT_ACCEPT. None of them is
reliable: the client sends a request again every 500 ms until the server
answers it, and the server answers every copy (see [Lobby and
rooms](#lobby-and-rooms)).

#### ROOM_LIST (0x40)

From the lobby: asks for the public rooms. No data. The server answers with
ROOM_LIST_REPLY, at most once every 250 ms per client (the extra requests get
no answer).

#### ROOM_CREATE (0x41)

From the lobby: opens a new room and joins it.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `visibility` | `RoomVisibility` | a private room gets a code of 6 digits, chosen by the server |

Refused with `server full` when the server already runs as many rooms as it
can (16 for ours).

#### ROOM_JOIN (0x42)

From the lobby: joins a room.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `roomId` | `u32` | a public room of the list, or 0 |
| 4 | `code` | `string` | 0 to 8 bytes |

`roomId` and `code` can't both be set (the packet is dropped).

- `code` set (1 to 8 ASCII letters and digits, letter case ignored): the
  **private** room of this code. If no room has this code yet, it is opened,
  so friends who all type the same code end up together. Players without the
  code never end up in it. The code is free again once its room is closed.
- `roomId` set: this **public** room.
- Neither (**quick match**): any public room with a free place, or a new one
  when they are all full.
- Refused with `party full` when the room already has 4 players, `room not
  found` when no public room has this id (it was closed, or it is private),
  `invalid party code` when the code has other characters, and `server full`
  when a new room is needed but the server already runs as many as it can.

#### ROOM_LEAVE (0x43)

From a room: leaves it and goes back to the lobby. No data. The others get
PLAYER_LEFT (reason "quit"). From the lobby, the server just answers
ROOM_LEFT again.

#### ROOM_LIST_REPLY (0x50)

The public rooms, oldest first. Private rooms are never listed.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `count` | `u8` | number of rooms, 32 max |
| 1 | `rooms` | list | `count` entries of 5 bytes |

Each entry:

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `roomId` | `u32` | to send in ROOM_JOIN, never 0 |
| 4 | `players` | `u8` | 0 to 4 |

`payloadSize` has to be exactly `1 + 5 x count`.

#### ROOM_JOINED (0x51)

The client is now in a room. Sent by the room itself, then come the
PLAYER_JOINED, ENTITY_SPAWN... of the game (see [Connection](#connection)).

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `roomId` | `u32` | never 0 |
| 4 | `playerId` | `u8` | id of this player in the room (0 to 3) |
| 5 | `code` | `string` | 0 to 8 bytes: the code of a private room, to give to friends; empty for a public one |

#### ROOM_REJECT (0x52)

The room request was refused. The client stays in the lobby.

| Offset | Field | Type | Notes |
| --- | --- | --- | --- |
| 0 | `reason` | `RejectReason` | |

#### ROOM_LEFT (0x53)

The client is back in the lobby: after ROOM_LEAVE, or after a room refused it.
No data.

## Reliable packets

UDP can lose packets, send them twice or in the wrong order. We have two kinds
of packets:

- **Normal packets** (inputs, snapshots, ping): we send them all the time, so
  if one is lost the next one replaces it. Nothing special to do.
- **Reliable packets** (flag `0x01`): important events (spawn, death,
  score...). They must arrive, so we use ACKs.

The `sequence` field: each side has its own counter and adds 1 for every new
packet it sends. When a reliable packet is sent again, it keeps the same
sequence.

**When you send a reliable packet:**

1. Keep it in a list.
2. If you didn't get an ACK after 100 ms, send it again (same bytes).
3. When the ACK arrives, remove it from the list.
4. After 20 tries without an ACK (about 2 seconds), consider the other side as
   disconnected.

**When you receive a reliable packet:**

1. Always send an ACK back, even if you already got this packet before (maybe
   your first ACK got lost).
2. Only handle it once. Keep the sequences you received in the last 5 seconds
   to spot the duplicates.

**Packets in the wrong order:**

- Snapshot older than the last one (smaller `serverTick`): ignore it. Same
  tick is fine (split snapshot).
- Input with a `clientTick` not bigger than the last one: ignore it.
- Snapshot entry for an entity the client doesn't know yet: ignore it, the
  ENTITY_SPAWN will arrive.
- ENTITY_DESTROY for an unknown entity: remember the id, and ignore the
  ENTITY_SPAWN if it comes later.
- PLAYER_STATE: only keep the one with the biggest `sequence`.

## Connection

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    C->>S: CONNECT_REQUEST("Alice")
    Note right of S: no answer? the client tries again after 500 ms
    C->>S: CONNECT_REQUEST("Alice")
    alt accepted
        S-->>C: CONNECT_ACCEPT
    else refused
        S-->>C: CONNECT_REJECT(server full)
    end
```

- The client sends CONNECT_REQUEST every 500 ms until it gets an answer. It
  gives up after 10 tries.
- The server knows a client by its **IP + port**. If the same IP + port sends
  CONNECT_REQUEST again from the lobby, the server sends CONNECT_ACCEPT
  again, it doesn't create a new client. If it is in a room, the server
  thinks the client restarted: it takes it out of its room, and accepts its
  next CONNECT_REQUEST.
- The server refuses when it already has 256 clients (`server full`), when
  the name is not valid UTF-8 or holds control characters (`invalid name`).
  If the version is different, it answers "wrong version".

## Lobby and rooms

```mermaid
sequenceDiagram
    participant C as Client
    participant L as Server (lobby)
    participant R as Server (room 3)
    C->>L: ROOM_LIST (every second)
    L-->>C: ROOM_LIST_REPLY([room 3: 2 players])
    C->>L: ROOM_JOIN(roomId=3)
    L->>R: adds the client
    R-->>C: ROOM_JOINED(roomId=3, playerId=2)
    C->>R: ACK
    R-->>C: PLAYER_JOINED for each player, ENTITY_SPAWN for each entity...
    Note over C,R: the game
    C->>R: ROOM_LEAVE
    R->>L: the client is back
    L-->>C: ROOM_LEFT
```

- A client in the **lobby** is in no room: it can ask for the room list,
  create a room or join one, and must keep talking (PING every second) or it
  is dropped after 5 seconds.
- A client in a **room** plays: every packet it sends goes to its room.
- Every request is sent again every 500 ms until it is answered, and the
  server answers every copy: a ROOM_CREATE or ROOM_JOIN that arrives once the
  client is in a room gets the same ROOM_JOINED again (no second room is
  opened), and a ROOM_LEAVE from the lobby gets ROOM_LEFT again.
- A client that gets ROOM_JOINED for a room it did not ask for anymore (late
  answer) sends ROOM_LEAVE.
- After ROOM_JOINED, the room sends the new client everything it needs: a
  PLAYER_JOINED per player (him included), an ENTITY_SPAWN per entity, a
  PLAYER_STATE per other player, and GAME_START if a game is running. The
  other clients get a PLAYER_JOINED, then everyone gets the ENTITY_SPAWN and
  the PLAYER_STATE of the new ship on the next tick. Our server lets players
  join a running game.
- A room is closed as soon as it has no player left (they all left with
  ROOM_LEAVE, quit or timed out).
- Each room has its own game: entity ids and server ticks start again from
  the beginning in every room. A client that changes room forgets the
  entities of the old one, and drops its game packets that still arrive.

## During the game

- The server sends GAME_START when the game of a room starts: as soon as its
  first player joins. A player who joins later gets it right away.
- The client sends PLAYER_INPUT every frame (at least 60 per second) and right
  away when a key changes. The server uses the last input of each player at
  each tick. Inputs sent before GAME_START are ignored.
- The server sends WORLD_SNAPSHOT 30 times per second. Between two snapshots,
  the client moves the entities with their speed so it looks smooth.
- Every event (spawn, missile fired, death, score change...) is sent to
  **all** the clients as a reliable packet.
- At the end the server sends GAME_OVER: "defeat" when every ship is lost
  (each player has 3 lives, a lost ship comes back after 2 seconds),
  "victory" when every wave of Bydos is beaten. Every entity left is
  destroyed (ENTITY_DESTROY), and 5 seconds later a new game starts with a
  new GAME_START.

## Disconnection and timeout

- A client that quits sends DISCONNECT(quit) a few times (it's not reliable),
  then closes.
- The server sends DISCONNECT(server shutdown) when it stops, or
  DISCONNECT(kicked) to remove a player.
- Both sides send a PING every second, and answer a PING with a PONG.
- Every valid packet counts as "still alive". If nothing comes from someone for
  **5 seconds**, he is disconnected, in the lobby as in a room.
- When a player leaves (ROOM_LEAVE, quit, timeout, kick), the server removes
  his ship with ENTITY_DESTROY and sends PLAYER_LEFT to the others. The game
  keeps going for them. That's how a crashed client is handled.

## Bad packets

When we receive something, we check it in this order. If a check fails, the
packet is **dropped**: no answer, no crash.

1. At least 12 bytes
2. At most 1200 bytes
3. `magic` is `0x5254`
4. `version` is 3 (if it's a CONNECT_REQUEST, the server can answer "wrong
   version")
5. `payloadSize` = packet size - 12
6. `type` is in the list
7. The data has exactly the right size, enums and ids are valid, floats are
   not NaN/infinity, strings are not too long
8. Right direction: the server drops server packets, the client drops client
   packets
9. Right sender: the server only accepts CONNECT_REQUEST from unknown IPs, the
   client only listens to the server

Sizes are always checked against the real number of bytes before allocating
anything. So a fake `count` or string length can't make us allocate a lot of
memory or read outside the buffer.

## Security

- There is no encryption and no login. The protocol is made for LAN games.
- Someone who can fake his IP could pretend to be another player.
- The answer to an unknown IP (CONNECT_ACCEPT or CONNECT_REJECT, 13 bytes)
  is always smaller than the request (at least 14 bytes), so the server can't
  be used for a DDoS amplification attack.
- ROOM_LIST_REPLY is bigger than ROOM_LIST, so the server sends it at most
  once every 250 ms to the same client: someone who fakes the address of a
  client can't use it to flood that address.
- Private rooms are hidden from the room list, but their code is not a
  password: someone who tries many codes can find one.
- The server accepts at most 500 packets per second from one sender (IP +
  port) and drops the rest. It counts per IP + port, not per IP, so that
  several players can play from the same machine; it counts at most 1024
  senders per second, so spoofed addresses can't make it use more memory.

## Constants

| Name | Value |
| --- | --- |
| `MAGIC` | `0x5254` |
| `VERSION` | `3` |
| `DEFAULT_PORT` | `4242` |
| `HEADER_SIZE` | 12 bytes |
| `MAX_DATAGRAM_SIZE` | 1200 bytes |
| `MAX_PAYLOAD_SIZE` | 1188 bytes |
| `MAX_PLAYERS` | 4 |
| `MAX_PLAYER_NAME_LENGTH` | 16 bytes |
| `MAX_PARTY_CODE_LENGTH` | 8 bytes |
| `MAX_LISTED_ROOMS` | 32 |
| `NO_OWNER` | `0xFF` |
| `MAX_SNAPSHOT_ENTITIES` | 59 |
| `SERVER_TICK_RATE` | 60 per second |
| `SNAPSHOT_RATE` | 30 per second |
| `RESEND_INTERVAL_MS` | 100 ms |
| `MAX_SEND_ATTEMPTS` | 20 |
| `PING_INTERVAL_MS` | 1000 ms |
| `TIMEOUT_MS` | 5000 ms |

## Examples

PLAYER_INPUT, sequence 42, frame 500, up + shoot:

```text
52 54 03 10 00 00 00 05 00 00 00 2A | 00 00 01 F4 11
magic ver type flags res size seq   | clientTick  keys
```

ENTITY_DESTROY of entity 1234 (killed), sequence 7, reliable:

```text
52 54 03 24 01 00 00 05 00 00 00 07 | 00 00 04 D2 00
```

The ACK the client sends back for it:

```text
52 54 03 07 00 00 00 04 00 00 00 2B | 00 00 00 07
```
