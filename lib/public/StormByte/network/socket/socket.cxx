#include <StormByte/network/socket/socket.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include <chrono>
#include <format>

constexpr const int SOCKET_BUFFER_SIZE = 65536;

using namespace StormByte::Network::Socket;

Socket::Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger::Log> logger) noexcept:
m_protocol(protocol), m_status(Connection::Status::Disconnected),
m_handle(nullptr), m_conn_handler(handler), m_conn_info(nullptr), m_mtu(DEFAULT_MTU), m_logger(logger) {}

Socket::Socket(Socket&& other) noexcept:
m_protocol(other.m_protocol), m_status(other.m_status),
m_handle(std::move(other.m_handle)), m_conn_handler(other.m_conn_handler),
m_conn_info(std::move(other.m_conn_info)), m_mtu(other.m_mtu) {
	other.m_status = Connection::Status::Disconnected;
}

Socket::~Socket() noexcept {
	Disconnect();
}

Socket& Socket::operator=(Socket&& other) noexcept {
	if (this != &other) {
		m_protocol = other.m_protocol;
		m_status = other.m_status;
		m_handle = std::move(other.m_handle);
		m_conn_handler = other.m_conn_handler;
		m_conn_info = std::move(other.m_conn_info);
		m_mtu = other.m_mtu;
		other.m_status = Connection::Status::Disconnected;
	}
	return *this;
}

void Socket::Disconnect() noexcept {
	if (m_status == Connection::Status::Disconnected)
		return;

	m_status = Connection::Status::Disconnecting;
	#ifdef LINUX
	shutdown(*m_handle, SHUT_RDWR);
	close(*m_handle);
	#else
	shutdown(*m_handle, SD_BOTH);
	closesocket(*m_handle);
	#endif
	m_handle.reset();

	m_status = Connection::Status::Disconnected;
	m_logger << Logger::Level::LowLevel << "Disconnected socket" << std::endl;
}

StormByte::Expected<StormByte::Network::Connection::Read::Result, StormByte::Network::ConnectionClosed> Socket::WaitForData(const long long& usecs) noexcept {
	if (!Connection::IsConnected(m_status)) {
		return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: Invalid connection status");
	}

	fd_set read_fds;
	struct timeval timeout;
	struct timeval* timeout_ptr = nullptr;

	// If timeout is specified, prepare timeout_ptr
	if (usecs > 0) {
		timeout.tv_sec = usecs / 1000000;
		timeout.tv_usec = usecs % 1000000;
		timeout_ptr = &timeout;
	}

	// Store the starting time
	auto start_time = std::chrono::steady_clock::now();

	while (true) {
		FD_ZERO(&read_fds);
		FD_SET(*m_handle, &read_fds);

		int select_result;

#ifdef WINDOWS
		select_result = select(0, &read_fds, nullptr, nullptr, timeout_ptr);
#else
		select_result = select(*m_handle + 1, &read_fds, nullptr, nullptr, timeout_ptr);
#endif

		if (select_result > 0) {
			// Check if our requested socket has data
			if (FD_ISSET(*m_handle, &read_fds)) {
				return Connection::Read::Result::Success; // Success, data available on our FD
			} else {
				// Data is available, but not on our requested FD
				// Check elapsed time to determine if we should continue
				auto elapsed_time = std::chrono::steady_clock::now() - start_time;
				auto elapsed_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count();

				if (usecs > 0 && elapsed_microseconds >= usecs) {
					return Connection::Read::Result::Timeout; // Timeout reached
				}
				// Otherwise, continue and retry
				continue;
			}
		} else if (select_result == 0) {
			// Timeout occurred
			return Connection::Read::Result::Timeout;
		} else {
			// Error occurred
#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAECONNRESET || m_conn_handler->LastErrorCode() == WSAENOTSOCK) {
#else
			if (m_conn_handler->LastErrorCode() == ECONNRESET || m_conn_handler->LastErrorCode() == EBADF) {
#endif
				return StormByte::Unexpected<ConnectionClosed>("Connection closed or invalid socket");
			} else {
				return StormByte::Unexpected<ConnectionClosed>(std::format("Failed to wait for data: {} (error code: {})",
																			m_conn_handler->LastError(),
																			m_conn_handler->LastErrorCode()));
			}
		}
	}

	return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: Unknown error occurred");
}

StormByte::Expected<StormByte::Network::Connection::Handler::Type, StormByte::Network::ConnectionError> Socket::CreateSocket() noexcept {
	auto protocol = m_protocol == Connection::Protocol::IPv4 ? AF_INET : AF_INET6;
	Connection::Handler::Type handle = ::socket(protocol, SOCK_STREAM, 0);
	#ifdef WINDOWS
	if (handle == INVALID_SOCKET) {
	#else
	if (handle == -1) {
	#endif
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected<ConnectionError>(m_conn_handler->LastError());
	}

	return handle;
}

void Socket::InitializeAfterConnect() noexcept {
	m_status = Connection::Status::Connected;
	m_mtu = GetMTU();
	SetNonBlocking();
	
	// Increase the socket buffer size
	//setsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, (const char*)&SOCKET_BUFFER_SIZE, sizeof(SOCKET_BUFFER_SIZE));
	setsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, (const char*)&SOCKET_BUFFER_SIZE, sizeof(SOCKET_BUFFER_SIZE));
}

#ifdef LINUX
int Socket::GetMTU() const noexcept {
if (!m_conn_info || !m_handle)
		return DEFAULT_MTU;

	int mtu;
	socklen_t optlen = sizeof(mtu);

	// Retrieve the MTU for the connected socket
	if (getsockopt(*m_handle, IPPROTO_IP, IP_MTU, &mtu, &optlen) >= 0)
		return mtu;
	else
		return DEFAULT_MTU;
}
#else
int Socket::GetMTU() const noexcept {
	// Ensure socket is connected is valid
	if (!m_conn_info || !m_handle)
		return DEFAULT_MTU; // Failure: Leave m_mtu as DEFAULT_MTU

	// Use IP Helper API to retrieve adapter info
	ULONG out_buf_len = 0;
	GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &out_buf_len); // Get required buffer size

	auto adapter_addresses = std::make_unique<BYTE[]>(out_buf_len);
	if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get()), &out_buf_len) != ERROR_SUCCESS) {
		return DEFAULT_MTU;
	}

	// Iterate through adapter addresses
	PIP_ADAPTER_ADDRESSES adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get());
	while (adapter) {
		// Check each unicast address for a match with m_sock_addr
		for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
			if (unicast->Address.lpSockaddr->sa_family == m_conn_info->SockAddr()->sa_family &&
				std::memcmp(unicast->Address.lpSockaddr, m_conn_info->SockAddr().get(), sizeof(sockaddr)) == 0) {
				return adapter->Mtu; // Success: Set the MTU
			}
		}
		adapter = adapter->Next; // Move to the next adapter
	}

	// Failure: Leave m_mtu as DEFAULT_MTU
	return DEFAULT_MTU;
}
#endif

void Socket::SetNonBlocking() noexcept {
	#ifdef WINDOWS
	u_long mode = 1; // Enable non-blocking mode
	ioctlsocket(*m_handle, FIONBIO, &mode);
	#else
	int flags = fcntl(*m_handle, F_GETFL, 0);
	fcntl(*m_handle, F_SETFL, flags | O_NONBLOCK);
	#endif
}