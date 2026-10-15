#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>

using ampersand::net::Endpoint;
using ampersand::net::NetError;
using ampersand::net::UdpSocket;

namespace {

// receive() never blocks, so poll until the datagram crosses the loopback.
std::optional<std::size_t> receiveWithin(UdpSocket& socket,
                                         std::span<std::byte> buffer,
                                         Endpoint& from) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        if (const auto length = socket.receive(buffer, from)) {
            return length;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

std::string receiveString(UdpSocket& socket, Endpoint& from) {
    std::array<std::byte, 1024> buffer{};
    const auto length = receiveWithin(socket, buffer, from);
    if (!length) {
        return {};
    }
    return {reinterpret_cast<const char*>(buffer.data()), *length};
}

}  // namespace

TEST(UdpSocket, DefaultConstructorBindsAnEphemeralPort) {
    const UdpSocket socket;
    EXPECT_NE(socket.localPort(), 0);
}

TEST(UdpSocket, ResolvesLocalhost) {
    const Endpoint endpoint = UdpSocket::resolve("localhost", 4242);
    EXPECT_EQ(endpoint.address, "127.0.0.1");
    EXPECT_EQ(endpoint.port, 4242);
    EXPECT_EQ(endpoint.toString(), "127.0.0.1:4242");
}

TEST(UdpSocket, HelloRoundTrip) {
    UdpSocket server;
    UdpSocket client;
    const Endpoint serverEndpoint =
        UdpSocket::resolve("127.0.0.1", server.localPort());

    client.send("hello", serverEndpoint);
    Endpoint sender;
    EXPECT_EQ(receiveString(server, sender), "hello");
    EXPECT_EQ(sender.port, client.localPort());

    server.send("ok", sender);
    Endpoint reply;
    EXPECT_EQ(receiveString(client, reply), "ok");
    EXPECT_EQ(reply, serverEndpoint);
}

TEST(UdpSocket, SendsRawBytes) {
    UdpSocket server;
    UdpSocket client;
    const std::array<std::byte, 3> data{
        std::byte{0x00},
        std::byte{0xFF},
        std::byte{0x2A},
    };

    client.send(data, {.address = "127.0.0.1", .port = server.localPort()});
    std::array<std::byte, 8> buffer{};
    Endpoint sender;
    ASSERT_EQ(receiveWithin(server, buffer, sender), data.size());
    EXPECT_EQ(buffer[0], data[0]);
    EXPECT_EQ(buffer[1], data[1]);
    EXPECT_EQ(buffer[2], data[2]);
}

TEST(UdpSocket, TruncatesDatagramsBiggerThanTheBuffer) {
    UdpSocket server;
    UdpSocket client;

    client.send("hello", {.address = "127.0.0.1", .port = server.localPort()});
    std::array<std::byte, 2> buffer{};
    Endpoint sender;
    EXPECT_EQ(receiveWithin(server, buffer, sender), buffer.size());
}

TEST(UdpSocket, ReceiveReturnsImmediatelyWhenNothingArrived) {
    UdpSocket socket;
    std::array<std::byte, 8> buffer{};
    Endpoint sender;
    EXPECT_EQ(socket.receive(buffer, sender), std::nullopt);
}

TEST(UdpSocket, IsMovable) {
    UdpSocket first;
    const auto port = first.localPort();
    UdpSocket second(std::move(first));
    EXPECT_EQ(second.localPort(), port);
}

TEST(UdpSocket, MovedFromSocketThrows) {
    UdpSocket first;
    const UdpSocket second(std::move(first));
    std::array<std::byte, 8> buffer{};
    Endpoint sender;

    // NOLINTBEGIN(bugprone-use-after-move): checking the moved-from state
    EXPECT_THROW((void)first.localPort(), NetError);
    EXPECT_THROW(first.send("hello", {.address = "127.0.0.1",
                                      .port = second.localPort()}),
                 NetError);
    EXPECT_THROW((void)first.receive(buffer, sender), NetError);
    // NOLINTEND(bugprone-use-after-move)
}

TEST(UdpSocket, MoveAssignmentKeepsTheSocket) {
    UdpSocket first;
    const auto port = first.localPort();
    UdpSocket second;
    second = std::move(first);
    EXPECT_EQ(second.localPort(), port);
}

TEST(UdpSocket, ThrowsWhenThePortIsTaken) {
    const UdpSocket first;
    EXPECT_THROW(UdpSocket{first.localPort()}, NetError);
}

TEST(UdpSocket, ThrowsOnInvalidAddress) {
    UdpSocket socket;
    EXPECT_THROW(
        socket.send("hello", {.address = "not an address", .port = 4242}),
        NetError);
}

TEST(UdpSocket, ThrowsOnUnknownHost) {
    EXPECT_THROW((void)UdpSocket::resolve("host.invalid", 4242), NetError);
}
