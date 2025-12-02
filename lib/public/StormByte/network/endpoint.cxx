#include <StormByte/buffer/forwarder.hxx>
#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/system.hxx>

using namespace StormByte::Network;

EndPoint::EndPoint(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
m_protocol(protocol), m_codec(codec.Clone()), m_timeout(timeout), m_logger(logger), m_status(Connection::Status::Disconnected) {}

EndPoint::~EndPoint() noexcept {
	Disconnect();
}

void EndPoint::Disconnect() noexcept {
	m_logger << Logger::Level::LowLevel << "EndPoint::Disconnect: begin (uuid=" << m_self_uuid << ")" << std::endl;
	m_status.store(Connection::Status::Disconnecting);
	m_client_pmap.erase(m_self_uuid);
	m_in_pipeline_pmap.erase(m_self_uuid);
	m_out_pipeline_pmap.erase(m_self_uuid);
	m_status.store(Connection::Status::Disconnected);
	m_logger << Logger::Level::LowLevel << "EndPoint::Disconnect: end (uuid=" << m_self_uuid << ")" << std::endl;
}

const Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}

Connection::Status EndPoint::Status() const noexcept {
	return m_status.load();
}

ExpectedPacket EndPoint::Receive(const std::string& client_uuid) noexcept {
	m_logger << Logger::Level::LowLevel << "EndPoint::Receive: by uuid begin (uuid=" << client_uuid << ")" << std::endl;
	return Receive(std::static_pointer_cast<Socket::Client>(GetSocketByUUID(client_uuid)));
}

ExpectedPacket EndPoint::Receive(std::shared_ptr<Socket::Client> client) noexcept {
	m_logger << Logger::Level::LowLevel << "EndPoint::Receive: begin (uuid=" << (client ? client->UUID() : std::string("<null>")) << ")" << std::endl;
	// Create the socket reader function to encapsulate sockets away from final user
	Buffer::ExternalReadFunction read_func = [client, this](const std::size_t& size) noexcept -> Buffer::ExpectedData<Buffer::ReadError> {
		auto expected_data = client->Receive(size, m_timeout);
		if (!expected_data) {
			m_logger << Logger::Level::LowLevel << "EndPoint::Receive: socket Receive failed (uuid=" << client->UUID() << ") error=" << expected_data.error()->what() << std::endl;
			return StormByte::Unexpected<Buffer::ReadError>(expected_data.error()->what());
		}
		Buffer::FIFO& buffer = expected_data.value();
		auto data_vec_expected = buffer.Read(size);
		if (!data_vec_expected) {
			m_logger << Logger::Level::LowLevel << "EndPoint::Receive: buffer.Read failed (uuid=" << client->UUID() << ") error=" << data_vec_expected.error()->what() << std::endl;
			return StormByte::Unexpected<Buffer::ReadError>(data_vec_expected.error()->what());
		}
		return data_vec_expected.value();
	};

	// Create forwarder to read from socket
	std::shared_ptr<Buffer::Forwarder> forwarder = std::make_shared<Buffer::Forwarder>(read_func);

	// Create a producer based on the forwarder
	Buffer::Producer producer(forwarder);

	// Delegate packet creation to codec
	auto pkt = m_codec->Encode(producer.Consumer(), GetInPipelineByUUID(client->UUID()));
	if (!pkt) {
		m_logger << Logger::Level::LowLevel << "EndPoint::Receive: codec Encode failed (uuid=" << client->UUID() << ") error=" << pkt.error()->what() << std::endl;
		return pkt;
	}
	m_logger << Logger::Level::LowLevel << "EndPoint::Receive: decoded packet opcode=" << pkt.value()->Opcode() << " (uuid=" << client->UUID() << ")" << std::endl;
	return pkt;
}

ExpectedVoid EndPoint::Send(const std::string& client_uuid, const Packet& packet) noexcept {
	return Send(std::static_pointer_cast<Socket::Client>(GetSocketByUUID(client_uuid)), packet);
}

ExpectedVoid EndPoint::Send(std::shared_ptr<Socket::Client> client, const Packet& packet) noexcept {
	// Use the output pipeline when sending data, not the input pipeline
	auto processed_data = m_codec->Process(packet, GetOutPipelineByUUID(client->UUID()));
	if (!processed_data) {
		m_logger << Logger::Level::LowLevel << "EndPoint::Send: codec Process failed (uuid=" << client->UUID() << ") error=" << processed_data.error()->what() << std::endl;
		Disconnect();
		return StormByte::Unexpected<ConnectionError>(processed_data.error()->what());
	}
	// Extract ready-to-send bytes and log for diagnostics
	auto out_fifo = processed_data->ExtractUntilEoF();
	m_logger << Logger::Level::LowLevel << "Endpoint::Send: sending "
			<< Logger::humanreadable_bytes << out_fifo.Size() << Logger::nohumanreadable
			<< " to client UUID " << client->UUID() << std::endl;
	auto send_res = client->Send(out_fifo);
	if (!send_res) {
		m_logger << Logger::Level::LowLevel << "Endpoint::Send: client Send failed (uuid=" << client->UUID() << ") error=" << send_res.error()->what() << std::endl;
		return send_res;
	}
	m_logger << Logger::Level::LowLevel << "Endpoint::Send: send completed (uuid=" << client->UUID() << ")" << std::endl;
	return {};
}

std::shared_ptr<Socket::Socket> EndPoint::GetSocketByUUID(const std::string& uuid) noexcept {
	auto it = m_client_pmap.find(uuid);
	if (it != m_client_pmap.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<StormByte::Buffer::Pipeline> EndPoint::GetInPipelineByUUID(const std::string& uuid) noexcept {
	auto it = m_in_pipeline_pmap.find(uuid);
	if (it != m_in_pipeline_pmap.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<StormByte::Buffer::Pipeline> EndPoint::GetOutPipelineByUUID(const std::string& uuid) noexcept {
	auto it = m_out_pipeline_pmap.find(uuid);
	if (it != m_out_pipeline_pmap.end()) {
		return it->second;
	}
	return nullptr;
}