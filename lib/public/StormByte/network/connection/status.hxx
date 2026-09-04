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

#include <StormByte/visibility.h>

#include <string>

/**
 * @brief Connection types of the Network module.
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum Status
	 * @brief Lifecycle of a connection or listener.
	 */
	enum class STORMBYTE_NETWORK_PUBLIC Status: unsigned short {
		Connected,		///< Connection established
		Disconnected,	///< Closed / idle
		Connecting,		///< Connect or listen in progress
		Disconnecting,	///< Shutdown in progress
		Negotiating,	///< Application-level handshake
		Rejected,		///< Peer or local rejection
		PeerClosed,		///< Peer closed the connection
		Error			///< Error state
	};

	/**
	 * @brief Status as text.
	 * @param status Status value.
	 * @return Human-readable name.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string StatusToString(const Status& status) {
		switch (status) {
			case Status::Connected:		return "Connected";
			case Status::Disconnected:	return "Disconnected";
			case Status::Connecting:	return "Connecting";
			case Status::Disconnecting:	return "Disconnecting";
			case Status::Negotiating:	return "Negotiating";
			case Status::Rejected:		return "Rejected";
			case Status::PeerClosed:	return "PeerClosed";
			case Status::Error:
			default:					return "Error";
		}
	}

	/**
	 * @brief Whether the connection is usable for I/O (Connected or Negotiating).
	 * @param status Status value.
	 * @return true if usable.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC bool IsConnected(const Status& status) noexcept {
		return status == Status::Connected || status == Status::Negotiating;
	}
}
