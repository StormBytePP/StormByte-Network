#include <StormByte/network/connection/info.hxx>

#ifdef LINUX

#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <vector>
#endif

#include <format>

using namespace StormByte::Network::Connection;

Info::Info(std::shared_ptr<sockaddr> sock_addr) noexcept:
	m_sock_addr(sock_addr), m_mtu(DEFAULT_MTU), m_ip(), m_port(0) {
	Initialize(sock_addr);
}

StormByte::Expected<Info, StormByte::Network::Exception> Info::FromHost(const std::string& hostname, const unsigned short& port, const Protocol& protocol, std::shared_ptr<const Handler> handler) noexcept {
	auto expected_sock_addr = Info::ResolveHostname(hostname, port, protocol, handler);
	if (!expected_sock_addr)
		return StormByte::Unexpected<Exception>(expected_sock_addr.error());

	return Info(std::move(expected_sock_addr.value()));
}

StormByte::Expected<Info, StormByte::Network::Exception> Info::FromSockAddr(std::shared_ptr<sockaddr> sockaddr) noexcept {
	if (!sockaddr)
		return StormByte::Unexpected<Exception>("Invalid socket address");

	return Info(sockaddr);
}

StormByte::Expected<std::shared_ptr<sockaddr>, StormByte::Network::Exception> Info::ResolveHostname(const std::string& hostname, const unsigned short& port, const Protocol& protocol, std::shared_ptr<const Handler> handler) noexcept {
	struct addrinfo hints{}, *res = nullptr;

	hints.ai_family = protocol == Protocol::IPv4 ? AF_INET : AF_INET6; // Use IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;              // For TCP connections

	int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
	if (ret != 0 || !res)
		return StormByte::Unexpected<StormByte::Network::Exception>(std::format("Can't resolve host '{}': {}", hostname, handler->LastError()));

	std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> res_guard(res, freeaddrinfo);

	char ipstr[INET6_ADDRSTRLEN];
	void* addr = nullptr;

	if (res->ai_family == AF_INET) {
		addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr; // IPv4
	} else if (res->ai_family == AF_INET6) {
		addr = &((struct sockaddr_in6*)res->ai_addr)->sin6_addr; // IPv6
	}
	if (!addr)
		return StormByte::Unexpected<StormByte::Network::Exception>("Unable to determine resolved address");

	inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr)); // Convert address to string
	if (!addr)
		return StormByte::Unexpected<StormByte::Network::Exception>("Unable to determine resolved address");

	sockaddr_in resolved{};
	resolved.sin_family = (protocol == Protocol::IPv4) ? AF_INET : AF_INET6;
	resolved.sin_port = htons(port);

	if (inet_pton(resolved.sin_family, ipstr, &resolved.sin_addr) <= 0) {
		return StormByte::Unexpected<StormByte::Network::Exception>(
			std::format("Invalid IP address '{}'", ipstr));
	}

	// Return the resolved address as sockaddr
	auto resolved_sock = std::make_unique<sockaddr_in>(resolved);
	return std::shared_ptr<sockaddr>(reinterpret_cast<sockaddr*>(resolved_sock.release()));
}

void Info::Initialize(std::shared_ptr<sockaddr> sock_addr) noexcept {
	// Get the IP address and port from the socket address
	char ipstr[INET6_ADDRSTRLEN];
	if (sock_addr->sa_family == AF_INET) {
		inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(sock_addr.get())->sin_addr, ipstr, sizeof(ipstr));
		m_ip = ipstr;
		m_port = ntohs(reinterpret_cast<sockaddr_in*>(sock_addr.get())->sin_port);
	}
	else if (sock_addr->sa_family == AF_INET6) {
		inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(sock_addr.get())->sin6_addr, ipstr, sizeof(ipstr));
		m_ip = ipstr;
		m_port = ntohs(reinterpret_cast<sockaddr_in6*>(sock_addr.get())->sin6_port);
	}
}