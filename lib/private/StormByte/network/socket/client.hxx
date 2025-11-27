#pragma once

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/network/typedefs.hxx>

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
		 * @param handler The handler of the socket.
		 * @param logger The logger to use.
		 */
		Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

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
		 * @return The expected result of the operation.
		 */
		ExpectedFutureBuffer											Receive() noexcept;

		/**
		 * @brief The function to receive data from the socket.
		 * @param size The size of the data to receive.
		 * @return The expected result of the operation.
		 */
		ExpectedFutureBuffer 											Receive(const std::size_t& size) noexcept;

		/**
		 * @brief Function to send data to the socket using a Packet.
		 * @param packet The packet to send.
		 * @return The expected result of the operation.
		 */
		ExpectedVoid 													Send(const Packet& packet) noexcept;

		/**
		 * @brief Function to send data to the socket using a std::span of bytes.
		 * @param data A view of the data to send.
		 * @return The expected result of the operation.
		 */
		ExpectedVoid 															Send(std::span<const std::byte> data) noexcept;

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
		 * @brief Function to read data from the socket (non-blocking version).
		 * @param buffer The buffer to read the data into.
		 * @param size The requested size of data.
		 * @return The result of the operation.
		 */
		Connection::Read::Result 										ReadNonBlocking(Buffer::FIFO& buffer) noexcept;

		/**
		 * @brief Function to read data from the socket asynchronously.
		 * @param promise The promise to set the buffer to.
		 * @param max_size The maximum size of the data to read.
		 */
		void 															Read(PromisedBuffer& promise, std::size_t max_size) noexcept;

		/**
		 * @brief Function to write data to the socket.
		 * @param data A view of the data to be written.
		 * @param size The number of bytes to write.
		 * @return The expected result of the operation.
		 */
		ExpectedVoid 													Write(std::span<const std::byte> data, const std::size_t& size) noexcept;
	};
}