#include <StormByte/network/address.hxx>

#ifdef LINUX
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#else
#include <ws2tcpip.h>
#endif

using namespace StormByte::Network;

Address::Address(const std::string& host, const unsigned short& port): m_host(host), m_port(port), m_addr(nullptr) {
	Initialize();
}

Address::Address(std::string&& host, unsigned short&& port): m_host(std::move(host)), m_port(std::move(port)), m_addr(nullptr) {
	Initialize();
}

Address::Address(const Address& other): m_host(other.m_host), m_port(other.m_port), m_addr(nullptr) {
	if (other.m_addr) {
        // Allocate memory for the copy
        m_addr = reinterpret_cast<sockaddr*>(new sockaddr_storage);

        // Perform a deep copy of the contents
        std::memcpy(m_addr, other.m_addr, sizeof(sockaddr_storage));
    }
}

Address::Address(Address&& other) noexcept: m_host(std::move(other.m_host)), m_port(std::move(other.m_port)), m_addr(nullptr) {
	// Move the address
	m_addr = other.m_addr;
	other.m_addr = nullptr;
}

Address& Address::operator=(const Address& other) {
	if (this != &other) {
		// Copy the host and port
		m_host = other.m_host;
		m_port = other.m_port;

		// Free the old address
		Cleanup();

		// Copy the address
		if (other.m_addr) {
			// Allocate memory for the copy
			m_addr = reinterpret_cast<sockaddr*>(new sockaddr_storage);

			// Perform a deep copy of the contents
			std::memcpy(m_addr, other.m_addr, sizeof(sockaddr_storage));
		}
	}

	return *this;
}

Address& Address::operator=(Address&& other) noexcept {
	if (this != &other) {
		// Move the host and port
		m_host = std::move(other.m_host);
		m_port = std::move(other.m_port);

		// Free the old address
		Cleanup();

		// Move the address
		m_addr = other.m_addr;
		other.m_addr = nullptr;
	}

	return *this;
}

Address::~Address() noexcept {
	Cleanup();
}

const std::string& Address::Host() const noexcept {
	return m_host;
}

const unsigned short& Address::Port() const noexcept {
	return m_port;
}

bool Address::IsValid() const noexcept {
	return m_addr != nullptr;
}

const sockaddr* Address::SockAddr() const noexcept {
	return m_addr;
}

void Address::Initialize() noexcept {
    #ifdef LINUX
        struct addrinfo hints{}, *res = nullptr;

        hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // For TCP connections

        // Resolve the hostname
        int ret = getaddrinfo(m_host.c_str(), nullptr, &hints, &res);
        if (ret != 0 || !res) {
            if (res) freeaddrinfo(res); // Free results if allocated
            return; // Return on failure
        }

        // Allocate memory for the sockaddr and copy the result
        m_addr = reinterpret_cast<sockaddr*>(new sockaddr_storage);
        if (res->ai_addrlen > sizeof(sockaddr_storage)) {
            freeaddrinfo(res);
            return; // Prevent overflow
        }
        std::memcpy(m_addr, res->ai_addr, res->ai_addrlen);

        freeaddrinfo(res); // Free the original results
    #else
        WSADATA wsaData;
        struct addrinfo hints{}, *res = nullptr;

        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return; // Return on failure
        }

        hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // For TCP connections

        // Resolve the hostname
        int ret = getaddrinfo(m_host.c_str(), nullptr, &hints, &res);
        if (ret != 0 || !res) {
            if (res) freeaddrinfo(res); // Free results if allocated
            WSACleanup(); // Cleanup Winsock
            return; // Return on failure
        }

        // Allocate memory for the sockaddr and copy the result
        m_addr = reinterpret_cast<sockaddr*>(new sockaddr_storage);
        if (res->ai_addrlen > sizeof(sockaddr_storage)) {
            freeaddrinfo(res);
            WSACleanup();
            return; // Prevent overflow
        }
        std::memcpy(m_addr, res->ai_addr, res->ai_addrlen);

        freeaddrinfo(res); // Free the original results
        WSACleanup(); // Cleanup Winsock
    #endif
}

void Address::Cleanup() noexcept {
	if (m_addr) {
		delete reinterpret_cast<sockaddr_storage*>(m_addr);
		m_addr = nullptr;
	}
}