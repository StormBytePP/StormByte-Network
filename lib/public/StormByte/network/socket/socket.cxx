#include <StormByte/network/socket/socket.hxx>

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <vector>
#endif

#include <format>

using namespace StormByte::Network::Socket;

Socket::Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler):
m_protocol(protocol), m_status(Connection::Status::Disconnected),
m_handle(nullptr), m_conn_handler(handler), m_conn_info(nullptr), m_mtu(DEFAULT_MTU) {}

Socket::Socket(Socket&& other) noexcept:
m_protocol(other.m_protocol), m_status(other.m_status),
m_handle(std::move(other.m_handle)), m_conn_handler(other.m_conn_handler),
m_conn_info(std::move(other.m_conn_info)) {
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
		other.m_status = Connection::Status::Disconnected;
	}
	return *this;
}

void Socket::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status))
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

#ifdef LINUX
void Socket::InitializeMTU() noexcept {
	m_mtu = DEFAULT_MTU;

	if (!m_conn_info || !m_handle)
		return;

	int mtu;
	socklen_t optlen = sizeof(mtu);

	// Retrieve the MTU for the connected socket
	if (getsockopt(*m_handle, IPPROTO_IP, IP_MTU, &mtu, &optlen) >= 0)
		m_mtu = mtu;
	else
		m_mtu = DEFAULT_MTU;
}
#else
void Socket::InitializeMTU() noexcept {
	// Set the default MTU in case of failure
	m_mtu = DEFAULT_MTU;

	// Ensure socket is connected is valid
	if (!m_conn_info || !m_handle)
		return; // Failure: Leave m_mtu as DEFAULT_MTU

	// Use IP Helper API to retrieve adapter info
	ULONG out_buf_len = 0;
	GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &out_buf_len); // Get required buffer size

	auto adapter_addresses = std::make_unique<BYTE[]>(out_buf_len);
	if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get()), &out_buf_len) != ERROR_SUCCESS) {
		return; // Failure: Leave m_mtu as DEFAULT_MTU
	}

	// Iterate through adapter addresses
	PIP_ADAPTER_ADDRESSES adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get());
	while (adapter) {
		// Check each unicast address for a match with m_sock_addr
		for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
			if (unicast->Address.lpSockaddr->sa_family == m_conn_info->SockAddr()->sa_family &&
				std::memcmp(unicast->Address.lpSockaddr, m_conn_info->SockAddr().get(), sizeof(sockaddr)) == 0) {
				m_mtu = adapter->Mtu; // Success: Set the MTU
				return;
			}
		}
		adapter = adapter->Next; // Move to the next adapter
	}

	// Failure: Leave m_mtu as DEFAULT_MTU
}
#endif