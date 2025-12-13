#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/endpoint.hxx>

using namespace StormByte::Network;

Endpoint::Endpoint(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
	m_deserialize_packet_function(deserialize_packet_function),
	m_logger(logger) {}

PacketPointer Endpoint::Send(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	if (!SendPacket(client_connection, packet)) {
		m_logger << Logger::Level::Error << "Failed to send packet." << std::endl;
		return nullptr;
	}
	else {
		Transport::Frame response_frame = client_connection->Receive(m_logger);
		return response_frame.ProcessPacket(m_deserialize_packet_function, m_logger);
	}
}

bool Endpoint::Reply(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	return SendPacket(client_connection, packet);
}

std::shared_ptr<Connection::Client> Endpoint::CreateConnection(std::shared_ptr<Socket::Client> socket) noexcept {
	Buffer::Pipeline in_pipeline = InputPipeline();
	Buffer::Pipeline out_pipeline = OutputPipeline();
	return std::make_shared<Connection::Client>(socket, in_pipeline, out_pipeline);
}

bool Endpoint::SendPacket(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	if (!client_connection || !Connection::IsConnected(client_connection->Status())) {
		m_logger << Logger::Level::Error << "Cannot send packet: not connected." << std::endl;
		return false;
	}

	bool result = client_connection->Send(packet, m_logger);
	if (!result) {
		m_logger << Logger::Level::Error << "Failed to send packet." << std::endl;
		return false;
	}

	return true;
}