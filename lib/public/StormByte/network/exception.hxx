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
			 * @brief The constructor of the Exception class.
			 * @param message The message of the exception.
			 */
			Exception(const std::string& message);

			/**
			 * @brief The constructor of the Exception class.
			 * @param message The message of the exception.
			 */
			Exception(const Exception& other)					= default;

			/**
			 * @brief The constructor of the Exception class.
			 * @param message The message of the exception.
			 */
			Exception(Exception&& other) noexcept				= default;

			/**
			 * @brief The destructor of the Exception class.
			 */
			~Exception() noexcept override						= default;

			/**
			 * @brief The assignment operator of the Exception class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			Exception& operator=(const Exception& other)		= default;

			/**
			 * @brief The assignment operator of the Exception class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			Exception& operator=(Exception&& other) noexcept	= default;
	};

	/**
	 * @class ConnectionError
	 * @brief The class representing an error in the connection.
	 */
	class ConnectionError: public Exception {
		public:
			/**
			 * @brief The constructor of the ConnectionError class.
			 * @param message The address that is invalid.
			 */
			ConnectionError(const std::string& message);

			/**
			 * @brief The constructor of the ConnectionError class.
			 * @param address The address that is invalid.
			 */
			ConnectionError(const ConnectionError& other)					= default;

			/**
			 * @brief The constructor of the ConnectionError class.
			 * @param address The address that is invalid.
			 */
			ConnectionError(ConnectionError&& other) noexcept				= default;

			/**
			 * @brief The destructor of the ConnectionError class.
			 */
			~ConnectionError() noexcept override							= default;

			/**
			 * @brief The assignment operator of the ConnectionError class.
			 * @param other The other invalid address to assign.
			 * @return The reference to the assigned invalid address.
			 */
			ConnectionError& operator=(const ConnectionError& other)		= default;

			/**
			 * @brief The assignment operator of the ConnectionError class.
			 * @param other The other invalid address to assign.
			 * @return The reference to the assigned invalid address.
			 */
			ConnectionError& operator=(ConnectionError&& other) noexcept	= default;
	};

	/**
	 * @class ConnectionClosed
	 * @brief The class representing a terminated connection.
	 */
	class STORMBYTE_NETWORK_PUBLIC ConnectionClosed final: public Exception {
		public:
			/**
			 * @brief The constructor of the ConnectionClosed class.
			 */
			ConnectionClosed();

			/**
			 * @brief The constructor of the ConnectionClosed class.
			 * @param message The message of the exception.
			 */
			ConnectionClosed(const std::string& message);

			/**
			 * @brief The constructor of the ConnectionClosed class.
			 * @param message The message of the exception.
			 */
			ConnectionClosed(const ConnectionClosed& other)					= default;

			/**
			 * @brief The constructor of the ConnectionClosed class.
			 * @param message The message of the exception.
			 */
			ConnectionClosed(ConnectionClosed&& other) noexcept				= default;

			/**
			 * @brief The destructor of the ConnectionClosed class.
			 */
			~ConnectionClosed() noexcept override								= default;

			/**
			 * @brief The assignment operator of the ConnectionClosed class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			ConnectionClosed& operator=(const ConnectionClosed& other)		= default;

			/**
			 * @brief The assignment operator of the ConnectionClosed class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			ConnectionClosed& operator=(ConnectionClosed&& other) noexcept	= default;
	};

	class STORMBYTE_NETWORK_PUBLIC CryptoException final: public Exception {
		public:
			/**
			 * @brief The constructor of the CryptoException class.
			 * @param message The message of the exception.
			 */
			CryptoException(const std::string& message);

			/**
			 * @brief The constructor of the CryptoException class.
			 * @param message The message of the exception.
			 */
			CryptoException(const CryptoException& other)					= default;

			/**
			 * @brief The constructor of the CryptoException class.
			 * @param message The message of the exception.
			 */
			CryptoException(CryptoException&& other) noexcept				= default;

			/**
			 * @brief The destructor of the CryptoException class.
			 */
			~CryptoException() noexcept override							= default;

			/**
			 * @brief The assignment operator of the CryptoException class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			CryptoException& operator=(const CryptoException& other)		= default;

			/**
			 * @brief The assignment operator of the CryptoException class.
			 * @param other The other exception to assign.
			 * @return The reference to the assigned exception.
			 */
			CryptoException& operator=(CryptoException&& other) noexcept	= default;
	};
}