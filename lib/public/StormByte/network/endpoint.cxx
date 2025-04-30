#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/socket/client.hxx>

using namespace StormByte::Network;

EndPoint::EndPoint(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept:
m_protocol(protocol), m_handler(handler), m_logger(logger), m_status(Connection::Status::Disconnected), m_socket(nullptr) {}

EndPoint::~EndPoint() noexcept {
	Disconnect();
}

void EndPoint::Disconnect() noexcept {
	m_status.store(Connection::Status::Disconnecting);
	if (m_socket) {
		delete m_socket;
		m_socket = nullptr;
	}
	m_status.store(Connection::Status::Disconnected);
}

const Connection::Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}

Connection::Status EndPoint::Status() const noexcept {
	return m_status.load();
}

ExpectedVoid EndPoint::Handshake(Socket::Client&) noexcept {
	return {};
}

ExpectedVoid EndPoint::Authenticate(Socket::Client&) noexcept {
	return {};
}

ExpectedPacket EndPoint::Receive(Socket::Client& client, const PacketInstanceFunction& pif) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<PacketError>("Client is not connected");

	auto expected_packet = Packet::Read(
		pif,
		[&client](const size_t& size) -> ExpectedBuffer {
			auto expected_buffer = client.Receive(size);
			if (!expected_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_buffer.error()->what());
			return expected_buffer.value().get();
		}
	);
	if (!expected_packet)
		return StormByte::Unexpected<PacketError>(expected_packet.error());

	return expected_packet;
}

ExpectedVoid EndPoint::Send(Socket::Client& client, const Packet& packet) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is not connected");

	auto expected_write = client.Send(packet);
	if (!expected_write)
		return StormByte::Unexpected<ConnectionError>(expected_write.error()->what());

	return {};
}