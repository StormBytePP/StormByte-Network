#pragma once

#include <StormByte/buffer/external.hxx>
#include <StormByte/network/visibility.h>

#include <functional>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	class Client;

	class STORMBYTE_NETWORK_PRIVATE Reader final: public Buffer::ExternalReader {
		public:
			/**
			 * @brief The constructor of the Reader class.
			 * @param client The client socket to read from.
			 */
			inline Reader(Client& client) noexcept:
			m_client(client) {}

			/**
			 * @brief Copy constructor
			 * @param other The other Reader to copy from.
			 */
			Reader(const Reader& other) noexcept				= default;

			/**
			 * @brief Move constructor
			 * @param other The other Reader to move from.
			 */
			Reader(Reader&& other) noexcept						= default;

			/**
			 * @brief Destructor
			 */
			~Reader() noexcept									= default;

			/**
			 * @brief Copy assignment operator
			 * @param other The other Reader to copy from.
			 * @return Reference to this Reader.
			 */
			Reader& operator=(const Reader& other) noexcept		= default;

			/**
			 * @brief Move assignment operator
			 * @param other The other Reader to move from.
			 * @return Reference to this Reader.
			 */
			Reader& operator=(Reader&& other) noexcept			= default;

			/**
			 * @brief Create a copy of this Reader.
			 * @return A unique pointer to the copied Reader.
			 */
			inline PointerType 									Clone() const noexcept override {
				return MakePointer<Reader>(*this);
			}

			/**
			 * @brief Create a moved instance of this Reader.
			 * @return A unique pointer to the moved Reader.
			 */
			inline PointerType 									Move() noexcept override {
				return MakePointer<Reader>(std::move(*this));
			}

			/**
			 * @brief Read data into the provided buffer.
			 * @param bytes Number of bytes to read.
			 * @param out DataType to fill with read data.
			 * @return true if data was successfully read, false otherwise.
			 * @note Const version do the same for sockets than non-const version.
			 */
			inline bool 										Read(std::size_t bytes, Buffer::DataType& out) const noexcept override {
				return const_cast<Reader*>(this)->Read(bytes, out);
			}
			
			/**
			 * @brief Read data into the provided buffer.
			 * @param bytes Number of bytes to read.
			 * @param out DataType to fill with read data.
			 * @return true if data was successfully read, false otherwise.
			 * @note Non-const version do the same as the const version.
			 */
			bool 												Read(std::size_t bytes, Buffer::DataType& out) noexcept override;

		private:
			std::reference_wrapper<Client> m_client;		///< Non owning reference to the client socket.
	};
}