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

#include <string>

/**
 * @brief Connection types of the Network module.
 */
namespace StormByte::Network::Connection {
	/**
	 * @brief Read-side result codes.
	 */
	namespace Read {
		/**
		 * @enum Result
		 * @brief Outcome of a wait/read.
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
	 * @brief Write-side result codes.
	 */
	namespace Write {
		/**
		 * @enum Result
		 * @brief Outcome of a write.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,	///< Write completed
			Failed		///< Write failed
		};
	}

	/**
	 * @brief Read result as text.
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
	 * @brief Write result as text.
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
