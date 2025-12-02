#include <StormByte/network/client.hxx>
#include <StormByte/network/socket/client.hxx>

using namespace StormByte::Network;

Client::Client(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
EndPoint(protocol, codec, timeout, logger) {}

ExpectedVoid Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	try {
		std::unique_ptr<Socket::Socket> self_socket = std::make_unique<Socket::Client>(m_protocol, m_logger);
		m_self_uuid = self_socket->UUID();
		m_client_pmap.emplace(m_self_uuid, std::move(self_socket));

		// Default pipelines
		m_in_pipeline_pmap.emplace(m_self_uuid, std::make_unique<Buffer::Pipeline>());
		m_out_pipeline_pmap.emplace(m_self_uuid, std::make_unique<Buffer::Pipeline>());
	}
	catch (const std::bad_alloc& e) {
		return StormByte::Unexpected<ConnectionError>("Can not create connection: {}", e.what());
	}

	m_status = Connection::Status::Connecting;
	auto expected_connect = static_cast<Socket::Client&>(*m_client_pmap.at(m_self_uuid)).Connect(hostname, port);
	if (!expected_connect) {
		m_client_pmap.erase(m_self_uuid);
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected(expected_connect.error());
	}
	m_status = Connection::Status::Connected;
	return {};
}

ExpectedPacket Client::Receive() noexcept {
	return EndPoint::Receive(m_self_uuid);
}

ExpectedVoid Client::Send(const Packet& packet) noexcept {
	return EndPoint::Send(m_self_uuid, packet);
}