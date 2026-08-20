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

#include <StormByte/network/typedefs.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif

#include <string>

/**
 * @namespace Connection
 * @brief Connection helpers (handler, info, client wrapper).
 */
namespace StormByte::Network::Connection {
	/**
	 * @class Handler
	 * @brief Platform network bootstrap and last-error helpers (singleton).
	 *
	 * Performs WSAStartup on Windows. Non-copyable / non-movable.
	 */
	class STORMBYTE_NETWORK_PRIVATE Handler {
		public:
			/**
			 * Copy constructor (deleted).
			 */
			Handler(const Handler& other) = delete;

			/**
			 * Move constructor (deleted).
			 */
			Handler(Handler&& other) noexcept = delete;

			/**
			 * Destructor (WSACleanup on Windows).
			 */
			~Handler() noexcept;

			/**
			 * Copy assignment (deleted).
			 */
			Handler& operator=(const Handler& other) = delete;

			/**
			 * Move assignment (deleted).
			 */
			Handler& operator=(Handler&& other) noexcept = delete;

			/**
			 * @return Global Handler instance.
			 */
			static Handler& Instance() noexcept;

			/**
			 * @return Human-readable last network error (platform-specific).
			 */
			std::string LastError() const noexcept;

			/**
			 * @return Raw last error code (errno / WSAGetLastError).
			 */
			int LastErrorCode() const noexcept;

			/**
			 * Converts a platform error code to a string.
			 * @param errnum Error code.
			 * @return Description, or numeric string on failure.
			 */
			std::string ErrnoToString(int errnum) const noexcept;

		private:
			bool m_initialized = false;	///< Initialization flag
			#ifdef WINDOWS
			WSADATA m_wsaData;			///< Winsock data
			#endif

			/**
			 * Private constructor (singleton).
			 */
			Handler() noexcept;
	};
}
