#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/socket/client.hxx>

#ifdef LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <span>
#include <cerrno>
#include <cstring>
#include <vector>
#include <thread>

constexpr const std::size_t BUFFER_SIZE = 65536;
// Optional safety cap for a single syscall to avoid extremely large syscalls
constexpr const std::size_t MAX_SINGLE_IO = 4 * 1024 * 1024; // 4 MiB

using namespace StormByte::Logger;
using namespace StormByte::Network;

Socket::Client::Client(const Protocol& protocol, Logger::ThreadedLog logger) noexcept
:Socket(protocol, logger) {}

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

	m_handle = std::make_unique<Connection::HandlerType>(expected_socket.value());

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol);
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
		m_logger << Logger::Level::Error << "Failed to connect: " << Connection::Handler::Instance().LastError() << std::endl;
		return StormByte::Unexpected<ConnectionError>(Connection::Handler::Instance().LastError());
	}

	InitializeAfterConnect();

	m_logger << Logger::Level::LowLevel << "Successfully connected to " << hostname << ":" << port << std::endl;

	return {};
}

ExpectedVoid Socket::Client::Send(const Buffer::FIFO& buffer) noexcept {
	auto expected_data = buffer.Read(0);
	if (!expected_data) {
		return StormByte::Unexpected<ConnectionError>(expected_data.error()->what());
	}
	auto& data_vec = expected_data.value();
	return Send(data_vec);
}

ExpectedVoid Socket::Client::Send(const std::vector<std::byte>& buffer) noexcept {
	return Send(std::span<const std::byte>(buffer.data(), buffer.size()));
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
		// Use poll() to avoid FD_SET/select limitations (fd may exceed FD_SETSIZE)
		struct pollfd pfd;
		pfd.fd = *m_handle;
		pfd.events = POLLOUT;
		int pol = poll(&pfd, 1, 50); // timeout 50ms
		if (pol < 0) {
			return StormByte::Unexpected<ConnectionError>(
							"Poll error: {} (error code: {})",
							Connection::Handler::Instance().LastError(), Connection::Handler::Instance().LastErrorCode());
		} else if (pol == 0) {
			// Timeout: no readiness detected, yield to other threads and retry.
			std::this_thread::yield();
			continue;
		} else {
			if (!(pfd.revents & POLLOUT)) {
				std::this_thread::yield();
				continue;
			}
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
							Connection::Handler::Instance().LastError(), Connection::Handler::Instance().LastErrorCode());
		} else if (sel == 0) {
			std::this_thread::yield();
			continue;
		}
#endif

		// Now that the socket is writable, send a chunk sized to the
		// effective send buffer (or a sensible fallback), capped to avoid
		// extremely large single syscalls.
		std::size_t chunk_capacity = BUFFER_SIZE;
		if (m_effective_send_buf > 0) {
			chunk_capacity = static_cast<std::size_t>(m_effective_send_buf);
		}
		chunk_capacity = std::min(chunk_capacity, MAX_SINGLE_IO);

		std::size_t chunk_size = std::min(chunk_capacity, data.size());
		std::span<const std::byte> chunk = data.subspan(0, chunk_size);

#ifdef LINUX
		const int send_flags = MSG_NOSIGNAL;
		const ssize_t written = ::send(*m_handle,
#else
		const int send_flags = 0;
		const int written = ::send(*m_handle,
#endif
						reinterpret_cast<const char*>(chunk.data()),
						chunk.size(), send_flags);

		if (written <= 0) {
			int sys_errno = errno;
			m_logger << Logger::Level::Error << "Send failed: " << Connection::Handler::Instance().LastError()
					 << " (code: " << Connection::Handler::Instance().LastErrorCode() << ")"
					 << " errno: " << sys_errno << " (" << Connection::Handler::Instance().ErrnoToString(sys_errno) << ")" << std::endl;
			return StormByte::Unexpected<ConnectionError>(
							"Failed to write: {} (error code: {})",
							Connection::Handler::Instance().LastError(), Connection::Handler::Instance().LastErrorCode());
		}

		total_bytes_sent += static_cast<std::size_t>(written);
		data = data.subspan(static_cast<std::size_t>(written));

		// Optional: add a brief sleep here to further throttle if needed
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_logger << Logger::Level::LowLevel << "All data sent successfully! Total bytes sent: " << humanreadable_bytes << total_bytes_sent << nohumanreadable << std::endl;

	return {};
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
		auto expected_send = Send(data_vec);
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
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
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
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN || Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
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
	std::size_t total_bytes_read = 0;

	while (true) {
		// Determine read buffer size based on effective recv buffer, fallback to BUFFER_SIZE
		std::size_t read_capacity = BUFFER_SIZE;
		if (m_effective_recv_buf > 0) {
			read_capacity = static_cast<std::size_t>(m_effective_recv_buf);
		}
		read_capacity = std::min(read_capacity, MAX_SINGLE_IO);

		const std::size_t bytes_to_read = (max_size > 0) ? std::min(read_capacity, max_size - total_bytes_read) : read_capacity;

		// Use a heap buffer so we don't stack-allocate very large arrays
		std::vector<char> internal_buffer(bytes_to_read);
#ifdef LINUX
	const ssize_t valread = recv(*m_handle, internal_buffer.data(), bytes_to_read, 0);
#else
	const int valread = recv(*m_handle, internal_buffer.data(), bytes_to_read, 0);
#endif

		if (valread > 0) {
			m_logger << Logger::Level::LowLevel << "Chunk received. Size: " << humanreadable_bytes << valread << nohumanreadable << std::endl;
			buffer.Write(std::string(internal_buffer.data(), static_cast<std::size_t>(valread)));
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
			if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
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
			if (Connection::Handler::Instance().LastErrorCode() == EAGAIN || Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
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
				m_logger << Logger::Level::LowLevel << "Read error: " << Connection::Handler::Instance().LastError() << std::endl;
				return StormByte::Unexpected<ConnectionError>("Receive failed: {}", Connection::Handler::Instance().LastError());
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

		// Chunk using effective send buffer if available
		std::size_t chunk_capacity = BUFFER_SIZE;
		if (m_effective_send_buf > 0) {
			chunk_capacity = static_cast<std::size_t>(m_effective_send_buf);
		}
		chunk_capacity = std::min(chunk_capacity, MAX_SINGLE_IO);

		std::size_t to_write = std::min(chunk_capacity, current_data.size());
		auto chunk = current_data.subspan(0, to_write);

#ifdef LINUX
		const int send_flags = MSG_NOSIGNAL;
		const ssize_t written = ::send(*m_handle,
#else
		const int send_flags = 0;
		const int written = ::send(*m_handle,
#endif
						reinterpret_cast<const char*>(chunk.data()),
						chunk.size(), send_flags);
		if (written <= 0) {
			int sys_errno = errno;
				m_logger << Logger::Level::Error << "Write failed: " << Connection::Handler::Instance().LastError()
					    << " (code: " << Connection::Handler::Instance().LastErrorCode() << ")"
					    << " errno: " << sys_errno << " (" << Connection::Handler::Instance().ErrnoToString(sys_errno) << ")" << std::endl;
			return StormByte::Unexpected<ConnectionError>(
				"Write failed: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode()
			);
		}
		total_written += static_cast<std::size_t>(written);
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
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
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
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN || Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
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