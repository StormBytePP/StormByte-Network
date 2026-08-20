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
