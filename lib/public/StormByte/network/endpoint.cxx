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
	m_status.store(Connection::Status::Disconnecting);
	m_client_pmap.erase(m_self_uuid);
	m_in_pipeline_pmap.erase(m_self_uuid);
	m_out_pipeline_pmap.erase(m_self_uuid);
	m_status.store(Connection::Status::Disconnected);
}

const Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}

Connection::Status EndPoint::Status() const noexcept {
	return m_status.load();
}

ExpectedPacket EndPoint::Receive(const std::string& client_uuid) noexcept {
	return Receive(std::static_pointer_cast<Socket::Client>(GetSocketByUUID(client_uuid)));
}

ExpectedPacket EndPoint::Receive(std::shared_ptr<Socket::Client> client) noexcept {
	// Create the socket reader function to encapsulate sockets away from final user
	Buffer::ExternalReadFunction read_func = [client, this](const std::size_t& size) noexcept -> Buffer::ExpectedData<Buffer::ReadError> {
		auto expected_data = client->Receive(size, m_timeout);
		if (!expected_data) {
			return StormByte::Unexpected<Buffer::ReadError>(expected_data.error()->what());
		}
		Buffer::FIFO& buffer = expected_data.value();
		auto data_vec_expected = buffer.Read(size);
		if (!data_vec_expected) {
			return StormByte::Unexpected<Buffer::ReadError>(data_vec_expected.error()->what());
		}
		return data_vec_expected.value();
	};

	// Create forwarder to read from socket
	std::shared_ptr<Buffer::Forwarder> forwarder = std::make_shared<Buffer::Forwarder>(read_func);

	// Create a producer based on the forwarder
	Buffer::Producer producer(forwarder);

	// Delegate packet creation to codec
	return m_codec->Encode(producer.Consumer(), GetInPipelineByUUID(client->UUID()));
}

ExpectedVoid EndPoint::Send(const std::string& client_uuid, const Packet& packet) noexcept {
	return Send(std::static_pointer_cast<Socket::Client>(GetSocketByUUID(client_uuid)), packet);
}

ExpectedVoid EndPoint::Send(std::shared_ptr<Socket::Client> client, const Packet& packet) noexcept {
	auto processed_data = m_codec->Process(packet, GetInPipelineByUUID(client->UUID()));
	if (!processed_data) {
		Disconnect();
		return StormByte::Unexpected<ConnectionError>(processed_data.error()->what());
	}
	return client->Send(processed_data->ExtractUntilEoF());
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