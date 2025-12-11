#pragma once

#include <StormByte/network/visibility.h>

#ifdef WINDOWS
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum class Protocol
	 * @brief The enum class representing the protocol.
	 */
	enum class STORMBYTE_NETWORK_PUBLIC Protocol: int {
		IPv4 = AF_INET,
		IPv6 = AF_INET6,
	};

	/**
	 * @brief The function to convert the protocol to a string.
	 * @param protocol The protocol to convert.
	 * @return The string representation of the protocol.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ProtocolString(const Protocol& protocol) noexcept {
		switch (protocol) {
			case Protocol::IPv4:
				return "IPv4";
			case Protocol::IPv6:
				return "IPv6";
			default:
				return "Unknown";
		}
	}

	/**
	 * @brief The function to convert the protocol to an integer.
	 * @param protocol The protocol to convert.
	 * @return The integer representation of the protocol.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC int ProtocolInt(const Protocol& protocol) noexcept {
		return static_cast<int>(protocol);
	}
}