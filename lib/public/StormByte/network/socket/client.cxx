#include <StormByte/network/socket/client.hxx>

#ifdef LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <iostream>
#include <format>

using namespace StormByte::Network::Socket;

Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler): Socket(protocol, handler) {}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	if (m_status != Connection::Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	m_status = Connection::Status::Connecting;

	auto expected_socket = CreateSocket();
	if (!expected_socket)
		return StormByte::Unexpected(expected_socket.error());

	m_handle = std::make_unique<Connection::Handler::Type>(expected_socket.value());

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
	if (!expected_conn_info)
		return StormByte::Unexpected<ConnectionError>(expected_conn_info.error()->what());

	m_conn_info = std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));
	InitializeMTU();

	#ifdef WINDOWS
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == SOCKET_ERROR) {
	#else
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == -1) {
	#endif
		return StormByte::Unexpected<ConnectionError>(m_conn_handler->LastError());
	}

	m_status = Connection::Status::Connected;

	return {};
}

void Client::ReadMode(const Network::Connection::ReadMode& mode) noexcept {
	if (!Connection::IsConnected(m_status))
		return;

	#ifdef LINUX
	int block_mode = (mode == Connection::ReadMode::Blocking) ? 0 : 1;
	ioctl(*m_handle, FIONBIO, &block_mode);
	#else
	u_long block_mode = (mode == Connection::ReadMode::Blocking) ? 0 : 1;
	ioctlsocket(*m_handle, FIONBIO, &block_mode);
	#endif
}
