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

	class STORMBYTE_NETWORK_PRIVATE Writer final: public Buffer::ExternalWriter {
		public:
			/**
			 * @brief The constructor of the Writer class.
			 * @param client The client socket to read from.
			 */
			inline Writer(Client& client) noexcept:
			m_client(client) {}

			/**
			 * @brief Copy constructor
			 * @param other The other Writer to copy from.
			 */
			Writer(const Writer& other) noexcept				= default;

			/**
			 * @brief Move constructor
			 * @param other The other Writer to move from.
			 */
			Writer(Writer&& other) noexcept						= default;

			/**
			 * @brief Destructor
			 */
			~Writer() noexcept									= default;

			/**
			 * @brief Copy assignment operator
			 * @param other The other Writer to copy from.
			 * @return Reference to this Writer.
			 */
			Writer& operator=(const Writer& other) noexcept		= default;

			/**
			 * @brief Move assignment operator
			 * @param other The other Writer to move from.
			 * @return Reference to this Writer.
			 */
			Writer& operator=(Writer&& other) noexcept			= default;

			/**
			 * @brief Create a copy of this Writer.
			 * @return A unique pointer to the copied Writer.
			 */
			inline PointerType 									Clone() const noexcept override {
				return MakePointer<Writer>(*this);
			}

			/**
			 * @brief Create a copy of this Writer.
			 * @return A unique pointer to the copied Writer.
			 */
			inline PointerType 									Move() noexcept override {
				return MakePointer<Writer>(std::move(*this));
			}

			/**
			 * @brief Write data from the provided data.
			 * @param data DataType containing the data to write.
			 * @return true if data was successfully written, false otherwise.
			 */
			bool 												Write(Buffer::DataType&& data) noexcept override;

		private:
			std::reference_wrapper<Client> m_client;			///< Non owning reference to the client socket.
	};
}