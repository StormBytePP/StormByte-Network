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

#include <string>

/**
 * @namespace Connection
 * @brief Connection-level types (protocol, status, read/write results).
 */
namespace StormByte::Network::Connection {
	/**
	 * @namespace Read
	 * @brief Read-side result codes.
	 */
	namespace Read {
		/**
		 * @enum Result
		 * @brief Outcome of a wait/read operation.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,			///< Data available or read ok
			WouldBlock,			///< Non-blocking: no data yet
			Closed,				///< Local/socket closed
			Failed,				///< Hard failure
			Timeout,			///< Wait timed out
			ShutdownRequest		///< Peer shutdown detected
		};
	}

	/**
	 * @namespace Write
	 * @brief Write-side result codes.
	 */
	namespace Write {
		/**
		 * @enum Result
		 * @brief Outcome of a write operation.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,	///< Write completed
			Failed		///< Write failed
		};
	}

	/**
	 * Converts a read result to a string.
	 * @param result Read result.
	 * @return Human-readable name.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Read::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Read::Result::Success:			return "Success";
			case StormByte::Network::Connection::Read::Result::WouldBlock:		return "WouldBlock";
			case StormByte::Network::Connection::Read::Result::Failed:			return "Failed";
			case StormByte::Network::Connection::Read::Result::Closed:			return "Closed";
			case StormByte::Network::Connection::Read::Result::Timeout:			return "Timeout";
			case StormByte::Network::Connection::Read::Result::ShutdownRequest:	return "ShutdownRequest";
			default:															return "Unknown";
		}
	}

	/**
	 * Converts a write result to a string.
	 * @param result Write result.
	 * @return Human-readable name.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Write::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Write::Result::Success:	return "Success";
			case StormByte::Network::Connection::Write::Result::Failed:		return "Failed";
			default:														return "Unknown";
		}
	}
}
