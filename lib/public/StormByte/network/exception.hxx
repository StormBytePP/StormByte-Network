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

#include <StormByte/exception.hxx>
#include <StormByte/network/visibility.h>

/**
 * @brief Network module of the StormByte suite.
 */
namespace StormByte::Network {
	/**
	 * @class Exception
	 * @brief Base exception for the Network module.
	 */
	class STORMBYTE_NETWORK_PUBLIC Exception: public StormByte::Exception {
		public:
			/**
			 * @brief Construct with a component prefix and a format string.
			 * @tparam Args Format argument types.
			 * @param component Subsystem name.
			 * @param fmt Format string.
			 * @param args Format arguments.
			 */
			template <typename... Args>
			Exception(const std::string& component, std::format_string<Args...> fmt, Args&&... args):
			StormByte::Exception("Network::" + component, fmt, std::forward<Args>(args)...) {}

			using StormByte::Exception::Exception;
	};

	/**
	 * @class ConnectionError
	 * @brief Connection or socket operation failed.
	 */
	class ConnectionError: public Exception {
		public:
			/**
			 * @brief Construct from a format string.
			 * @tparam Args Format argument types.
			 * @param fmt Format string.
			 * @param args Format arguments.
			 */
			template <typename... Args>
			ConnectionError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Connection", fmt, std::forward<Args>(args)...) {}

			using Exception::Exception;
	};

	/**
	 * @class ConnectionClosed
	 * @brief Peer closed while waiting or transferring.
	 */
	class STORMBYTE_NETWORK_PUBLIC ConnectionClosed final: public Exception {
		public:
			/**
			 * @brief Construct from a format string.
			 * @tparam Args Format argument types.
			 * @param fmt Format string.
			 * @param args Format arguments.
			 */
			template <typename... Args>
			ConnectionClosed(std::format_string<Args...> fmt, Args&&... args):
			Exception("Connection: Connection closed. " + std::string(fmt.get()), std::forward<Args>(args)...) {}

			using Exception::Exception;
	};

	/**
	 * @class PacketError
	 * @brief Transport packet error.
	 */
	class STORMBYTE_NETWORK_PUBLIC PacketError final: public Exception {
		public:
			/**
			 * @brief Construct from a format string.
			 * @tparam Args Format argument types.
			 * @param fmt Format string.
			 * @param args Format arguments.
			 */
			template <typename... Args>
			PacketError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Transport::Packet: ", fmt, std::forward<Args>(args)...) {}

			using Exception::Exception;
	};

	/**
	 * @class FrameError
	 * @brief Transport frame error.
	 */
	class STORMBYTE_NETWORK_PUBLIC FrameError final: public Exception {
		public:
			/**
			 * @brief Construct from a format string.
			 * @tparam Args Format argument types.
			 * @param fmt Format string.
			 * @param args Format arguments.
			 */
			template <typename... Args>
			FrameError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Transport::Frame: ", fmt, std::forward<Args>(args)...) {}

			using Exception::Exception;
	};
}
