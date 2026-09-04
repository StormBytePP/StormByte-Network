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

#include <StormByte/network/typedefs.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif

#include <string>

/**
 * @brief Connection helpers of the Network module.
 */
namespace StormByte::Network::Connection {
	/**
	 * @class Handler
	 * @brief Platform bootstrap and last-error helpers (singleton).
	 *
	 * WSAStartup on Windows. Non-copyable / non-movable.
	 */
	class STORMBYTE_NETWORK_PRIVATE Handler {
		public:
			/**
			 * @brief Copy constructor (deleted).
			 */
			Handler(const Handler& other) = delete;

			/**
			 * @brief Move constructor (deleted).
			 */
			Handler(Handler&& other) noexcept = delete;

			/**
			 * @brief Destructor (WSACleanup on Windows).
			 */
			~Handler() noexcept;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Handler& operator=(const Handler& other) = delete;

			/**
			 * @brief Move assignment (deleted).
			 */
			Handler& operator=(Handler&& other) noexcept = delete;

			/**
			 * @brief Global instance.
			 * @return Handler.
			 */
			static Handler& Instance() noexcept;

			/**
			 * @brief Last network error as text.
			 * @return Description.
			 */
			std::string LastError() const noexcept;

			/**
			 * @brief Raw last error (errno / WSAGetLastError).
			 * @return Code.
			 */
			int LastErrorCode() const noexcept;

			/**
			 * @brief Platform error code as text.
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
			 * @brief Private constructor (singleton).
			 */
			Handler() noexcept;
	};
}
