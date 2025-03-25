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
			/**
			 * Constructor
			 */
			using Exception::Exception;
	};

	class STORMBYTE_NETWORK_PUBLIC CryptoException final: public Exception {
		public:
			/**
			 * Constructor
			 */
			using Exception::Exception;
	};
}