#pragma once

#include <StormByte/network/exception.hxx>

#include <string>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	class STORMBYTE_NETWORK_PUBLIC Address {
		public:
			/**
			 * @brief The constructor of the Address class.
			 * @param host The address to create.
			 */
			Address(const std::string& host, const unsigned short& port);

			/**
			 * @brief The constructor of the Address class.
			 * @param host The address to create.
			 */
			Address(std::string&& host, unsigned short&& port);

			/**
			 * @brief The copy constructor of the Address class.
			 * @param other The other address to copy.
			 */
			Address(const Address& other)							= default;
			
			/**
			 * @brief The move constructor of the Address class.
			 * @param other The other address to move.
			 */
			Address(Address&& other) noexcept						= default;

			/**
			 * @brief The assignment operator of the Address class.
			 * @param other The other address to assign.
			 * @return The reference to the assigned address.
			 */
			Address& operator=(const Address& other)				= default;

			/**
			 * @brief The move assignment operator of the Address class.
			 * @param other The other address to assign.
			 * @return The reference to the assigned address.
			 */
			Address& operator=(Address&& other) noexcept			= default;

			/**
			 * @brief The destructor of the Address class.
			 */
			virtual ~Address() noexcept								= default;

			/**
			 * @brief The function to get the address
			 * @return The address
			 */
			const std::string& 										Host() const noexcept;

			/**
			 * @brief The function to get the port
			 * @return The port
			 */
			const unsigned short&									Port() const noexcept;

			/**
			 * Is the address valid?
			 * @return True if the address is valid, false otherwise.
			 */
			bool 													IsValid() const noexcept;

		private:
			const std::string m_host;								///< The address of the address.
			const unsigned short m_port;							///< The port of the address.
	};
}