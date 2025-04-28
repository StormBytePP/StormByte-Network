#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/socket/socket.hxx>

using namespace StormByte::Network;

EndPoint::EndPoint(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept:
m_protocol(protocol), m_handler(handler), m_logger(logger), m_status(Connection::Status::Disconnected), m_socket(nullptr) {}

EndPoint::~EndPoint() noexcept {
	if (m_socket) {
		delete m_socket;
		m_socket = nullptr;
	}
}

const Connection::Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}