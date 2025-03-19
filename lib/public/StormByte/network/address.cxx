#include <StormByte/network/address.hxx>

#ifdef LINUX
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using namespace StormByte::Network;

Address::Address(const std::string& host, const unsigned short& port): m_host(host), m_port(port) {}

Address::Address(std::string&& host, unsigned short&& port): m_host(std::move(host)), m_port(std::move(port)) {}

const std::string& Address::Host() const noexcept {
	return m_host;
}

const unsigned short& Address::Port() const noexcept {
	return m_port;
}

bool Address::IsValid() const noexcept {
	#ifdef LINUX
		struct addrinfo hints{}, *res = nullptr;

		hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; // For TCP connections

		int ret = getaddrinfo(m_host.c_str(), nullptr, &hints, &res);
		freeaddrinfo(res); // Free the results

		if (ret != 0)
			return false;
		else
			return true;
	#else
		WSADATA wsaData;
		struct addrinfo hints{}, *res = nullptr;

		// Initialize Winsock
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			return false;
		}

		hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; // For TCP connections

		// Resolve the hostname
		int ret = getaddrinfo(m_host.c_str(), nullptr, &hints, &res);

		freeaddrinfo(res); // Free the results
		WSACleanup(); // Cleanup Winsock

		if (ret != 0)
			return false;
		else
			return true;
	#endif
}