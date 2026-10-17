# Net

The net module sends and receives raw bytes over UDP, and gives the tools to
turn values into bytes and back. It knows nothing about the game protocol:
which packets exist and what they hold is up to the game. Asio stays hidden
inside the module, so the game only includes `<ampersand/net/UdpSocket.hpp>`,
`<ampersand/net/BinaryWriter.hpp>` and `<ampersand/net/BinaryReader.hpp>`.

## Concepts

| Name        | Kind  | What it does                                                                  |
| ----------- | ----- | ----------------------------------------------------------------------------- |
| `UdpSocket` | class | a **non-blocking** IPv4 UDP socket: no call waits for the network             |
| `Endpoint`  | struct | a peer on the network: numeric `address` (`"127.0.0.1"`) and `port`          |
| `NetError`  | error | thrown when a socket operation fails (port already used, unknown host...)     |
| `BinaryWriter` | class | appends integers, floats, enums and strings to a byte buffer, in big-endian |
| `BinaryReader` | class | reads them back; every read checks the size and returns `false` instead of going past the end |

| Method                    | What it does                                                    |
| ------------------------- | --------------------------------------------------------------- |
| `UdpSocket()`             | binds to a port picked by the system (clients)                  |
| `UdpSocket(port)`         | binds to the given port (servers)                               |
| `resolve(host, port)`     | turns `"localhost"` or `"127.0.0.1"` into an `Endpoint`         |
| `send(data, to)`          | sends one datagram (bytes or a string)                          |
| `receive(buffer, from)`   | reads one waiting datagram into `buffer`, fills `from`, returns its size, or `std::nullopt` if none is waiting |
| `localPort()`             | the port the socket is bound to                                 |

## Example

```cpp
namespace net = ampersand::net;

// Server
net::UdpSocket server(4242);
std::array<std::byte, 1024> buffer{};
net::Endpoint client;

while (running) {
    // once per frame: read every datagram that arrived since the last one
    while (const auto length = server.receive(buffer, client)) {
        handlePacket(std::span(buffer).first(*length), client);
        server.send("ok", client);
    }
    // ... update the game, send the snapshots
}

// Client
net::UdpSocket socket;
const net::Endpoint server = net::UdpSocket::resolve("localhost", 4242);
socket.send("hello", server);
```


## Writing and reading bytes

Values are written one by one in big-endian (network byte order), never with
a `memcpy` of a struct, so the bytes are the same on every compiler and
machine.

| Method                          | What it does                                                        |
| ------------------------------- | ------------------------------------------------------------------- |
| `write(value)`                  | any unsigned integer (`std::uint8_t` to `std::uint64_t`)            |
| `writeFloat(value)`             | a float, as its 4 IEEE 754 bytes                                    |
| `writeEnum(value)`              | an enum, as its underlying integer                                  |
| `writeString(value)`            | 1 byte of length, then the bytes; throws `NetError` past 255 bytes |
| `read(out)` / `readFloat(out)`  | `false` if not enough bytes are left; `readFloat` also refuses NaN and infinity |
| `readEnum(out, last)`           | `false` if the value is above `last`                                |
| `readString(out, maxLength)`    | `false` if the length is above `maxLength` or past the end          |
| `remaining()` / `atEnd()`       | bytes not read yet                                                  |

A failed read leaves `out` untouched and never throws, so a reader can be used
directly on bytes received from anyone:

```cpp
namespace net = ampersand::net;

std::vector<std::byte> buffer;
net::BinaryWriter writer(buffer);
writer.write(std::uint16_t{42});
writer.writeString("hello");

net::BinaryReader reader(buffer);
std::uint16_t id = 0;
std::string name;
if (!reader.read(id) || !reader.readString(name, 16) || !reader.atEnd()) {
    // too short, too long or invalid: drop the packet
}
```

## Limits

| Rule      | Value                                                                       |
| --------- | --------------------------------------------------------------------------- |
| Protocol  | UDP over IPv4 only                                                          |
| Datagrams | bytes beyond the size of `buffer` are lost: give a buffer as big as the biggest packet |
| Delivery  | none of UDP's limits are handled: packets can be lost, duplicated or arrive out of order |
