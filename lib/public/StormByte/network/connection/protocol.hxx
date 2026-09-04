/*
* Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
*
* This file is part of StormByte-Network.
*
* StormByte-Network is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License version 3
* or later, as published by the Free Software Foundation.
*
* StormByte-Network is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with StormByte-Network. If not, see
* <https://www.gnu.org/licenses/lgpl-3.0.html>.
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
 * @brief Connection types of the Network module.
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
	 * @brief Protocol as text.
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
	 * @brief Protocol as AF_* integer.
	 * @param protocol Protocol value.
	 * @return AF_INET or AF_INET6.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC int ProtocolInt(const Protocol& protocol) noexcept {
		return static_cast<int>(protocol);
	}
}
