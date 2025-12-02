#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/system.hxx>
#include <StormByte/uuid.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include <chrono>
#include <format>
#include <atomic>
#ifdef LINUX
#include <fstream>
#include <string>
#endif

constexpr const int SOCKET_BUFFER_SIZE = 262144; // 256 KiB

using namespace StormByte::Network::Socket;

Socket::Socket(const Protocol& protocol, Logger::ThreadedLog logger) noexcept:
m_protocol(protocol), m_status(Connection::Status::Disconnected),
m_handle(-1), m_conn_info(nullptr), m_mtu(DEFAULT_MTU), m_logger(logger),
m_UUID(StormByte::GenerateUUIDv4()) {
	// Ensure platform networking is initialized (e.g., WSAStartup on Windows)
	(void)StormByte::Network::Connection::Handler::Instance();
}

Socket::Socket(Socket&& other) noexcept:
m_protocol(other.m_protocol), m_status(other.m_status),
m_handle(std::move(other.m_handle)), m_conn_info(std::move(other.m_conn_info)),
m_mtu(other.m_mtu), m_logger(other.m_logger), m_UUID(std::move(other.m_UUID)) {
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
		m_conn_info = std::move(other.m_conn_info);
		m_mtu = other.m_mtu;
		m_logger = std::move(other.m_logger);
		m_UUID = std::move(other.m_UUID);
		other.m_status = Connection::Status::Disconnected;
	}
	return *this;
}

void Socket::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status))
		return;

	m_status = Connection::Status::Disconnecting;

	if (m_handle > 0) {
		#ifdef LINUX
		shutdown(m_handle, SHUT_RDWR);
		StormByte::System::Sleep(std::chrono::milliseconds(100)); // Allow time for TCP FIN to be sent
		close(m_handle);
		m_handle = -1;
		#else
		shutdown(m_handle, SD_BOTH);
		StormByte::System::Sleep(std::chrono::milliseconds(100)); // Allow time for TCP FIN to be sent
		closesocket(m_handle);
		m_handle = INVALID_SOCKET;
		#endif
	}

	m_status = Connection::Status::Disconnected;
	m_logger << Logger::Level::LowLevel << "Disconnected socket " << m_UUID << std::endl;
}

StormByte::Network::ExpectedReadResult Socket::WaitForData(const long long& usecs) noexcept {
	if (!Connection::IsConnected(m_status)) {
		return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: Invalid connection status");
	}

	// Store the starting time and compute deadline when a timeout is requested
	auto start_time = std::chrono::steady_clock::now();
	const std::chrono::microseconds requested_usecs = std::chrono::microseconds(usecs);
	// Enforce a sensible minimum wait to avoid very tight polling from callers
	// Use a smaller minimum (10ms) to remain responsive while avoiding busy loops
	constexpr std::chrono::microseconds MIN_WAIT = std::chrono::microseconds(10000); // 10ms
	const std::chrono::microseconds effective_usecs = (usecs > 0) ? std::max(requested_usecs, MIN_WAIT) : std::chrono::microseconds::zero();
	const auto deadline = (usecs > 0) ? (start_time + effective_usecs) : std::chrono::steady_clock::time_point::max();

	// Throttle logging so we don't spam on tight loops. Use a process-wide
	// atomic timestamp (microseconds since epoch) so multiple threads don't
	// all log simultaneously. This ensures at most one "Waiting for data"
	// log per second across the process.
	static std::atomic<long long> s_last_log_us{0};

	while (Connection::IsConnected(m_status)) {
		auto now = std::chrono::steady_clock::now();
		long long now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
		long long prev = s_last_log_us.load(std::memory_order_relaxed);
		if (now_us - prev >= 1000000) {
			// Attempt to update; only the successful thread should log.
			if (s_last_log_us.compare_exchange_strong(prev, now_us, std::memory_order_acq_rel)) {
				m_logger << Logger::Level::LowLevel << "Waiting for data on socket..." << std::endl;
			}
		}



		// Platform-specific wait: use epoll on Linux for event-driven semantics
#ifdef LINUX
		int timeout_ms = -1;
		if (usecs > 0) {
			auto now2 = std::chrono::steady_clock::now();
			if (now2 >= deadline) return Connection::Read::Result::Timeout;
			auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now2);
			timeout_ms = static_cast<int>(remaining.count());
			if (timeout_ms < 0) timeout_ms = 0;
		}

		// Create a one-shot epoll instance, add our socket, wait, then clean up.
		int epfd = epoll_create1(0);
		if (epfd == -1) {
			// Fall back to select path if epoll creation failed
			return StormByte::Unexpected<ConnectionClosed>("Failed to create epoll instance");
		}

		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLPRI;
		ev.data.fd = m_handle;

		if (epoll_ctl(epfd, EPOLL_CTL_ADD, m_handle, &ev) == -1) {
			close(epfd);
			return StormByte::Unexpected<ConnectionClosed>("Failed to add fd to epoll");
		}

		struct epoll_event events[1];
		int nfds = epoll_wait(epfd, events, 1, timeout_ms);
		// Clean up epoll registration and fd
		epoll_ctl(epfd, EPOLL_CTL_DEL, m_handle, nullptr);
		close(epfd);

		if (nfds > 0) {
			// Event(s) available
			if (m_status != Connection::Status::Connected) return Connection::Read::Result::Closed;
			return Connection::Read::Result::Success;
		} else if (nfds == 0) {
			return Connection::Read::Result::Timeout;
		} else {
			// Error occurred
			if (errno == ECONNRESET || errno == EBADF) {
				return StormByte::Unexpected<ConnectionClosed>("Connection closed or invalid socket");
			}
			return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: epoll_wait error");
		}
#else
		// Windows: use WSAEventSelect + WSAWaitForMultipleEvents for event-driven wait
		int timeout_ms = -1;
		if (usecs > 0) {
			auto now2 = std::chrono::steady_clock::now();
			if (now2 >= deadline) return Connection::Read::Result::Timeout;
			auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now2);
			timeout_ms = static_cast<int>(remaining.count());
			if (timeout_ms < 0) timeout_ms = 0;
		}

		WSAEVENT ev = WSACreateEvent();
		if (ev == WSA_INVALID_EVENT) {
			return StormByte::Unexpected<ConnectionClosed>("Failed to create WSA event");
		}

		// Request notification for read/close/accept events on the socket
		long mask = FD_READ | FD_CLOSE | FD_ACCEPT;
		if (WSAEventSelect(m_handle, ev, mask) == SOCKET_ERROR) {
			WSACloseEvent(ev);
			return StormByte::Unexpected<ConnectionClosed>("WSAEventSelect failed");
		}

		DWORD wait_res = WSAWaitForMultipleEvents(1, &ev, FALSE, (timeout_ms < 0 ? WSA_INFINITE : static_cast<DWORD>(timeout_ms)), FALSE);

		// Disable event notification and clean up
		WSAEventSelect(m_handle, NULL, 0);
		WSACloseEvent(ev);

		if (wait_res == WSA_WAIT_TIMEOUT) {
			return Connection::Read::Result::Timeout;
		} else if (wait_res == WSA_WAIT_FAILED) {
			int wsa_err = WSAGetLastError();
			if (wsa_err == WSAECONNRESET || wsa_err == WSAENOTSOCK) {
				return StormByte::Unexpected<ConnectionClosed>("Connection closed or invalid socket");
			}
			return StormByte::Unexpected<ConnectionClosed>("WSAWaitForMultipleEvents failed");
		} else {
			// Event signaled — treat as data available or connection state change
			if (m_status != Connection::Status::Connected) return Connection::Read::Result::Closed;
			return Connection::Read::Result::Success;
		}
	#endif
	}

	return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: Unknown error occurred");
}

StormByte::Expected<StormByte::Network::Connection::HandlerType, StormByte::Network::ConnectionError> Socket::CreateSocket() noexcept {
	// Make sure the platform handler is constructed before calling ::socket
	(void)StormByte::Network::Connection::Handler::Instance();
	auto protocol = m_protocol == Protocol::IPv4 ? AF_INET : AF_INET6;
	Connection::HandlerType handle = ::socket(protocol, SOCK_STREAM, 0);
	#ifdef WINDOWS
	if (handle == INVALID_SOCKET) {
	#else
	if (handle == -1) {
	#endif
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected<ConnectionError>(Connection::Handler::Instance().LastError());
	}

	return handle;
}

void Socket::InitializeAfterConnect() noexcept {
	m_status = Connection::Status::Connecting;
	m_mtu = GetMTU();
	SetNonBlocking();
	
	// Increase the socket buffer sizes (send and receive)
	int desired_buf = SOCKET_BUFFER_SIZE;
	int rc = 0;

	// On Linux try to detect system maximums and use them if larger than our desired size
#ifdef LINUX
	auto read_proc_int = [](const char* path) -> int {
		std::ifstream f(path);
		if (!f.is_open()) return -1;
		std::string s;
		std::getline(f, s);
		try {
			return std::stoi(s);
		} catch (...) {
			return -1;
		}
	};

	int sys_wmem_max = read_proc_int("/proc/sys/net/core/wmem_max");
	int sys_rmem_max = read_proc_int("/proc/sys/net/core/rmem_max");
	if (sys_wmem_max > 0) {
		m_logger << Logger::Level::LowLevel << "System wmem_max: " << Logger::humanreadable_bytes << sys_wmem_max << Logger::nohumanreadable << std::endl;
	}
	if (sys_rmem_max > 0) {
		m_logger << Logger::Level::LowLevel << "System rmem_max: " << Logger::humanreadable_bytes << sys_rmem_max << Logger::nohumanreadable << std::endl;
	}

	int send_buf = desired_buf;
	int recv_buf = desired_buf;
	if (sys_wmem_max > send_buf) send_buf = sys_wmem_max;
	if (sys_rmem_max > recv_buf) recv_buf = sys_rmem_max;

	rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) failed: " << Connection::Handler::Instance().LastError() << std::endl;
	}
	rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) failed: " << Connection::Handler::Instance().LastError() << std::endl;
	}
#else
	// On Windows there's no simple /proc equivalent. Try to request a large buffer
	// (the system will clamp it to the maximum allowed). We'll attempt a reasonably
	// large value and read back the effective size.
	constexpr int WINDOWS_DESIRED_MAX = 64 * 1024 * 1024; // 64 MiB
	int try_send = WINDOWS_DESIRED_MAX;
	int try_recv = WINDOWS_DESIRED_MAX;

	rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&try_send), sizeof(try_send));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) attempt failed: " << Connection::Handler::Instance().LastError() << std::endl;
		// Fall back to requested default
		rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) fallback failed: " << Connection::Handler::Instance().LastError() << std::endl;
		}
	}

	rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&try_recv), sizeof(try_recv));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) attempt failed: " << Connection::Handler::Instance().LastError() << std::endl;
		rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) fallback failed: " << Connection::Handler::Instance().LastError() << std::endl;
		}
	}
#endif

	// Query and log effective buffer sizes
	int effective = 0;
#ifdef WINDOWS
	int optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << Logger::humanreadable_bytes << effective << Logger::nohumanreadable << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << Logger::humanreadable_bytes << effective << Logger::nohumanreadable << std::endl;
		m_effective_recv_buf = effective;

		// Compute per-call chunk capacities (capped by the client-side MAX_SINGLE_IO)
		std::size_t send_cap = static_cast<std::size_t>(m_effective_send_buf);
		std::size_t recv_cap = static_cast<std::size_t>(m_effective_recv_buf);
		const std::size_t max_single = 4 * 1024 * 1024; // must match client.cxx MAX_SINGLE_IO
		if (send_cap == 0) send_cap = 65536;
		if (recv_cap == 0) recv_cap = 65536;
		send_cap = std::min(send_cap, max_single);
		recv_cap = std::min(recv_cap, max_single);
		m_logger << Logger::Level::LowLevel << Logger::humanreadable_bytes << "Per-call send capacity: " << send_cap << ", recv capacity: " << recv_cap << " (max single IO: " << max_single << ")" << Logger::nohumanreadable << std::endl;
	}
#else
	socklen_t optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << Logger::humanreadable_bytes << effective << Logger::nohumanreadable << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << Logger::humanreadable_bytes << effective << Logger::nohumanreadable << std::endl;
		m_effective_recv_buf = effective;
	}
#if defined(LINUX)
	// Compute per-call chunk capacities used by the client code (capped by MAX_SINGLE_IO)
	std::size_t send_cap = static_cast<std::size_t>(m_effective_send_buf);
	std::size_t recv_cap = static_cast<std::size_t>(m_effective_recv_buf);
	const std::size_t max_single = 4 * 1024 * 1024; // must match client.cxx MAX_SINGLE_IO
	if (send_cap == 0) send_cap = 65536;
	if (recv_cap == 0) recv_cap = 65536;
	send_cap = std::min(send_cap, max_single);
	recv_cap = std::min(recv_cap, max_single);
	m_logger << Logger::Level::LowLevel << Logger::humanreadable_bytes << "Per-call send capacity: " << send_cap << ", recv capacity: " << recv_cap << " (max single IO: " << max_single << ")" << Logger::nohumanreadable << std::endl;
#endif
#endif

	// Disable Nagle for lower latency on small writes
	int flag = 1;
#ifdef WINDOWS
	rc = setsockopt(m_handle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(TCP_NODELAY) failed: " << Connection::Handler::Instance().LastError() << std::endl;
	}
#else
	rc = setsockopt(m_handle, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(TCP_NODELAY) failed: " << Connection::Handler::Instance().LastError() << std::endl;
	}
#endif
	m_status = Connection::Status::Connected;
}

#ifdef LINUX
int Socket::GetMTU() const noexcept {
if (!m_conn_info || !m_handle)
		return DEFAULT_MTU;

	int mtu;
	socklen_t optlen = sizeof(mtu);

	// Retrieve the MTU for the connected socket
	if (getsockopt(m_handle, IPPROTO_IP, IP_MTU, &mtu, &optlen) >= 0)
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
	ioctlsocket(m_handle, FIONBIO, &mode);
	#else
	int flags = fcntl(m_handle, F_GETFL, 0);
	fcntl(m_handle, F_SETFL, flags | O_NONBLOCK);
	#endif
}
