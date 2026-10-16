# Net

The net module sends and receives raw bytes over UDP. It knows nothing about
the game protocol: building and reading packets is up to the game. Asio stays
hidden inside the module, so the game only includes
`<ampersand/net/UdpSocket.hpp>`.

## Concepts

| Name        | Kind  | What it does                                                                  |
| ----------- | ----- | ----------------------------------------------------------------------------- |
| `UdpSocket` | class | a **non-blocking** IPv4 UDP socket: no call waits for the network             |
| `Endpoint`  | struct | a peer on the network: numeric `address` (`"127.0.0.1"`) and `port`          |
| `NetError`  | error | thrown when a socket operation fails (port already used, unknown host...)     |

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


## Limits

| Rule      | Value                                                                       |
| --------- | --------------------------------------------------------------------------- |
| Protocol  | UDP over IPv4 only                                                          |
| Datagrams | bytes beyond the size of `buffer` are lost: give a buffer as big as the biggest packet |
| Delivery  | none of UDP's limits are handled: packets can be lost, duplicated or arrive out of order |
