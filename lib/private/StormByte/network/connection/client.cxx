#include <StormByte/network/connection/client.hxx>

using namespace StormByte::Network::Connection;

Client::Client(std::shared_ptr<Socket::Client> socket, Buffer::Pipeline in_pipeline, Buffer::Pipeline out_pipeline) noexcept:
	m_socket(socket),
	m_in_pipeline(in_pipeline),
	m_out_pipeline(out_pipeline)
{}

bool Client::Send(const Transport::Frame& frame, std::shared_ptr<Logger::Log> logger) noexcept {
	ExpectedVoid result = m_socket->Send(frame.ProcessOutput(m_out_pipeline, logger));
	if (!result) {
		logger << Logger::Level::Error << "Failed to send frame to socket: " << result.error()->what();
		return false;
	}
	return true;
}

StormByte::Network::Transport::Frame Client::Receive(std::shared_ptr<Logger::Log> logger) noexcept {
	return Transport::Frame::ProcessInput(m_socket, m_in_pipeline, logger);
}