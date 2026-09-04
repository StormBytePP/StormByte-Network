/*
* Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
*
* This file is part of StormByte-Network.
*
* StormByte-Network is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License version 3
* or later, as published by the Free Software Foundation.
*
* StormByte-Network is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with StormByte-Network. If not, see
* <https://www.gnu.org/licenses/lgpl-3.0.html>.
*/

#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/transport/frame.hxx>

/**
 * @brief Connection helpers of the Network module.
 */
namespace StormByte::Network::Connection {
	/**
	 * @class Client
	 * @brief High-level connection over a Socket::Client with I/O pipelines.
	 */
	class STORMBYTE_NETWORK_PRIVATE Client final {
		public:
			/**
			 * @brief Bind a socket and two pipelines.
			 * @param socket Underlying socket client.
			 * @param in_pipeline Input pipeline.
			 * @param out_pipeline Output pipeline.
			 */
			Client(std::shared_ptr<Socket::Client> socket, Buffer::Pipeline in_pipeline, Buffer::Pipeline out_pipeline) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			Client(const Client& other) = delete;

			/**
			 * @brief Move constructor.
			 */
			Client(Client&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			~Client() noexcept = default;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * @brief Input pipeline.
			 * @return Pipeline.
			 */
			inline Buffer::Pipeline& InputPipeline() noexcept {
				return m_in_pipeline;
			}

			/**
			 * @brief Output pipeline.
			 * @return Pipeline.
			 */
			inline Buffer::Pipeline& OutputPipeline() noexcept {
				return m_out_pipeline;
			}

			/**
			 * @brief Underlying socket client.
			 * @return Socket.
			 */
			inline std::shared_ptr<Socket::Client>& Socket() noexcept {
				return m_socket;
			}

			/**
			 * @brief Send a frame (payload through the output pipeline).
			 * @param frame Frame to send (use std::move).
			 * @param logger Logger.
			 * @return true on success.
			 */
			bool Send(Transport::Frame&& frame, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Status from the socket (or Disconnected).
			 * @return Status.
			 */
			inline Connection::Status Status() const noexcept {
				return m_socket ? m_socket->Status() : Connection::Status::Disconnected;
			}

			/**
			 * @brief Receive one framed message.
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
