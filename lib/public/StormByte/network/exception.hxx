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

#include <StormByte/exception.hxx>
#include <StormByte/network/visibility.h>

/**
 * @namespace Network
 * @brief StormByte networking subsystem.
 */
namespace StormByte::Network {
	/**
	 * @class Exception
	 * @brief Base exception for the network module.
	 */
	class STORMBYTE_NETWORK_PUBLIC Exception: public StormByte::Exception {
		public:
			/**
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
	 * @brief Connection or socket operation failure.
	 */
	class ConnectionError: public Exception {
		public:
			/**
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
	 * @brief Connection closed while waiting or transferring.
	 */
	class STORMBYTE_NETWORK_PUBLIC ConnectionClosed final: public Exception {
		public:
			/**
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
