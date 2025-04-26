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
#include <format>
#include <future>
#include <thread>

constexpr const std::size_t BUFFER_SIZE = 4096;

using namespace StormByte::Network;

Socket::Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
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
			// Timeout: no readiness detected, so sleep a little and retry.
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

ExpectedVoid Socket::Client::Send(const Data::Packet& packet) noexcept {
	std::span<const std::byte> data = packet.Data();
	return Send(data);
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

ExpectedFutureBuffer Socket::Client::Receive() noexcept {
	return Receive(0);
}

ExpectedFutureBuffer Socket::Client::Receive(const std::size_t& max_size) noexcept {
	m_logger << Logger::Level::LowLevel << "Initiating receive operation with max_size: " << humanreadable_bytes << max_size << nohumanreadable << std::endl;

	try {
		auto promise = std::make_shared<PromisedBuffer>();
		auto future = promise->get_future();

		std::thread([this, promise, max_size]() {
			m_logger << Logger::Level::LowLevel << "Read thread started. Max size: " << humanreadable_bytes << max_size << nohumanreadable << std::endl;
			try {
				Read(*promise, max_size);
				m_logger << Logger::Level::LowLevel << "Read thread completed successfully." << std::endl;
			} catch (const std::exception& e) {
				m_logger << Logger::Level::Error << "Exception in Read thread: " << e.what() << std::endl;
				promise->set_exception(std::make_exception_ptr(
					ConnectionError("Exception during read: {}", e.what())));
			}
		}).detach();

		m_logger << Logger::Level::LowLevel << "Receive operation initiated successfully. Waiting for future..." << std::endl;
		return future;
	} catch (const std::exception& e) {
		m_logger << Logger::Level::Error << "Failed to initiate receive operation: " << e.what() << std::endl;
		return StormByte::Unexpected<ConnectionError>("Receive failed: {}", e.what());
	}
}

void Socket::Client::Read(PromisedBuffer& promise, std::size_t max_size) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting to read data with max_size: " << humanreadable_bytes << max_size << nohumanreadable << std::endl;

	Buffers::Simple buffer;
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
			buffer << std::string(internal_buffer, static_cast<std::size_t>(valread));
			total_bytes_read += valread;
			if (max_size > 0 && total_bytes_read >= max_size) {
				m_logger << Logger::Level::LowLevel << "Reached requested max_size: " << total_bytes_read << " bytes. Exiting loop." << std::endl;
				break;
			}
		} else if (valread == 0) {
			m_logger << Logger::Level::LowLevel << "Connection closed by peer. Exiting read loop." << std::endl;
			break;
		} else {
#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
#else
			if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
#endif
				m_logger << Logger::Level::LowLevel << "Socket would block, retrying..." << std::endl;
				if (max_size == 0 && total_bytes_read > 0) {
					m_logger << Logger::Level::LowLevel << "No additional data expected and socket would block. Exiting loop." << std::endl;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			} else {
				m_logger << Logger::Level::LowLevel << "Read error: " << m_conn_handler->LastError() << std::endl;
				promise.set_exception(std::make_exception_ptr(
					ConnectionError("Receive failed: {}", m_conn_handler->LastError())));
				return;
			}
		}
	}

	m_logger << Logger::Level::LowLevel << "Total data received: " << humanreadable_bytes << buffer.Size() << nohumanreadable << std::endl;
	promise.set_value(buffer);
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