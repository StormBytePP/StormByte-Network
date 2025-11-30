#include <StormByte/network/socket/client.hxx>

#ifdef LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <span>
#include <thread>

constexpr const std::size_t BUFFER_SIZE = 65536;

using namespace StormByte::Logger;
using namespace StormByte::Network;

Socket::Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, Logger::ThreadedLog logger) noexcept
:Socket(protocol, handler, logger) {}

ExpectedVoid Socket::Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	m_logger << Logger::Level::LowLevel << "Connecting to " << hostname << ":" << port << std::endl;

	if (m_status != Connection::Status::Disconnected) {
		m_logger << Logger::Level::Error << "Client is already connected" << std::endl;
		return StormByte::Unexpected<ConnectionError>("Client is already connected");
	}

	m_status = Connection::Status::Connecting;

	auto expected_socket = CreateSocket();
	if (!expected_socket) {
		m_logger << Logger::Level::Error << "Failed to create socket: " << expected_socket.error()->what() << std::endl;
		return StormByte::Unexpected(expected_socket.error());
	}

	m_handle = std::make_unique<Connection::Handler::Type>(expected_socket.value());

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
	if (!expected_conn_info) {
		m_logger << Logger::Level::Error << "Failed to resolve host: " << expected_conn_info.error()->what() << std::endl;
		return StormByte::Unexpected<ConnectionError>(expected_conn_info.error()->what());
	}

	m_conn_info =
		std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));

#ifdef WINDOWS
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == SOCKET_ERROR) {
#else
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == -1) {
#endif
		m_logger << Logger::Level::Error << "Failed to connect: " << m_conn_handler->LastError() << std::endl;
		return StormByte::Unexpected<ConnectionError>(m_conn_handler->LastError());
	}

	InitializeAfterConnect();

	m_logger << Logger::Level::LowLevel << "Successfully connected to " << hostname << ":" << port << std::endl;

	return {};
}

ExpectedVoid Socket::Client::Send(std::span<const std::byte> data) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	if (!m_handle) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Invalid socket handle");
	}

	std::size_t total_bytes_sent = 0;

	while (!data.empty()) {
		// Wait for the socket to be writable
#ifdef LINUX
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(*m_handle, &writefds);
		timeval tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 50000; // wait up to 50ms for writability
		int sel = select(*m_handle + 1, nullptr, &writefds, nullptr, &tv);
		if (sel < 0) {
			return StormByte::Unexpected<ConnectionError>(
							"Select error: {} (error code: {})",
							m_conn_handler->LastError(), m_conn_handler->LastErrorCode());
		} else if (sel == 0) {
			// Timeout: no readiness detected, yield to other threads and retry.
			std::this_thread::yield();
			continue;
		}
#else // WINDOWS
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(*m_handle, &writefds);
		TIMEVAL tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 50000; // 50ms
		int sel = select(0, nullptr, &writefds, nullptr, &tv);
		if (sel == SOCKET_ERROR) {
			return StormByte::Unexpected<ConnectionError>(
							"Select error: {} (error code: {})",
							m_conn_handler->LastError(), m_conn_handler->LastErrorCode());
		} else if (sel == 0) {
			std::this_thread::yield();
			continue;
		}
#endif

		// Now that the socket is writable, send a chunk.
		std::size_t chunk_size = std::min(BUFFER_SIZE, data.size());
		std::span<const std::byte> chunk = data.subspan(0, chunk_size);

#ifdef LINUX
		const ssize_t written = ::send(*m_handle,
#else
		const int written = ::send(*m_handle,
#endif
										reinterpret_cast<const char*>(chunk.data()),
										chunk.size(), 0);

		if (written <= 0) {
			return StormByte::Unexpected<ConnectionError>(
							"Failed to write: {} (error code: {})",
							m_conn_handler->LastError(), m_conn_handler->LastErrorCode());
		}

		total_bytes_sent += written;
		data = data.subspan(written);

		// Optional: add a brief sleep here to further throttle if needed
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_logger << Logger::Level::LowLevel << "All data sent successfully! Total bytes sent: " << humanreadable_bytes << total_bytes_sent << nohumanreadable << std::endl;

	return {};
}

ExpectedVoid Socket::Client::Send(const Packet& packet) noexcept {
	m_logger << Logger::Level::LowLevel << "Sending packet with opcode: " << packet.Opcode() << std::endl;

	return Send(packet.Serialize());
}

ExpectedVoid Socket::Client::Send(Buffer::Consumer data) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	if (!m_handle) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Invalid socket handle");
	}

	while (data.IsWritable() || data.AvailableBytes() > 0) {
		if (data.AvailableBytes() == 0) {
			if (!data.IsWritable()) {
				break;
			}
			m_logger << Logger::Level::LowLevel << "No data available to send. Yielding..." << std::endl;
			std::this_thread::yield();
			continue;
		}
		auto send_bytes = data.AvailableBytes();
		auto expected_data = data.Read(send_bytes);
		if (!expected_data) {
			return StormByte::Unexpected<ConnectionError>(expected_data.error()->what());
		}
		auto& data_vec = expected_data.value();
		auto expected_send = Send(std::span<const std::byte>(data_vec.data(), data_vec.size()));
		if (!expected_send) {
			return StormByte::Unexpected<ConnectionError>(expected_send.error()->what());
		}
	}

	return {};
}

bool Socket::Client::HasShutdownRequest() noexcept {
	char buffer[1]; // Inspect only one byte
#ifdef LINUX
	ssize_t result = ::recv(*m_handle, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
#else
	int result = ::recv(*m_handle, buffer, sizeof(buffer), MSG_PEEK);
	if (result == SOCKET_ERROR) {
		if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
			// No data available; not a shutdown
			return false;
		} else {
			// Treat other errors as a shutdown request
			return true;
		}
	}
#endif

	if (result == 0) {
		// Connection closed by peer
		return true;
	} else if (result < 0) {
#ifdef LINUX
		if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
			// No data available; not a shutdown
			return false;
		} else {
			// Treat other errors as a shutdown request
			return true;
		}
#endif
	}

	// Data is available; not a shutdown
	return false;
}

ExpectedBuffer Socket::Client::Receive() noexcept {
	return Receive(0);
}

ExpectedBuffer Socket::Client::Receive(const std::size_t& max_size) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting to read data with max_size: " << humanreadable_bytes << max_size << nohumanreadable << std::endl;

	if (!m_handle) {
		return StormByte::Unexpected<ConnectionError>("Receive failed: Invalid socket handle");
	}

	Buffer::FIFO buffer;
	char internal_buffer[BUFFER_SIZE];
	std::size_t total_bytes_read = 0;

	while (true) {
		const std::size_t bytes_to_read = (max_size > 0) ? std::min(BUFFER_SIZE, max_size - total_bytes_read) : BUFFER_SIZE;
#ifdef LINUX
		const ssize_t valread = recv(*m_handle, internal_buffer, bytes_to_read, 0);
#else
		const int valread = recv(*m_handle, internal_buffer, bytes_to_read, 0);
#endif

		if (valread > 0) {
			m_logger << Logger::Level::LowLevel << "Chunk received. Size: " << humanreadable_bytes << valread << nohumanreadable << std::endl;
			buffer.Write(std::string(internal_buffer, static_cast<std::size_t>(valread)));
			total_bytes_read += valread;
			if (max_size > 0 && total_bytes_read >= max_size) {
				m_logger << Logger::Level::LowLevel << "Reached requested max_size: " << humanreadable_bytes << total_bytes_read << nohumanreadable << ". Exiting loop." << std::endl;
				break;
			}
		} else if (valread == 0) {
			m_logger << Logger::Level::LowLevel << "Connection closed by peer. Exiting read loop." << std::endl;
			break;
		} else {
#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
				// On Windows, fall through to a short wait using select via WaitForData
				auto wait_res = WaitForData(100000); // 100ms
				if (!wait_res) {
					// Treat as connection closed/error
					break;
				}
				if (wait_res.value() == Connection::Read::Result::Timeout) {
					if (max_size == 0 && total_bytes_read > 0) {
						break;
					}
					continue;
				}
#else
			if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
				// Socket would block: wait efficiently for data instead of busy-looping and logging repeatedly
				auto wait_res = WaitForData(100000); // 100ms
				if (!wait_res) {
					// Connection closed or error
					break;
				}
				if (wait_res.value() == Connection::Read::Result::Timeout) {
					if (max_size == 0 && total_bytes_read > 0) {
						break;
					}
					continue;
				}
#endif
			} else {
				m_logger << Logger::Level::LowLevel << "Read error: " << m_conn_handler->LastError() << std::endl;
				return StormByte::Unexpected<ConnectionError>("Receive failed: {}", m_conn_handler->LastError());
			}
		}
	}

	m_logger << Logger::Level::LowLevel << "Total data received: " << humanreadable_bytes << buffer.Size() << nohumanreadable << std::endl;
	return buffer;
}

ExpectedVoid Socket::Client::Write(std::span<const std::byte> data, const std::size_t& size) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting to write data..." << std::endl;

	if (m_status != Connection::Status::Connected) {
		m_logger << Logger::Level::LowLevel << "Failed to write: Client is not connected" << std::endl;
		return StormByte::Unexpected<ConnectionError>("Failed to write: Client is not connected");
	}

	// If the passed-in size is larger than the span, adjust accordingly.
	std::size_t bytes_to_write = std::min(size, data.size());
	std::size_t total_written = 0;

	while (total_written < bytes_to_write) {
		auto current_data = data.subspan(total_written);
#ifdef LINUX
		const ssize_t written = ::send(*m_handle,
#else
		const int written = ::send(*m_handle,
#endif
										reinterpret_cast<const char*>(current_data.data()),
										current_data.size(), 0);
		if (written <= 0) {
			m_logger << Logger::Level::Error << "Write failed: " << m_conn_handler->LastError()
					<< " (code: " << m_conn_handler->LastErrorCode() << ")" << std::endl;
			return StormByte::Unexpected<ConnectionError>(
				"Write failed: {} (error code: {})",
				m_conn_handler->LastError(),
				m_conn_handler->LastErrorCode()
			);
		}
		total_written += written;
	}

	m_logger << Logger::Level::LowLevel << "Write of size " << humanreadable_bytes << bytes_to_write << nohumanreadable << " bytes completed successfully" << std::endl;
	return {};
}

bool Socket::Client::Ping() noexcept {
	if (m_status != Connection::Status::Connected) {
		return false;
	}
	bool ping_success = false;
	char buffer[1]; // Inspect only one byte
#ifdef LINUX
	ssize_t result = ::recv(*m_handle, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
#else
	int result = ::recv(*m_handle, buffer, sizeof(buffer), MSG_PEEK);
	if (result == SOCKET_ERROR) {
		if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
			// No data available; not a shutdown
			ping_success = true;
		}
	}
#endif
	if (result > 0) {
		// Data is available; connection is still alive
		ping_success = true;
	}
	else if (result == 0) {
		// Connection closed by peer
		ping_success = false;
	} else {
#ifdef LINUX
		if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
			// No data available; connection is still alive
			ping_success = true;
		} else {
			// Treat other errors as a connection closed
			ping_success = false;
		}
#endif
	}

	// Data is available; not a shutdown
	if (ping_success) {
		m_logger << Logger::Level::LowLevel << "Ping successful" << std::endl;
	} else {
		m_logger << Logger::Level::LowLevel << "Ping failed" << std::endl;
		m_status = Connection::Status::Disconnected;
	}

	return ping_success;
}