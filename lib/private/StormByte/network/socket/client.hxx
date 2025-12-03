#pragma once

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/network/typedefs.hxx>

#include <span>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {

	/**
	 * @class Client
	 * @brief The class representing a client socket.
	 */
	class STORMBYTE_NETWORK_PRIVATE Client final: public Socket {
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param protocol The protocol of the socket.
			 * @param logger The logger to use.
			 */
			Client(const Protocol& protocol, Logger::ThreadedLog logger) noexcept;

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other socket to copy.
			 */
			Client(const Client& other) 									= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other socket to move.
			 */
			Client(Client&& other) noexcept 								= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			~Client() noexcept override 									= default;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(const Client& other) 							= delete;

			/**
			 * @brief The move assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(Client&& other) noexcept 						= default;

			/**
			 * @brief The function to connect to a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Connect(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief The function to receive data from the socket.
			 * @param size The size of the data to receive.
			 * @return The expected result of the operation.
			 */
			/**
			 * @brief Receive data (blocking) — kept for compatibility.
			 *
			 * This overload is a convenience wrapper that forwards to the
			 * timeout-enabled variant with `timeout_seconds == 0` (disable
			 * timeout, wait forever).
			 */
			ExpectedBuffer 													Receive(const std::size_t& size = 0) noexcept;

			/**
			 * @brief Receive data with a timeout.
			 *
			 * @param size Maximum number of bytes to receive (0 = unlimited).
			 * @param timeout_seconds Timeout in seconds (0 = wait forever).
			 * @return ExpectedBuffer with the received data or an error on timeout/failure.
			 */
			ExpectedBuffer 													Receive(const std::size_t& size, const unsigned short& timeout_seconds) noexcept;

			/**
			 * @brief Peek data from the socket without consuming it.
			 *
			 * Uses OS-level MSG_PEEK to read up to `size` bytes from the
			 * socket receive buffer without removing them. Returns the data
			 * in a Buffer::FIFO for convenient processing.
			 *
			 * @param size Number of bytes to peek (must be > 0).
			 * @return ExpectedBuffer with the peeked data or an error.
			 */
			ExpectedBuffer 													Peek(const std::size_t& size) noexcept;

			/**
			 * @brief Function to send data to the socket using a FIFO buffer.
			 * @param buffer The buffer containing the data to send.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Send(const Buffer::FIFO& buffer) noexcept;

			/**
			 * @brief Function to send data to the socket using a byte vector
			 * @param buffer The vector containing the data to send.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Send(const std::vector<std::byte>& buffer) noexcept;

			/**
			 * @brief Function to send data to the socket using a byte span.
			 * @param data The span containing the data to send.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Send(std::span<const std::byte> data) noexcept;

			/**
			 * @brief Function to send data to the socket using a Buffer::Consumer.
			 * @param data A consumer of the data to send.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Send(Buffer::Consumer data) noexcept;

			/**
			 * @brief Function to check if a shutdown request has been made.
			 * @return True if a shutdown request has been made, false otherwise.
			 */
			bool 															HasShutdownRequest() noexcept;

			/**
			 * @brief Pings a connected client, updates the connection status, and returns the result.
			 * @return True if the ping was successful, false otherwise.
			 */
			bool 															Ping() noexcept;

		private:
			/**
			 * @brief Perform a single recv with custom flags.
			 *
			 * Wraps ::recv and maps common errors to ExpectedBuffer.
			 * Used by Peek and can be reused by other single-shot reads.
			 */
			ExpectedBuffer 											ReadOnce(const std::size_t& size, int flags) noexcept;
			/**
			 * @brief Function to read data from the socket (non-blocking version).
			 * @param buffer The buffer to read the data into.
			 * @param size The requested size of data.
			 * @return The result of the operation.
			 */
			Connection::Read::Result 										ReadNonBlocking(Buffer::FIFO& buffer) noexcept;

			/**
			 * @brief Function to write data to the socket.
			 * @param data A view of the data to be written.
			 * @param size The number of bytes to write.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid 													Write(std::span<const std::byte> data, const std::size_t& size) noexcept;
	};
}