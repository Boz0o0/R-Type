#include <ampersand/net/UdpSocket.hpp>
#include <asio.hpp>

namespace ampersand::net {

namespace {

asio::ip::udp::endpoint toAsio(const Endpoint& endpoint) {
    asio::error_code error;
    const auto address = asio::ip::make_address_v4(endpoint.address, error);
    if (error) {
        throw NetError("invalid address '" + endpoint.address +
                       "': " + error.message());
    }
    return {address, endpoint.port};
}

Endpoint fromAsio(const asio::ip::udp::endpoint& endpoint) {
    return {.address = endpoint.address().to_string(), .port = endpoint.port()};
}

}  // namespace

struct UdpSocket::Impl {
    asio::io_context context;
    asio::ip::udp::socket socket{context};
};

UdpSocket::UdpSocket() : UdpSocket(0) {}

UdpSocket::UdpSocket(std::uint16_t port) : _impl(std::make_unique<Impl>()) {
    try {
        this->_impl->socket.open(asio::ip::udp::v4());
        this->_impl->socket.bind({asio::ip::udp::v4(), port});
        this->_impl->socket.non_blocking(true);
    } catch (const asio::system_error& e) {
        throw NetError("cannot bind UDP port " + std::to_string(port) + ": " +
                       e.what());
    }
}

UdpSocket::~UdpSocket() = default;
UdpSocket::UdpSocket(UdpSocket&&) noexcept = default;
UdpSocket& UdpSocket::operator=(UdpSocket&&) noexcept = default;

UdpSocket::Impl& UdpSocket::impl() const {
    if (!this->_impl) {
        throw NetError("socket has been moved from");
    }
    return *this->_impl;
}

Endpoint UdpSocket::resolve(const std::string& host, std::uint16_t port) {
    asio::io_context context;
    asio::ip::udp::resolver resolver(context);
    asio::error_code error;
    const auto results = resolver.resolve(asio::ip::udp::v4(), host,
                                          std::to_string(port), error);
    if (error || results.empty()) {
        throw NetError("cannot resolve '" + host + "': " + error.message());
    }
    return fromAsio(*results.begin());
}

void UdpSocket::send(std::span<const std::byte> data, const Endpoint& target) {
    asio::error_code error;
    this->impl().socket.send_to(asio::buffer(data.data(), data.size()),
                                toAsio(target), 0, error);
    if (error && error != asio::error::would_block) {
        throw NetError("cannot send to " + target.toString() + ": " +
                       error.message());
    }
}

void UdpSocket::send(std::string_view data, const Endpoint& target) {
    this->send(std::as_bytes(std::span(data)), target);
}

std::optional<std::size_t> UdpSocket::receive(std::span<std::byte> buffer,
                                              Endpoint& from) {
    asio::ip::udp::endpoint sender;
    asio::error_code error;
    const std::size_t length = this->impl().socket.receive_from(
        asio::buffer(buffer.data(), buffer.size()), sender, 0, error);
    if (error == asio::error::would_block) {
        return std::nullopt;
    }
    if (error && error != asio::error::message_size) {
        throw NetError("cannot receive: " + error.message());
    }
    from = fromAsio(sender);
    return length;
}

std::uint16_t UdpSocket::localPort() const {
    asio::error_code error;
    const auto endpoint = this->impl().socket.local_endpoint(error);
    if (error) {
        throw NetError("cannot read local port: " + error.message());
    }
    return endpoint.port();
}

}  // namespace ampersand::net
