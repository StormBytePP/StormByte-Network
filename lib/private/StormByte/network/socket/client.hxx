#pragma once

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/network/socket/reader.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/network/socket/writer.hxx>
#include <StormByte/network/typedefs.hxx>

#include <span>

/**
 * @namespace Socket
 * @brief Low-level socket wrappers.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Client
	 * @brief Connected client socket (connect, send, receive, peek).
	 */
	class STORMBYTE_NETWORK_PRIVATE Client final: public Socket {
		public:
			/**
			 * @param protocol Address family.
			 * @param logger Logger.
			 */
			Client(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Copy constructor (deleted).
			 */
			Client(const Client& other) = delete;

			/**
			 * Move constructor.
			 */
			Client(Client&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			~Client() noexcept override = default;

			/**
			 * Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * Connects to host:port.
			 * @param hostname Host name.
			 * @param port Port.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Connect(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @return Reader adapter for this client.
			 */
			inline Reader Reader() noexcept {
				return { *this };
			}

			/**
			 * Receives up to @p size bytes (no timeout).
			 * @param size Max bytes (0 = implementation default / until close policy).
			 * @return Buffer or error.
			 */
			ExpectedBuffer Receive(const std::size_t& size = 0) noexcept;

			/**
			 * Receives with timeout.
			 * @param size Max bytes (0 = unlimited until close if not require_exact).
			 * @param timeout_seconds 0 = wait forever between chunks.
			 * @return Buffer or error.
			 */
			ExpectedBuffer Receive(const std::size_t& size, const unsigned short& timeout_seconds) noexcept;

			/**
			 * Receives exactly into @p out (append).
			 * @param size Required byte count.
			 * @param out Destination.
			 * @param timeout_seconds Timeout between chunks (0 = forever).
			 * @return Empty Expected on success.
			 */
			ExpectedVoid ReceiveInto(const std::size_t& size, Buffer::DataType& out, const unsigned short& timeout_seconds = 0) noexcept;

			/**
			 * Peeks without consuming (MSG_PEEK).
			 * @param size Bytes to peek.
			 * @return Buffer or error.
			 */
			ExpectedBuffer Peek(const std::size_t& size) const noexcept;

			/**
			 * Sends a FIFO buffer.
			 * @param buffer Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(const Buffer::FIFO& buffer) noexcept;

			/**
			 * Sends a byte vector.
			 * @param buffer Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(const std::vector<std::byte>& buffer) noexcept;

			/**
			 * Sends a byte span.
			 * @param data Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(std::span<const std::byte> data) noexcept;

			/**
			 * Sends from a Consumer until EoF.
			 * @param data Consumer.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(Buffer::Consumer data) noexcept;

			/**
			 * @return true if peer has requested shutdown (peek).
			 */
			bool HasShutdownRequest() noexcept;

			/**
			 * Lightweight connectivity check; may mark Disconnected on failure.
			 * @return true if still up.
			 */
			bool Ping() noexcept;

			/**
			 * @return Writer adapter for this client.
			 */
			inline Writer Writer() noexcept {
				return { *this };
			}

		private:
			/**
			 * Single recv with flags.
			 * @param size Max bytes.
			 * @param flags recv flags.
			 * @return Buffer or error.
			 */
			ExpectedBuffer ReadOnce(const std::size_t& size, int flags) noexcept;

			/**
			 * Non-blocking read helper (if used by implementation).
			 * @param buffer Destination FIFO.
			 * @return Read result.
			 */
			Connection::Read::Result ReadNonBlocking(Buffer::FIFO& buffer) noexcept;

			/**
			 * Shared receive loop for Receive / ReceiveInto.
			 * @param max_size Cap (0 = until peer close if !require_exact).
			 * @param out Append target.
			 * @param timeout_seconds Inter-chunk timeout.
			 * @param require_exact Peer close early is error when true.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid ReceiveLoop(const std::size_t& max_size, Buffer::DataType& out, const unsigned short& timeout_seconds, bool require_exact) noexcept;

			/**
			 * Low-level write of @p size bytes from @p data.
			 * @param data Source span.
			 * @param size Bytes to write.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Write(std::span<const std::byte> data, const std::size_t& size) noexcept;
	};
}
