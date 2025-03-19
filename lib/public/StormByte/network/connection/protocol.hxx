#pragma once

#include <StormByte/network/visibility.h>

#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing the connection items.
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum class Protocol
	 * @brief The enum class representing the protocol.
	 */
	enum class STORMBYTE_NETWORK_PUBLIC Protocol: int {
		IPv4,
		IPv6,
	};

	/**
	 * @brief The function to convert the protocol to a string.
	 * @param protocol The protocol to convert.
	 * @return The string representation of the protocol.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const Protocol& protocol) noexcept {
		switch (protocol) {
			case Protocol::IPv4:
				return "IPv4";
			case Protocol::IPv6:
				return "IPv6";
			default:
				return "Unknown";
		}
	}
}