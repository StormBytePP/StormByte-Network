/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte.
 *
 * StormByte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StormByte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte. If not, see <https://www.gnu.org/licenses/>.
 */

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
