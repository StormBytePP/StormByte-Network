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

#include <StormByte/visibility.h>

#include <string>

/**
 * @namespace Connection
 * @brief Connection-level types (protocol, status, read/write results).
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum Status
	 * @brief Lifecycle state of a connection or listener.
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
	 * Converts Status to a string.
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
	 * @return true if the connection is usable for I/O (Connected or Negotiating).
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC bool IsConnected(const Status& status) noexcept {
		return status == Status::Connected || status == Status::Negotiating;
	}
}
