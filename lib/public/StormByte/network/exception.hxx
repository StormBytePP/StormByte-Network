#pragma once

#include <StormByte/exception.hxx>
#include <StormByte/network/visibility.h>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Exception
	 * @brief The class representing an exception in the network module.
	 */
	class STORMBYTE_NETWORK_PUBLIC Exception: public StormByte::Exception {
		public:
			template <typename... Args>
			Exception(const std::string& component, std::format_string<Args...> fmt, Args&&... args):
			StormByte::Exception("Network::" + component, fmt, std::forward<Args>(args)...) {}

			/**
			 * Constructor
			 */
			using StormByte::Exception::Exception;
	};

	/**
	 * @class ConnectionError
	 * @brief The class representing an error in the connection.
	 */
	class ConnectionError: public Exception {
		public:
			template <typename... Args>
			ConnectionError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Connection", fmt, std::forward<Args>(args)...) {}

			/**
			 * Constructor
			 */
			using Exception::Exception;
	};

	/**
	 * @class ConnectionClosed
	 * @brief The class representing a terminated connection.
	 */
	class STORMBYTE_NETWORK_PUBLIC ConnectionClosed final: public Exception {
		public:
			template <typename... Args>
			ConnectionClosed(std::format_string<Args...> fmt, Args&&... args):
			Exception("Connection: Connection closed. " + fmt, std::forward<Args>(args)...) {}

			/**
			 * Constructor
			 */
			using Exception::Exception;
	};

	/**
	 * @class PacketError
	 * @brief The class representing an error in the packet.
	 */
	class STORMBYTE_NETWORK_PUBLIC PacketError final: public Exception {
		public:
			template <typename... Args>
			PacketError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Transport::Packet: ", fmt, std::forward<Args>(args)...) {}

			/**
			 * Constructor
			 */
			using Exception::Exception;
	};

	/**
	 * @class FrameError
	 * @brief The class representing an error in the packet.
	 */
	class STORMBYTE_NETWORK_PUBLIC FrameError final: public Exception {
		public:
			template <typename... Args>
			FrameError(std::format_string<Args...> fmt, Args&&... args):
			Exception("Transport::Frame: ", fmt, std::forward<Args>(args)...) {}
			
			/**
			 * Constructor
			 */
			using Exception::Exception;
	};
}