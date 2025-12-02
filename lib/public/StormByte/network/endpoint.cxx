#include <StormByte/buffer/forwarder.hxx>
#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/system.hxx>

using namespace StormByte::Network;

EndPoint::EndPoint(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
m_protocol(protocol), m_codec(codec.Clone()), m_timeout(timeout), m_logger(logger), m_status(Connection::Status::Disconnected),
m_self_socket(nullptr) {}

EndPoint::~EndPoint() noexcept {
	Disconnect();
}

void EndPoint::Disconnect() noexcept {
	m_status.store(Connection::Status::Disconnecting);
	if (m_self_socket) {
		delete m_self_socket;
		m_self_socket = nullptr;
	}
	m_status.store(Connection::Status::Disconnected);
}

const Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}

Connection::Status EndPoint::Status() const noexcept {
	return m_status.load();
}

ExpectedPacket EndPoint::Receive(Socket::Client& socket) noexcept {
	// Create the socket reader function to encapsulate sockets away from final user
	Buffer::ExternalReadFunction read_func = [&socket, this](const std::size_t& size) noexcept -> Buffer::ExpectedData<Buffer::ReadError> {
		auto expected_data = socket.Receive(size, m_timeout);
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
	return m_codec->Encode(producer.Consumer());
}

ExpectedVoid EndPoint::Send(Socket::Client& socket, const Packet& packet) noexcept {
	return socket.Send(m_codec->Process(packet));
}