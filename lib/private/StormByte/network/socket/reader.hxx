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
	 * @class Reader
	 * @brief @ref Buffer::ExternalReader adapter over a socket @ref Client.
	 *
	 * Socket I/O is inherently destructive (bytes leave the kernel buffer).
	 * @c Read / @c Extract behave the same; @c Peek is not supported and fails.
	 */
	class STORMBYTE_NETWORK_PRIVATE Reader final: public Buffer::ExternalReader {
		public:
			/**
			 * @brief Construct a reader bound to @p client.
			 * @param client Non-owning client socket.
			 */
			inline Reader(Client& client) noexcept
				: m_client(client) {}

			Reader(const Reader& other) noexcept				= default;
			Reader(Reader&& other) noexcept					= default;
			~Reader() noexcept override						= default;

			Reader& operator=(const Reader& other) noexcept	= default;
			Reader& operator=(Reader&& other) noexcept		= default;

			inline PointerType Clone() const noexcept override {
				return MakePointer<Reader>(*this);
			}

			inline PointerType Move() noexcept override {
				return MakePointer<Reader>(std::move(*this));
			}

			/** @name Queries */
			/** @{ */

			/**
			 * @brief Bytes known to be available without blocking.
			 * @return Always @c 0 for this adapter (sockets do not expose a cheap reliable count).
			 * @note Callers should @ref Read with an explicit size or loop until @ref EoF.
			 */
			std::size_t AvailableBytes() const noexcept override;

			/**
			 * @brief Whether no buffered application data is pending.
			 * @return @c true (same limitation as @ref AvailableBytes).
			 */
			bool Empty() const noexcept override;

			/**
			 * @brief End-of-stream (connection no longer readable).
			 */
			bool EoF() const noexcept override;

			/**
			 * @brief Whether further reads may succeed.
			 */
			bool IsReadable() const noexcept override;

			/** @} */

			/** @name Read / Extract / Peek */
			/** @{ */

			/**
			 * @brief Receive up to @p bytes into @p out (const interface required by ExternalReader).
			 * @param bytes Requested size; implementation forwards to @c Client::Receive.
			 * @param out   Destination buffer (appended/filled by Extract from the receive buffer).
			 * @return @c true on success, @c false on error or disconnect.
			 */
			bool Read(std::size_t bytes, Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Same as @ref Read (socket receives are destructive).
			 */
			bool Extract(std::size_t count, Buffer::DataType& out) noexcept override;

			/**
			 * @brief Not supported on raw sockets without MSG_PEEK plumbing.
			 * @return Always @c false.
			 */
			bool Peek(std::size_t count, Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Read until the peer closes or an error occurs.
			 */
			void ReadUntilEoF(Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Same as @ref ReadUntilEoF for sockets.
			 */
			void ExtractUntilEoF(Buffer::DataType& out) noexcept override;

			/** @} */

		private:
			std::reference_wrapper<Client> m_client;	///< Non-owning reference to the client socket.
	};
}
