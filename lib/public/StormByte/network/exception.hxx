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

	class InvalidAddress: public Exception {
		public:
			/**
			 * @brief The constructor of the InvalidAddress class.
			 * @param address The address that is invalid.
			 */
			InvalidAddress(const std::string& address);

			/**
			 * @brief The constructor of the InvalidAddress class.
			 * @param address The address that is invalid.
			 */
			InvalidAddress(const InvalidAddress& other)					= default;

			/**
			 * @brief The constructor of the InvalidAddress class.
			 * @param address The address that is invalid.
			 */
			InvalidAddress(InvalidAddress&& other) noexcept				= default;

			/**
			 * @brief The destructor of the InvalidAddress class.
			 */
			~InvalidAddress() noexcept override							= default;

			/**
			 * @brief The assignment operator of the InvalidAddress class.
			 * @param other The other invalid address to assign.
			 * @return The reference to the assigned invalid address.
			 */
			InvalidAddress& operator=(const InvalidAddress& other)		= default;

			/**
			 * @brief The assignment operator of the InvalidAddress class.
			 * @param other The other invalid address to assign.
			 * @return The reference to the assigned invalid address.
			 */
			InvalidAddress& operator=(InvalidAddress&& other) noexcept	= default;
	};
}