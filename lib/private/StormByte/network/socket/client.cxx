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

// Hard cap per syscall — never allocate/send 200 MiB in one go.
constexpr std::size_t MAX_SINGLE_IO     = 4 * 1024 * 1024; // 4 MiB
constexpr std::size_t DEFAULT_IO_CHUNK  = 64 * 1024;        // 64 KiB fallback

using namespace StormByte::Logger;
using namespace StormByte::Network;

namespace {
	std::size_t ClampChunk(std::size_t preferred, std::size_t remaining) noexcept {
		if (preferred == 0)
			preferred = DEFAULT_IO_CHUNK;
		preferred = std::min(preferred, MAX_SINGLE_IO);
		if (remaining > 0)
			preferred = std::min(preferred, remaining);
		return std::max<std::size_t>(preferred, 1);
	}
}

Socket::Client::Client(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept
:Socket(protocol, logger) {
	m_logger << Logger::Level::LowLevel << "Created client socket with UUID: " << m_UUID << std::endl;
}

ExpectedVoid Socket::Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	m_logger << Logger::Level::LowLevel << "Connecting to " << hostname << ":" << port << std::endl;

	if (m_status.load(std::memory_order_acquire) != Connection::Status::Disconnected) {
		m_logger << Logger::Level::Error << "Client is already connected" << std::endl;
		return Unexpected<ConnectionError>("Client is already connected");
	}

	m_status.store(Connection::Status::Connecting, std::memory_order_release);

	auto expected_socket = CreateSocket();
	if (!expected_socket) {
		m_logger << Logger::Level::Error << "Failed to create socket: " << expected_socket.error()->what() << std::endl;
		return Unexpected(expected_socket.error());
	}

	m_handle = expected_socket.value();

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol);
	if (!expected_conn_info) {
		m_logger << Logger::Level::Error << "Failed to resolve host: " << expected_conn_info.error()->what() << std::endl;
		return Unexpected<ConnectionError>(expected_conn_info.error()->what());
	}

	m_conn_info =
		std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));

#ifdef WINDOWS
	if (::connect(m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == SOCKET_ERROR) {
#else
	if (::connect(m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == -1) {
#endif
		m_logger << Logger::Level::Error << "Failed to connect: " << Connection::Handler::Instance().LastError() << std::endl;
		return Unexpected<ConnectionError>(Connection::Handler::Instance().LastError());
	}

	InitializeAfterConnect();

	m_logger << Logger::Level::LowLevel << "Successfully connected to " << hostname << ":" << port << std::endl;

	return {};
}

ExpectedVoid Socket::Client::Send(const Buffer::FIFO& buffer) noexcept {
	return Send(std::span<const std::byte>(buffer.Data().data(), buffer.Size()));
}

ExpectedVoid Socket::Client::Send(const std::vector<std::byte>& buffer) noexcept {
	return Send(std::span<const std::byte>(buffer.data(), buffer.size()));
}

ExpectedVoid Socket::Client::Send(std::span<const std::byte> data) noexcept {
	if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected) {
		return Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	if (!m_handle) {
		return Unexpected<ConnectionError>("Failed to send: Invalid socket handle");
	}

	std::size_t total_bytes_sent = 0;
	const std::size_t preferred = (m_effective_send_buf > 0)
		? static_cast<std::size_t>(m_effective_send_buf)
		: DEFAULT_IO_CHUNK;

	while (!data.empty()) {
#ifdef LINUX
		struct pollfd pfd;
		pfd.fd = m_handle;
		pfd.events = POLLOUT;
		int pol = poll(&pfd, 1, 50); // 50ms
		if (pol < 0) {
			return Unexpected<ConnectionError>(
				"Poll error: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode());
		} else if (pol == 0) {
			continue; // wait again — no busy yield
		} else if (!(pfd.revents & POLLOUT)) {
			continue;
		}
#else // WINDOWS
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(m_handle, &writefds);
		TIMEVAL tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 50000; // 50ms
		int sel = select(0, nullptr, &writefds, nullptr, &tv);
		if (sel == SOCKET_ERROR) {
			return Unexpected<ConnectionError>(
				"Select error: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode());
		} else if (sel == 0) {
			continue;
		}
#endif

		const std::size_t chunk_size = ClampChunk(preferred, data.size());
		std::span<const std::byte> chunk = data.subspan(0, chunk_size);

#ifdef LINUX
		const int send_flags = MSG_NOSIGNAL;
		const ssize_t written = ::send(m_handle,
#else
		const int send_flags = 0;
		const int written = ::send(m_handle,
#endif
			reinterpret_cast<const char*>(chunk.data()),
			static_cast<int>(chunk.size()), send_flags);

		if (written <= 0) {
#ifdef WINDOWS
			const int wsa = Connection::Handler::Instance().LastErrorCode();
			if (wsa == WSAEWOULDBLOCK) {
				continue; // not ready; wait again
			}
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
#endif
			int sys_errno = errno;
			m_logger << Logger::Level::Error << "Send failed: " << Connection::Handler::Instance().LastError()
					<< " (code: " << Connection::Handler::Instance().LastErrorCode() << ")"
					<< " errno: " << sys_errno << " (" << Connection::Handler::Instance().ErrnoToString(sys_errno) << ")" << std::endl;
			return Unexpected<ConnectionError>(
				"Failed to write: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode());
		}

		total_bytes_sent += static_cast<std::size_t>(written);
		data = data.subspan(static_cast<std::size_t>(written));
	}

	m_logger << Logger::Level::LowLevel << "All data sent successfully! Total bytes sent: "
			<< humanreadable_bytes << total_bytes_sent << nohumanreadable << std::endl;

	return {};
}

ExpectedVoid Socket::Client::Send(Buffer::Consumer data) noexcept {
	if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected) {
		return Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	if (!m_handle) {
		return Unexpected<ConnectionError>("Failed to send: Invalid socket handle");
	}

	// Block on buffer semantics (Extract(0) waits for data or EoF) — no yield spin.
	while (true) {
		Buffer::DataType byte_data;
		if (!data.Extract(0, byte_data) || byte_data.empty()) {
			if (data.EoF())
				break;
			continue;
		}

		auto expected_send = Send(std::span<const std::byte>(byte_data.data(), byte_data.size()));
		if (!expected_send) {
			return Unexpected<ConnectionError>(expected_send.error()->what());
		}
	}

	return {};
}

bool Socket::Client::HasShutdownRequest() noexcept {
	char buffer[1];
#ifdef LINUX
	ssize_t result = ::recv(m_handle, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
#else
	int result = ::recv(m_handle, buffer, sizeof(buffer), MSG_PEEK);
	if (result == SOCKET_ERROR) {
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
			return false;
		} else {
			return true;
		}
	}
#endif

	if (result == 0) {
		return true;
	} else if (result < 0) {
#ifdef LINUX
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN ||
			Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
			return false;
		} else {
			return true;
		}
#endif
	}

	return false;
}

ExpectedBuffer Socket::Client::Receive(const std::size_t& max_size) noexcept {
	return Receive(max_size, 0);
}

ExpectedBuffer Socket::Client::Peek(const std::size_t& size) const noexcept {
	return const_cast<Client*>(this)->ReadOnce(size, MSG_PEEK);
}

ExpectedBuffer Socket::Client::ReadOnce(const std::size_t& size, int flags) noexcept {
	if (size == 0) {
		return Unexpected<ConnectionError>("Read failed: size must be greater than 0");
	}

	if (!m_handle) {
		return Unexpected<ConnectionError>("Read failed: Invalid socket handle");
	}

	const std::size_t preferred = (m_effective_recv_buf > 0)
		? static_cast<std::size_t>(m_effective_recv_buf)
		: DEFAULT_IO_CHUNK;
	const std::size_t bytes_to_read = ClampChunk(preferred, size);

	std::vector<char> internal_buffer(bytes_to_read);
#ifdef LINUX
	const ssize_t valread = ::recv(m_handle, internal_buffer.data(), bytes_to_read, flags);
#else
	const int valread = ::recv(m_handle, internal_buffer.data(), static_cast<int>(bytes_to_read), flags);
#endif

	if (valread > 0) {
		Buffer::FIFO buffer;
		(void)buffer.Write(std::span<const std::byte>(
			reinterpret_cast<const std::byte*>(internal_buffer.data()),
			static_cast<std::size_t>(valread)));
		return buffer;
	} else if (valread == 0) {
		return Unexpected<ConnectionError>("Read failed: connection closed by peer");
	} else {
#ifdef LINUX
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN ||
			Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
			return Unexpected<ConnectionError>("Read would block: no data available");
		}
#else
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
			return Unexpected<ConnectionError>("Read would block: no data available");
		}
#endif
		return Unexpected<ConnectionError>("Read failed: {}", Connection::Handler::Instance().LastError());
	}
}

ExpectedBuffer Socket::Client::Receive(const std::size_t& max_size, const unsigned short& timeout_seconds) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting to read data with max_size: "
			<< humanreadable_bytes << max_size << nohumanreadable << std::endl;

	if (!m_handle) {
		return Unexpected<ConnectionError>("Receive failed: Invalid socket handle");
	}

	Buffer::FIFO buffer;
	std::size_t total_bytes_read = 0;
	const auto start_time = std::chrono::steady_clock::now();

	const std::size_t preferred = (m_effective_recv_buf > 0)
		? static_cast<std::size_t>(m_effective_recv_buf)
		: DEFAULT_IO_CHUNK;

	// Reuse one heap buffer across iterations (sized to max chunk, not 200 MiB).
	const std::size_t buf_cap = ClampChunk(preferred, max_size > 0 ? max_size : MAX_SINGLE_IO);
	std::vector<char> internal_buffer(buf_cap);

	while (true) {
		const std::size_t remaining = (max_size > 0) ? (max_size - total_bytes_read) : buf_cap;
		if (max_size > 0 && remaining == 0)
			break;

		const std::size_t bytes_to_read = ClampChunk(preferred, remaining);
		if (internal_buffer.size() < bytes_to_read)
			internal_buffer.resize(bytes_to_read);

#ifdef LINUX
		const ssize_t valread = recv(m_handle, internal_buffer.data(), bytes_to_read, 0);
#else
		const int valread = recv(m_handle, internal_buffer.data(), static_cast<int>(bytes_to_read), 0);
#endif

		if (valread > 0) {
			m_logger << Logger::Level::Debug << "Chunk received. Size: "
					<< humanreadable_bytes << valread << nohumanreadable << std::endl;
			(void)buffer.Write(std::span<const std::byte>(
				reinterpret_cast<const std::byte*>(internal_buffer.data()),
				static_cast<std::size_t>(valread)));
			total_bytes_read += static_cast<std::size_t>(valread);
			if (max_size > 0 && total_bytes_read >= max_size) {
				m_logger << Logger::Level::LowLevel << "Reached requested max_size: "
						<< humanreadable_bytes << total_bytes_read << nohumanreadable
						<< ". Exiting loop." << std::endl;
				break;
			}
			continue;
		}

		if (valread == 0) {
			m_logger << Logger::Level::Debug << "Connection closed by peer. Exiting read loop." << std::endl;
			break;
		}

		// valread < 0
#ifdef WINDOWS
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
#else
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN ||
			Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
#endif
			if (timeout_seconds > 0) {
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() >= timeout_seconds) {
					m_logger << Logger::Level::LowLevel << "Receive timed out after "
							<< timeout_seconds << " seconds" << std::endl;
					return Unexpected<ConnectionError>("Receive timed out");
				}
			}

			auto wait_res = WaitForData(100000); // 100ms
			if (!wait_res) {
				break;
			}
			if (wait_res.value() == Connection::Read::Result::Timeout) {
				if (timeout_seconds > 0) {
					auto now = std::chrono::steady_clock::now();
					if (std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() >= timeout_seconds) {
						m_logger << Logger::Level::LowLevel << "Receive timed out after "
								<< timeout_seconds << " seconds" << std::endl;
						return Unexpected<ConnectionError>("Receive timed out");
					}
				}
				// max_size > 0 (frame reads): keep waiting until full size or peer close
				if (max_size == 0 && total_bytes_read > 0) {
					break;
				}
				continue;
			}
			continue;
		}

		m_logger << Logger::Level::LowLevel << "Read error: "
				<< Connection::Handler::Instance().LastError() << std::endl;
		return Unexpected<ConnectionError>("Receive failed: {}", Connection::Handler::Instance().LastError());
	}

	m_logger << Logger::Level::LowLevel << "Total data received: "
			<< humanreadable_bytes << buffer.Size() << nohumanreadable << std::endl;
	return buffer;
}

ExpectedVoid Socket::Client::Write(std::span<const std::byte> data, const std::size_t& size) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting to write data..." << std::endl;

	if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected) {
		m_logger << Logger::Level::LowLevel << "Failed to write: Client is not connected" << std::endl;
		return Unexpected<ConnectionError>("Failed to write: Client is not connected");
	}

	std::size_t bytes_to_write = std::min(size, data.size());
	std::size_t total_written = 0;
	const std::size_t preferred = (m_effective_send_buf > 0)
		? static_cast<std::size_t>(m_effective_send_buf)
		: DEFAULT_IO_CHUNK;

	while (total_written < bytes_to_write) {
		auto current_data = data.subspan(total_written);
		std::size_t to_write = ClampChunk(preferred, bytes_to_write - total_written);
		to_write = std::min(to_write, current_data.size());
		auto chunk = current_data.subspan(0, to_write);

#ifdef LINUX
		const int send_flags = MSG_NOSIGNAL;
		const ssize_t written = ::send(m_handle,
#else
		const int send_flags = 0;
		const int written = ::send(m_handle,
#endif
			reinterpret_cast<const char*>(chunk.data()),
			static_cast<int>(chunk.size()), send_flags);

		if (written <= 0) {
#ifdef WINDOWS
			if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
				continue;
			}
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
#endif
			int sys_errno = errno;
			m_logger << Logger::Level::Error << "Write failed: " << Connection::Handler::Instance().LastError()
					<< " (code: " << Connection::Handler::Instance().LastErrorCode() << ")"
					<< " errno: " << sys_errno << " (" << Connection::Handler::Instance().ErrnoToString(sys_errno) << ")" << std::endl;
			return Unexpected<ConnectionError>(
				"Write failed: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode());
		}
		total_written += static_cast<std::size_t>(written);
	}

	m_logger << Logger::Level::LowLevel << "Write of size " << humanreadable_bytes << bytes_to_write
			<< nohumanreadable << " bytes completed successfully" << std::endl;
	return {};
}

bool Socket::Client::Ping() noexcept {
	if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected) {
		return false;
	}
	bool ping_success = false;
	char buffer[1];
#ifdef LINUX
	ssize_t result = ::recv(m_handle, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
#else
	int result = ::recv(m_handle, buffer, sizeof(buffer), MSG_PEEK);
	if (result == SOCKET_ERROR) {
		if (Connection::Handler::Instance().LastErrorCode() == WSAEWOULDBLOCK) {
			ping_success = true;
		}
	}
#endif
	if (result > 0) {
		ping_success = true;
	} else if (result == 0) {
		ping_success = false;
	} else {
#ifdef LINUX
		if (Connection::Handler::Instance().LastErrorCode() == EAGAIN ||
			Connection::Handler::Instance().LastErrorCode() == EWOULDBLOCK) {
			ping_success = true;
		} else {
			ping_success = false;
		}
#endif
	}

	if (ping_success) {
		m_logger << Logger::Level::LowLevel << "Ping successful" << std::endl;
	} else {
		m_logger << Logger::Level::LowLevel << "Ping failed" << std::endl;
		m_status.store(Connection::Status::Disconnected, std::memory_order_release);
	}

	return ping_success;
}
