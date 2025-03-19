#include <StormByte/network/socket/socket.hxx>

#ifdef LINUX
#include <unistd.h>
#endif

#include <format>

using namespace StormByte::Network::Socket;

Socket::Socket(const Address& address, std::shared_ptr<const Handler> handler): m_address(address), m_status(Status::Disconnected), m_handle(nullptr), m_handler(handler) {
	#ifdef WINDOWS
	m_wsaData = std::make_unique<WSADATA>();
	WSAStartup(MAKEWORD(2, 2), m_wsaData.get());
	#endif
}

Socket::Socket(Socket&& other) noexcept: m_address(std::move(other.m_address)), m_status(std::move(other.m_status)), m_handle(std::move(other.m_handle)), m_handler(std::move(other.m_handler)) {
	#ifdef WINDOWS
	m_wsaData = std::move(other.m_wsaData);
	#endif
}

Socket& Socket::operator=(Socket&& other) noexcept {
	if (this != &other) {
		m_address = std::move(other.m_address);
		m_status = std::move(other.m_status);
		m_handle = std::move(other.m_handle);
		#ifdef WINDOWS
		m_wsaData = std::move(other.m_wsaData);
		#endif
	}

	return *this;
}

Socket::~Socket() noexcept {
	Disconnect();
	#ifdef WINDOWS
	WSACleanup();
	#endif
}

StormByte::Expected<void, StormByte::Network::ConnectionError> Socket::Listen() noexcept {
	if (m_status != Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Socket is already connected");
	
	m_status = Status::Connecting;

	m_handle = std::make_unique<Handler::Type>(socket(AF_INET, SOCK_STREAM, 0));

	if (*m_handle == -1)
		return StormByte::Unexpected<ConnectionError>("Failed to create socket");

	bind(*m_handle, m_address.SockAddr(), sizeof(m_address.SockAddr()));
	
	const std::string error_string = m_handler->LastError();
	if (!error_string.empty())
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to bind socket: {}", error_string));

	m_status = Status::Connected;

	return {};
}

void Socket::Disconnect() noexcept {
	if (m_status == Status::Disconnected)
		return;

	m_status = Status::Disconnecting;
	#ifdef LINUX
	close(*m_handle);
	#else
	closesocket(*m_handle);
	#endif
	m_handle.reset();

	m_status = Status::Disconnected;
}

const StormByte::Network::Status& Socket::Status() const noexcept {
	return m_status;
}

const std::unique_ptr<const StormByte::Network::Handler::Type>& Socket::Handle() const noexcept {
	return m_handle;
}