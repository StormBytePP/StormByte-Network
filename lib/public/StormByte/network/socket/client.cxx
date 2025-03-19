#include <StormByte/network/socket/client.hxx>

#ifdef LINUX
#include <unistd.h>
#include <cstring> // For strerror
#endif

#include <format>

using namespace StormByte::Network::Socket;

Client::Client(const Address& address, std::shared_ptr<const Handler> handler): Socket(address, handler) {}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Connect() noexcept {
	if (m_status != Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	m_status = Status::Connecting;

	m_handle = std::make_unique<Handler::Type>(socket(AF_INET, SOCK_STREAM, 0));

	std::string error_string = m_handler->LastError();
	if (!error_string.empty())
		return StormByte::Unexpected<ConnectionError>(error_string);

	connect(*m_handle, m_address.SockAddr(), sizeof(m_address.SockAddr()));
	error_string = m_handler->LastError();

	if (!error_string.empty()) {
		m_status = Status::Disconnected;
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to connect to socket: {}", error_string));
	}

	m_status = Status::Connected;

	return {};
}
