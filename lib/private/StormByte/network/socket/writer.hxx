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

	/**
	 * @class Writer
	 * @brief @ref Buffer::ExternalWriter adapter over a socket @ref Client.
	 */
	class STORMBYTE_NETWORK_PRIVATE Writer final: public Buffer::ExternalWriter {
		public:
			/**
			 * @brief Construct a writer bound to @p client.
			 * @param client Non-owning client socket.
			 */
			inline Writer(Client& client) noexcept
				: m_client(client) {}

			Writer(const Writer& other) noexcept				= default;
			Writer(Writer&& other) noexcept					= default;
			~Writer() noexcept override						= default;

			Writer& operator=(const Writer& other) noexcept	= default;
			Writer& operator=(Writer&& other) noexcept		= default;

			/**
			 * @brief Create a copy of this Writer.
			 * @return Unique pointer to the copied instance.
			 */
			inline PointerType Clone() const noexcept override {
				return MakePointer<Writer>(*this);
			}

			/**
			 * @brief Create a moved instance of this Writer.
			 * @return Unique pointer to the moved instance.
			 */
			inline PointerType Move() noexcept override {
				return MakePointer<Writer>(std::move(*this));
			}

			/**
			 * @brief Whether the socket still accepts writes.
			 * @return @c false after @ref Close or @ref SetError.
			 */
			bool IsWritable() const noexcept override;

			/**
			 * @brief Send the full contents of @p data.
			 * @param data Bytes to send.
			 * @return @c true on success.
			 */
			bool Write(const Buffer::DataType& data) noexcept override;

			/**
			 * @brief Send the full contents of @p data (move).
			 * @param data Bytes to send.
			 * @return @c true on success.
			 * @note Required pure virtual on @ref Buffer::ExternalWriter.
			 */
			bool Write(Buffer::DataType&& data) noexcept override;

			/**
			 * @brief Send up to @p count bytes from @p data.
			 * @param count Maximum bytes to send.
			 * @param data  Source buffer.
			 * @return @c true on success.
			 */
			bool Write(std::size_t count, const Buffer::DataType& data) noexcept override;

			/**
			 * @brief Send up to @p count bytes from @p data (rvalue).
			 * @param count Maximum bytes to send.
			 * @param data  Source buffer.
			 * @return @c true on success.
			 */
			bool Write(std::size_t count, Buffer::DataType&& data) noexcept override;

			/**
			 * @brief Disallow further writes (local flag).
			 */
			void Close() noexcept override;

			/**
			 * @brief Enter permanent error state (local flag).
			 */
			void SetError() noexcept override;

			/** Convenience overloads from the base (string, span, …). */
			using Buffer::ExternalWriter::Write;

		private:
			std::reference_wrapper<Client> m_client;	///< Non-owning reference to the client socket.
			bool m_writable = true;						///< Cleared by Close / SetError.
			bool m_error    = false;					///< Set by SetError.
	};
}
