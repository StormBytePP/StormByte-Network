#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/transport/frame.hxx>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	class STORMBYTE_NETWORK_PRIVATE Client final {
		public:
			/**
			 * @brief Constructor for the Client connection.
			 * @param socket The underlying socket client.
			 * @param in_pipeline The input buffer pipeline.
			 * @param out_pipeline The output buffer pipeline.
			 */
			Client(std::shared_ptr<Socket::Client> socket, Buffer::Pipeline in_pipeline, Buffer::Pipeline out_pipeline) noexcept;

			/**
			 * @brief Copy constructor (deleted)
			 * @param other The Client to copy from.
			 */
			Client(const Client& other) 								= delete;

			/**
			 * @brief Move constructor.
			 * @param other The Client to move from.
			 */
			Client(Client&& other) noexcept 							= default;

			/**
			 * @brief Destructor.
			 */
			~Client() noexcept 											= default;

			/**
			 * @brief Copy-assignment operator (deleted)
			 * @param other The Client to copy from.
			 * @return Reference to this Client.
			 */
			Client& operator=(const Client& other) 						= delete;

			/**
			 * @brief Move-assignment operator.
			 * @param other The Client to move from.
			 * @return Reference to this Client.
			 */
			Client& operator=(Client&& other) noexcept 					= default;

			/**
			 * @brief Get the input buffer pipeline.
			 * @return Reference to the input pipeline.
			 */
			inline Buffer::Pipeline& 									InputPipeline() noexcept {
				return m_in_pipeline;
			}

			/**
			 * @brief Get the output buffer pipeline.
			 * @return Reference to the output pipeline.
			 */
			inline Buffer::Pipeline& 									OutputPipeline() noexcept {
				return m_out_pipeline;
			}

			/**
			 * @brief Get the underlying socket client.
			 * @return Shared pointer to the socket client.
			 */
			inline std::shared_ptr<Socket::Client>& 					Socket() noexcept {
				return m_socket;
			}

			/**
			 * @brief Send a Frame to the connected client.
			 * @param frame The Frame to send.
			 * @param logger The logger to use.
			 * @return The expected result of the operation.
			 */
			bool 														Send(const Transport::Frame& frame, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Get the current connection status.
			 * @return The current connection status.
			 */
			inline Connection::Status 									Status() const noexcept {
				return m_socket ? m_socket->Status() : Connection::Status::Disconnected;
			}
			
			/**
			 * @brief Receive a Frame from the connected client.
			 * @param logger The logger to use.
			 * @return The received Frame.
			 */
			Transport::Frame 											Receive(std::shared_ptr<Logger::Log> logger) noexcept;

		private:
			std::shared_ptr<Socket::Client> m_socket;					///< The underlying socket.
			Buffer::Pipeline m_in_pipeline;								///< The input pipeline.
			Buffer::Pipeline m_out_pipeline;							///< The output pipeline.
	};
}