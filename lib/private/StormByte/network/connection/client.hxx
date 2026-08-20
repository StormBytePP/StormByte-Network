#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/transport/frame.hxx>

/**
 * @namespace Connection
 * @brief Connection helpers (handler, info, client wrapper).
 */
namespace StormByte::Network::Connection {
	/**
	 * @class Client
	 * @brief High-level connection over a Socket::Client with I/O pipelines.
	 */
	class STORMBYTE_NETWORK_PRIVATE Client final {
		public:
			/**
			 * @param socket Underlying socket client.
			 * @param in_pipeline Input pipeline.
			 * @param out_pipeline Output pipeline.
			 */
			Client(std::shared_ptr<Socket::Client> socket, Buffer::Pipeline in_pipeline, Buffer::Pipeline out_pipeline) noexcept;

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
			~Client() noexcept = default;

			/**
			 * Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * @return Input pipeline.
			 */
			inline Buffer::Pipeline& InputPipeline() noexcept {
				return m_in_pipeline;
			}

			/**
			 * @return Output pipeline.
			 */
			inline Buffer::Pipeline& OutputPipeline() noexcept {
				return m_out_pipeline;
			}

			/**
			 * @return Underlying socket client.
			 */
			inline std::shared_ptr<Socket::Client>& Socket() noexcept {
				return m_socket;
			}

			/**
			 * Sends a frame (moves payload through the output pipeline).
			 * @param frame Frame to send (use std::move).
			 * @param logger Logger.
			 * @return true on success.
			 */
			bool Send(Transport::Frame&& frame, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @return Connection status from the socket (or Disconnected).
			 */
			inline Connection::Status Status() const noexcept {
				return m_socket ? m_socket->Status() : Connection::Status::Disconnected;
			}

			/**
			 * Receives one framed message.
			 * @param logger Logger.
			 * @return Frame (empty on failure).
			 */
			Transport::Frame Receive(std::shared_ptr<Logger::Log> logger) noexcept;

		private:
			std::shared_ptr<Socket::Client> m_socket;	///< Socket
			Buffer::Pipeline m_in_pipeline;				///< Input pipeline
			Buffer::Pipeline m_out_pipeline;			///< Output pipeline
	};
}
