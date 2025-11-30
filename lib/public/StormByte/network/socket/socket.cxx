#include <StormByte/network/socket/socket.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <atomic>
#ifdef LINUX
#include <fstream>
#include <string>
#endif

constexpr const int SOCKET_BUFFER_SIZE = 262144; // 256 KiB

using namespace StormByte::Network::Socket;

Socket::Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, Logger::ThreadedLog logger) noexcept:
m_protocol(protocol), m_status(Connection::Status::Disconnected),
m_handle(nullptr), m_conn_handler(handler), m_conn_info(nullptr), m_mtu(DEFAULT_MTU), m_logger(logger) {}

Socket::Socket(Socket&& other) noexcept:
m_protocol(other.m_protocol), m_status(other.m_status),
m_handle(std::move(other.m_handle)), m_conn_handler(other.m_conn_handler),
m_conn_info(std::move(other.m_conn_info)), m_mtu(other.m_mtu), m_logger(other.m_logger) {
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
		m_logger = std::move(other.m_logger);
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
	m_handle = nullptr;

	m_status = Connection::Status::Disconnected;
	m_logger << Logger::Level::LowLevel << "Disconnected socket" << std::endl;
}

StormByte::Network::ExpectedReadResult Socket::WaitForData(const long long& usecs) noexcept {
	if (!Connection::IsConnected(m_status)) {
		return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: Invalid connection status");
	}

	fd_set read_fds;

	// Store the starting time and compute deadline when a timeout is requested
	auto start_time = std::chrono::steady_clock::now();
	const std::chrono::microseconds requested_usecs = std::chrono::microseconds(usecs);
	// Enforce a sensible minimum wait to avoid very tight polling from callers
	constexpr std::chrono::microseconds MIN_WAIT = std::chrono::microseconds(100000); // 100ms
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

		FD_ZERO(&read_fds);
		FD_SET(*m_handle, &read_fds);

		// Recompute remaining timeout for this select call (select may modify timeval)
		struct timeval tv_storage;
		struct timeval* tv_ptr = nullptr;
		if (usecs > 0) {
			auto now2 = std::chrono::steady_clock::now();
			if (now2 >= deadline) {
				return Connection::Read::Result::Timeout; // Deadline expired
			}
			auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now2);
			tv_storage.tv_sec = static_cast<time_t>(remaining.count() / 1000000);
			tv_storage.tv_usec = static_cast<suseconds_t>(remaining.count() % 1000000);
			tv_ptr = &tv_storage;
		}

		int select_result;

#ifdef WINDOWS
		select_result = select(0, &read_fds, nullptr, nullptr, tv_ptr);
#else
		select_result = select(*m_handle + 1, &read_fds, nullptr, nullptr, tv_ptr);
#endif

		if (select_result > 0) {
			// Connection can be closed in this step and select will notify us
			if (m_status != Connection::Status::Connected) {
				return Connection::Read::Result::Closed; // Connection closed
			}
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
				return StormByte::Unexpected<ConnectionClosed>("Failed to wait for data: {} (error code: {})",
																			m_conn_handler->LastError(),
																			m_conn_handler->LastErrorCode());
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
		m_logger << Logger::Level::LowLevel << "System wmem_max: " << sys_wmem_max << std::endl;
	}
	if (sys_rmem_max > 0) {
		m_logger << Logger::Level::LowLevel << "System rmem_max: " << sys_rmem_max << std::endl;
	}

	int send_buf = desired_buf;
	int recv_buf = desired_buf;
	if (sys_wmem_max > send_buf) send_buf = sys_wmem_max;
	if (sys_rmem_max > recv_buf) recv_buf = sys_rmem_max;

	rc = setsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) failed: " << m_conn_handler->LastError() << std::endl;
	}
	rc = setsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) failed: " << m_conn_handler->LastError() << std::endl;
	}
#else
	// On Windows there's no simple /proc equivalent. Try to request a large buffer
	// (the system will clamp it to the maximum allowed). We'll attempt a reasonably
	// large value and read back the effective size.
	constexpr int WINDOWS_DESIRED_MAX = 64 * 1024 * 1024; // 64 MiB
	int try_send = WINDOWS_DESIRED_MAX;
	int try_recv = WINDOWS_DESIRED_MAX;

	rc = setsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&try_send), sizeof(try_send));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) attempt failed: " << m_conn_handler->LastError() << std::endl;
		// Fall back to requested default
		rc = setsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) fallback failed: " << m_conn_handler->LastError() << std::endl;
		}
	}

	rc = setsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&try_recv), sizeof(try_recv));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) attempt failed: " << m_conn_handler->LastError() << std::endl;
		rc = setsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) fallback failed: " << m_conn_handler->LastError() << std::endl;
		}
	}
#endif

	// Query and log effective buffer sizes
	int effective = 0;
#ifdef WINDOWS
	int optlen = sizeof(effective);
	if (getsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << effective << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << effective << std::endl;
		m_effective_recv_buf = effective;

		// Compute per-call chunk capacities (capped by the client-side MAX_SINGLE_IO)
		std::size_t send_cap = static_cast<std::size_t>(m_effective_send_buf);
		std::size_t recv_cap = static_cast<std::size_t>(m_effective_recv_buf);
		const std::size_t max_single = 4 * 1024 * 1024; // must match client.cxx MAX_SINGLE_IO
		if (send_cap == 0) send_cap = 65536;
		if (recv_cap == 0) recv_cap = 65536;
		send_cap = std::min(send_cap, max_single);
		recv_cap = std::min(recv_cap, max_single);
		m_logger << Logger::Level::LowLevel << "Per-call send capacity: " << send_cap << " bytes, recv capacity: " << recv_cap << " bytes (max single IO: " << max_single << ")" << std::endl;
	}
#else
	socklen_t optlen = sizeof(effective);
	if (getsockopt(*m_handle, SOL_SOCKET, SO_SNDBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << effective << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(*m_handle, SOL_SOCKET, SO_RCVBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << effective << std::endl;
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
	m_logger << Logger::Level::LowLevel << "Per-call send capacity: " << send_cap << " bytes, recv capacity: " << recv_cap << " bytes (max single IO: " << max_single << ")" << std::endl;
#endif
#endif

	// Disable Nagle for lower latency on small writes
	int flag = 1;
#ifdef WINDOWS
	rc = setsockopt(*m_handle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(TCP_NODELAY) failed: " << m_conn_handler->LastError() << std::endl;
	}
#else
	rc = setsockopt(*m_handle, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(TCP_NODELAY) failed: " << m_conn_handler->LastError() << std::endl;
	}
#endif
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