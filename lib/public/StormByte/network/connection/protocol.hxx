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
 * @brief Connection-level types (protocol, status, read/write results).
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum Protocol
	 * @brief Address family for sockets.
	 */
	enum class STORMBYTE_NETWORK_PUBLIC Protocol: int {
		IPv4 = AF_INET,		///< IPv4 (AF_INET)
		IPv6 = AF_INET6,	///< IPv6 (AF_INET6)
	};

	/**
	 * Converts a Protocol to a human-readable string.
	 * @param protocol Protocol value.
	 * @return "IPv4", "IPv6", or "Unknown".
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ProtocolString(const Protocol& protocol) noexcept {
		switch (protocol) {
			case Protocol::IPv4:	return "IPv4";
			case Protocol::IPv6:	return "IPv6";
			default:				return "Unknown";
		}
	}

	/**
	 * Converts a Protocol to the underlying AF_* integer.
	 * @param protocol Protocol value.
	 * @return AF_INET or AF_INET6.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC int ProtocolInt(const Protocol& protocol) noexcept {
		return static_cast<int>(protocol);
	}
}
