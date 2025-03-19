#pragma once

#include <StormByte/network/visibility.h>

#include <string>

struct sockaddr; 	// Forward declaration

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
			Address(const Address& other);
			
			/**
			 * @brief The move constructor of the Address class.
			 * @param other The other address to move.
			 */
			Address(Address&& other) noexcept;

			/**
			 * @brief The assignment operator of the Address class.
			 * @param other The other address to assign.
			 * @return The reference to the assigned address.
			 */
			Address& operator=(const Address& other);

			/**
			 * @brief The move assignment operator of the Address class.
			 * @param other The other address to assign.
			 * @return The reference to the assigned address.
			 */
			Address& operator=(Address&& other) noexcept;

			/**
			 * @brief The destructor of the Address class.
			 */
			virtual ~Address() noexcept;

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

			// /**
			//  * @brief The function to get the socket address
			//  * @return The sock address
			//  */
			const sockaddr*											SockAddr() const noexcept;

		private:
			std::string m_host;										///< The address of the address.
			unsigned short m_port;									///< The port of the address.
			sockaddr* m_addr;										///< The socket address of the address.

			/**
			 * @brief The function to initialize the address
			 */
			void 													Initialize() noexcept;

			/**
			 * @brief The function to cleanup the address
			 */
			void 													Cleanup() noexcept;
	};
}