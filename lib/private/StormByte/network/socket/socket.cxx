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

constexpr const int SOCKET_BUFFER_SIZE = 262144; // 256 KiB desired minimum
constexpr const std::size_t MAX_SINGLE_IO = 4 * 1024 * 1024; // must match client.cxx

using namespace StormByte::Network::Socket;

Socket::Socket(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept:
m_protocol(protocol), m_status(Connection::Status::Disconnected),
m_handle(-1), m_conn_info(nullptr), m_mtu(DEFAULT_MTU), m_logger(logger),
m_UUID(StormByte::GenerateUUIDv4()) {
	(void)StormByte::Network::Connection::Handler::Instance();
}

Socket::Socket(Socket&& other) noexcept:
	m_protocol(other.m_protocol),
	m_status(other.m_status.load(std::memory_order_relaxed)),
	m_handle(std::move(other.m_handle)),
	m_conn_info(std::move(other.m_conn_info)),
	m_mtu(other.m_mtu),
	m_logger(other.m_logger),
	m_UUID(std::move(other.m_UUID)),
	m_effective_send_buf(other.m_effective_send_buf),
	m_effective_recv_buf(other.m_effective_recv_buf)
{
	other.m_status.store(Connection::Status::Disconnected, std::memory_order_relaxed);
	other.m_effective_send_buf = 0;
	other.m_effective_recv_buf = 0;
}

Socket& Socket::operator=(Socket&& other) noexcept {
	if (this != &other) {
		m_protocol = other.m_protocol;
		m_status.store(other.m_status.load(std::memory_order_relaxed), std::memory_order_relaxed);
		m_handle = std::move(other.m_handle);
		m_conn_info = std::move(other.m_conn_info);
		m_mtu = other.m_mtu;
		m_logger = std::move(other.m_logger);
		m_UUID = std::move(other.m_UUID);
		m_effective_send_buf = other.m_effective_send_buf;
		m_effective_recv_buf = other.m_effective_recv_buf;

		other.m_status.store(Connection::Status::Disconnected, std::memory_order_relaxed);
		other.m_effective_send_buf = 0;
		other.m_effective_recv_buf = 0;
	}
	return *this;
}

Socket::~Socket() noexcept {
	Disconnect();
}

void Socket::Disconnect() noexcept {
	// Only one thread performs the real close.
	auto prev = m_status.exchange(Connection::Status::Disconnecting,
								std::memory_order_acq_rel);
	if (prev == Connection::Status::Disconnected ||
		prev == Connection::Status::Disconnecting) {
		return;
	}

	if (m_handle > 0) {
#ifdef LINUX
		shutdown(m_handle, SHUT_RDWR);
		StormByte::System::Sleep(std::chrono::milliseconds(100));
		close(m_handle);
		m_handle = -1;
#else
		shutdown(m_handle, SD_BOTH);
		StormByte::System::Sleep(std::chrono::milliseconds(100));
		closesocket(m_handle);
		m_handle = INVALID_SOCKET;
#endif
	}

	m_status.store(Connection::Status::Disconnected, std::memory_order_release);
	m_logger << Logger::Level::LowLevel << "Disconnected socket " << m_UUID << std::endl;
}

StormByte::Network::ExpectedReadResult Socket::WaitForData(const long long& usecs) noexcept {
	if (!Connection::IsConnected(m_status.load(std::memory_order_acquire))) {
		return Unexpected<ConnectionClosed>("Failed to wait for data: Invalid connection status");
	}

	auto start_time = std::chrono::steady_clock::now();
	const std::chrono::microseconds requested_usecs = std::chrono::microseconds(usecs);
	constexpr std::chrono::microseconds MIN_WAIT = std::chrono::microseconds(10000); // 10ms
	const std::chrono::microseconds effective_usecs =
		(usecs > 0) ? std::max(requested_usecs, MIN_WAIT) : std::chrono::microseconds::zero();
	const auto deadline = (usecs > 0) ? (start_time + effective_usecs)
									: std::chrono::steady_clock::time_point::max();

	static std::atomic<long long> s_last_log_us{0};

	while (Connection::IsConnected(m_status.load(std::memory_order_acquire))) {
		auto now = std::chrono::steady_clock::now();
		long long now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
		long long prev = s_last_log_us.load(std::memory_order_relaxed);
		if (now_us - prev >= 1000000) {
			if (s_last_log_us.compare_exchange_strong(prev, now_us, std::memory_order_acq_rel)) {
				m_logger << Logger::Level::LowLevel << "Waiting for data on socket..." << std::endl;
			}
		}

#ifdef LINUX
		int timeout_ms = -1;
		if (usecs > 0) {
			auto now2 = std::chrono::steady_clock::now();
			if (now2 >= deadline) return Connection::Read::Result::Timeout;
			auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now2);
			timeout_ms = static_cast<int>(remaining.count());
			if (timeout_ms < 0) timeout_ms = 0;
		}

		int epfd = epoll_create1(0);
		if (epfd == -1) {
			return Unexpected<ConnectionClosed>("Failed to create epoll instance");
		}

		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
		ev.data.fd = m_handle;

		if (epoll_ctl(epfd, EPOLL_CTL_ADD, m_handle, &ev) == -1) {
			close(epfd);
			return Unexpected<ConnectionClosed>("Failed to add fd to epoll");
		}

		struct epoll_event events[1];
		int nfds = epoll_wait(epfd, events, 1, timeout_ms);
		epoll_ctl(epfd, EPOLL_CTL_DEL, m_handle, nullptr);
		close(epfd);

		if (nfds > 0) {
			uint32_t evflags = events[0].events;
			if (evflags & EPOLLERR) {
				return Unexpected<ConnectionClosed>("Socket error while waiting for data");
			}

			if (evflags & (EPOLLHUP | EPOLLRDHUP)) {
				if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected)
					return Connection::Read::Result::Closed;
				if (evflags & EPOLLIN) {
					char tmp;
					ssize_t r = recv(m_handle, &tmp, 1, MSG_PEEK | MSG_DONTWAIT);
					if (r > 0) {
						return Connection::Read::Result::Success;
					} else if (r == 0) {
						return Connection::Read::Result::ShutdownRequest;
					} else {
						if (errno == EWOULDBLOCK || errno == EAGAIN) {
							return Connection::Read::Result::Success;
						}
						return Unexpected<ConnectionClosed>("recv(MSG_PEEK) error while checking shutdown");
					}
				}
				return Connection::Read::Result::ShutdownRequest;
			}

			if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected)
				return Connection::Read::Result::Closed;
			if (evflags & (EPOLLIN | EPOLLPRI))
				return Connection::Read::Result::Success;
			return Unexpected<ConnectionClosed>("Unknown epoll event while waiting for data");
		} else if (nfds == 0) {
			return Connection::Read::Result::Timeout;
		} else {
			if (errno == ECONNRESET || errno == EBADF) {
				return Unexpected<ConnectionClosed>("Connection closed or invalid socket");
			}
			return Unexpected<ConnectionClosed>("Failed to wait for data: epoll_wait error");
		}
#else
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
			return Unexpected<ConnectionClosed>("Failed to create WSA event");
		}

		long mask = FD_READ | FD_CLOSE | FD_ACCEPT;
		if (WSAEventSelect(m_handle, ev, mask) == SOCKET_ERROR) {
			WSACloseEvent(ev);
			return Unexpected<ConnectionClosed>("WSAEventSelect failed");
		}

		DWORD wait_res = WSAWaitForMultipleEvents(
			1, &ev, FALSE,
			(timeout_ms < 0 ? WSA_INFINITE : static_cast<DWORD>(timeout_ms)),
			FALSE);

		if (wait_res == WSA_WAIT_TIMEOUT) {
			WSAEventSelect(m_handle, NULL, 0);
			WSACloseEvent(ev);
			return Connection::Read::Result::Timeout;
		} else if (wait_res == WSA_WAIT_FAILED) {
			int wsa_err = WSAGetLastError();
			WSAEventSelect(m_handle, NULL, 0);
			WSACloseEvent(ev);
			if (wsa_err == WSAECONNRESET || wsa_err == WSAENOTSOCK) {
				return Unexpected<ConnectionClosed>("Connection closed or invalid socket");
			}
			return Unexpected<ConnectionClosed>("WSAWaitForMultipleEvents failed");
		} else {
			WSANETWORKEVENTS netev;
			if (WSAEnumNetworkEvents(m_handle, ev, &netev) == SOCKET_ERROR) {
				WSAEventSelect(m_handle, NULL, 0);
				WSACloseEvent(ev);
				return Unexpected<ConnectionClosed>("WSAEnumNetworkEvents failed");
			}

			WSAEventSelect(m_handle, NULL, 0);
			WSACloseEvent(ev);

			if (netev.lNetworkEvents & FD_CLOSE) {
				int err = netev.iErrorCode[FD_CLOSE_BIT];
				if (err != 0) {
					return Unexpected<ConnectionClosed>("Connection closed with error");
				}
				if ((netev.lNetworkEvents & FD_READ) != 0) {
					char tmp;
					int r = recv(m_handle, &tmp, 1, MSG_PEEK);
					if (r > 0) {
						return Connection::Read::Result::Success;
					} else if (r == 0) {
						return Connection::Read::Result::ShutdownRequest;
					} else {
						int wsaerr = WSAGetLastError();
						if (wsaerr == WSAEWOULDBLOCK) {
							return Connection::Read::Result::Success;
						}
						return Unexpected<ConnectionClosed>("recv(MSG_PEEK) failed while checking shutdown");
					}
				}
				return Connection::Read::Result::ShutdownRequest;
			}

			if (netev.lNetworkEvents & FD_READ) {
				if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected)
					return Connection::Read::Result::Closed;
				return Connection::Read::Result::Success;
			}

			m_logger << Logger::Level::LowLevel
					<< "WSA wait signaled unknown network event (flags=0x" << std::hex
					<< netev.lNetworkEvents << std::dec << ")" << std::endl;
			if (m_status.load(std::memory_order_acquire) != Connection::Status::Connected)
				return Connection::Read::Result::Closed;
			return Connection::Read::Result::Success;
		}
#endif
	}

	return Unexpected<ConnectionClosed>("Failed to wait for data: Unknown error occurred");
}

StormByte::Expected<StormByte::Network::Connection::HandlerType, StormByte::Network::ConnectionError>
Socket::CreateSocket() noexcept {
	(void)StormByte::Network::Connection::Handler::Instance();
	Connection::HandlerType handle = ::socket(Connection::ProtocolInt(m_protocol), SOCK_STREAM, 0);
#ifdef WINDOWS
	if (handle == INVALID_SOCKET) {
#else
	if (handle == -1) {
#endif
		m_status.store(Connection::Status::Disconnected, std::memory_order_release);
		return Unexpected<ConnectionError>(Connection::Handler::Instance().LastError());
	}

	return handle;
}

void Socket::InitializeAfterConnect() noexcept {
	m_status.store(Connection::Status::Connecting, std::memory_order_release);
	m_mtu = GetMTU();
	SetNonBlocking();

	int desired_buf = SOCKET_BUFFER_SIZE;
	int rc = 0;

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
		m_logger << Logger::Level::LowLevel << "System wmem_max: " << Logger::humanreadable_bytes
				<< sys_wmem_max << Logger::nohumanreadable << std::endl;
	}
	if (sys_rmem_max > 0) {
		m_logger << Logger::Level::LowLevel << "System rmem_max: " << Logger::humanreadable_bytes
				<< sys_rmem_max << Logger::nohumanreadable << std::endl;
	}

	int send_buf = desired_buf;
	int recv_buf = desired_buf;
	if (sys_wmem_max > send_buf) send_buf = sys_wmem_max;
	if (sys_rmem_max > recv_buf) recv_buf = sys_rmem_max;

	rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) failed: "
				<< Connection::Handler::Instance().LastError() << std::endl;
	}
	rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) failed: "
				<< Connection::Handler::Instance().LastError() << std::endl;
	}
#else
	constexpr int WINDOWS_DESIRED_MAX = 8 * 1024 * 1024; // 8 MiB request (OS will clamp)
	int try_send = WINDOWS_DESIRED_MAX;
	int try_recv = WINDOWS_DESIRED_MAX;

	rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF,
		reinterpret_cast<const char*>(&try_send), sizeof(try_send));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) attempt failed: "
				<< Connection::Handler::Instance().LastError() << std::endl;
		rc = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF,
			reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_SNDBUF) fallback failed: "
					<< Connection::Handler::Instance().LastError() << std::endl;
		}
	}

	rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF,
		reinterpret_cast<const char*>(&try_recv), sizeof(try_recv));
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) attempt failed: "
				<< Connection::Handler::Instance().LastError() << std::endl;
		rc = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF,
			reinterpret_cast<const char*>(&desired_buf), sizeof(desired_buf));
		if (rc != 0) {
			m_logger << Logger::Level::Warning << "setsockopt(SO_RCVBUF) fallback failed: "
					<< Connection::Handler::Instance().LastError() << std::endl;
		}
	}
#endif

	int effective = 0;
#ifdef WINDOWS
	int optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << Logger::humanreadable_bytes
				<< effective << Logger::nohumanreadable << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&effective), &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << Logger::humanreadable_bytes
				<< effective << Logger::nohumanreadable << std::endl;
		m_effective_recv_buf = effective;
	}
#else
	socklen_t optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_SNDBUF: " << Logger::humanreadable_bytes
				<< effective << Logger::nohumanreadable << std::endl;
		m_effective_send_buf = effective;
	}
	optlen = sizeof(effective);
	if (getsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, &effective, &optlen) == 0) {
		m_logger << Logger::Level::LowLevel << "Effective SO_RCVBUF: " << Logger::humanreadable_bytes
				<< effective << Logger::nohumanreadable << std::endl;
		m_effective_recv_buf = effective;
	}
#endif

	{
		std::size_t send_cap = static_cast<std::size_t>(m_effective_send_buf);
		std::size_t recv_cap = static_cast<std::size_t>(m_effective_recv_buf);
		if (send_cap == 0) send_cap = 65536;
		if (recv_cap == 0) recv_cap = 65536;
		send_cap = std::min(send_cap, MAX_SINGLE_IO);
		recv_cap = std::min(recv_cap, MAX_SINGLE_IO);
		m_logger << Logger::Level::LowLevel << "Per-call send capacity: " << Logger::humanreadable_bytes
				<< send_cap << ", recv capacity: " << recv_cap
				<< " (max single IO: " << MAX_SINGLE_IO << ")" << Logger::nohumanreadable << std::endl;
	}

	int flag = 1;
#ifdef WINDOWS
	rc = setsockopt(m_handle, IPPROTO_TCP, TCP_NODELAY,
		reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
	rc = setsockopt(m_handle, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif
	if (rc != 0) {
		m_logger << Logger::Level::Warning << "setsockopt(TCP_NODELAY) failed: "
				<< Connection::Handler::Instance().LastError() << std::endl;
	}
	m_status.store(Connection::Status::Connected, std::memory_order_release);
}

#ifdef LINUX
int Socket::GetMTU() const noexcept {
	if (!m_conn_info || !m_handle)
		return DEFAULT_MTU;

	int mtu;
	socklen_t optlen = sizeof(mtu);
	if (getsockopt(m_handle, IPPROTO_IP, IP_MTU, &mtu, &optlen) >= 0)
		return mtu;
	return DEFAULT_MTU;
}
#else
int Socket::GetMTU() const noexcept {
	if (!m_conn_info || !m_handle)
		return DEFAULT_MTU;

	ULONG out_buf_len = 0;
	GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &out_buf_len);

	auto adapter_addresses = std::make_unique<BYTE[]>(out_buf_len);
	if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL,
			reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get()),
			&out_buf_len) != ERROR_SUCCESS) {
		return DEFAULT_MTU;
	}

	PIP_ADAPTER_ADDRESSES adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses.get());
	while (adapter) {
		for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
			unicast != nullptr; unicast = unicast->Next) {
			if (unicast->Address.lpSockaddr->sa_family == m_conn_info->SockAddr()->sa_family &&
				std::memcmp(unicast->Address.lpSockaddr, m_conn_info->SockAddr().get(),
					sizeof(sockaddr)) == 0) {
				return static_cast<int>(adapter->Mtu);
			}
		}
		adapter = adapter->Next;
	}
	return DEFAULT_MTU;
}
#endif

void Socket::SetNonBlocking() noexcept {
#ifdef WINDOWS
	u_long mode = 1;
	ioctlsocket(m_handle, FIONBIO, &mode);
#else
	int flags = fcntl(m_handle, F_GETFL, 0);
	fcntl(m_handle, F_SETFL, flags | O_NONBLOCK);
#endif
}
