#pragma once

#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/util/buffer.hxx>

#include <future>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Client
	 * @brief The class representing a client socket.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client final: public Socket {
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param protocol The protocol of the socket.
			 * @param handler The handler of the socket.
			 */
			Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler);

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other socket to copy.
			 */
			Client(const Client& other) 									= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other socket to move.
			 */
			Client(Client&& other) noexcept									= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			~Client() noexcept override										= default;

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
			Client& operator=(Client&& other) noexcept						= default;

			/**
			 * @brief The function to connect to a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>						Connect(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief The function to receive data from the socket.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<std::future<Util::Buffer>, ConnectionError>	Receive() noexcept;

			/**
			 * @brief The function to receive data from the socket.
			 * @param size The size of the data to receive.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<std::future<Util::Buffer>, ConnectionError>	Receive(const std::size_t& size) noexcept;

			/**
			 * @brief Function to send data to the socket.
			 * @param packet The packet to send.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>						Send(const Packet& packet) noexcept;

		private:
			/**
			 * @brief Function to read data from the socket.
			 * @param buffer The buffer to read the data into.
			 * @param size The size of the data to read.
			 * @return The result of the operation.
			 */
			Connection::Read::Result										ReadNonBlocking(Util::Buffer& buffer) noexcept;

			/**
			 * @brief Function to read data from the socket.
			 * @param promise The promise to set the buffer to.
			 * @param max_size The maximum size of the data to read.
			 */
			void 															Read(std::promise<Util::Buffer>& promise, std::size_t max_size) noexcept;

			/**
			 * @brief Function to write data to the socket.
			 * @param buffer The buffer to write the data from.
			 * @return The result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>						Write(const Util::Buffer& buffer, const std::size_t& size) noexcept;
	};
}