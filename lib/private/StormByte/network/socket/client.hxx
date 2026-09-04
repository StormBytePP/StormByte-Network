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

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/network/socket/reader.hxx>
#include <StormByte/network/socket/socket.hxx>
#include <StormByte/network/socket/writer.hxx>
#include <StormByte/network/typedefs.hxx>

#include <span>

/**
 * @brief Socket wrappers of the Network module.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Client
	 * @brief Connected client socket (connect, send, receive, peek).
	 */
	class STORMBYTE_NETWORK_PRIVATE Client final: public Socket {
		public:
			/**
			 * @brief Construct with protocol and logger.
			 * @param protocol Address family.
			 * @param logger Logger.
			 */
			Client(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept;

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
			~Client() noexcept override = default;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * @brief Connect to host:port.
			 * @param hostname Host name.
			 * @param port Port.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Connect(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief Reader adapter.
			 * @return Reader.
			 */
			inline Reader Reader() noexcept {
				return { *this };
			}

			/**
			 * @brief Receive up to @p size bytes (no timeout).
			 * @param size Max bytes.
			 * @return Buffer or error.
			 */
			ExpectedBuffer Receive(const std::size_t& size = 0) noexcept;

			/**
			 * @brief Receive with timeout.
			 * @param size Max bytes.
			 * @param timeout_seconds 0 = wait forever between chunks.
			 * @return Buffer or error.
			 */
			ExpectedBuffer Receive(const std::size_t& size, const unsigned short& timeout_seconds) noexcept;

			/**
			 * @brief Receive exactly into @p out (append).
			 * @param size Required byte count.
			 * @param out Destination.
			 * @param timeout_seconds Timeout between chunks (0 = forever).
			 * @return Empty Expected on success.
			 */
			ExpectedVoid ReceiveInto(const std::size_t& size, Buffer::DataType& out, const unsigned short& timeout_seconds = 0) noexcept;

			/**
			 * @brief Peek without consuming (MSG_PEEK).
			 * @param size Bytes to peek.
			 * @return Buffer or error.
			 */
			ExpectedBuffer Peek(const std::size_t& size) const noexcept;

			/**
			 * @brief Send a FIFO buffer.
			 * @param buffer Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(const Buffer::FIFO& buffer) noexcept;

			/**
			 * @brief Send a byte vector.
			 * @param buffer Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(const std::vector<std::byte>& buffer) noexcept;

			/**
			 * @brief Send a byte span.
			 * @param data Data.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(std::span<const std::byte> data) noexcept;

			/**
			 * @brief Send from a Consumer until EoF.
			 * @param data Consumer.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Send(Buffer::Consumer data) noexcept;

			/**
			 * @brief Whether the peer requested shutdown.
			 * @return true if so.
			 */
			bool HasShutdownRequest() noexcept;

			/**
			 * @brief Lightweight connectivity check; may mark Disconnected.
			 * @return true if still up.
			 */
			bool Ping() noexcept;

			/**
			 * @brief Writer adapter.
			 * @return Writer.
			 */
			inline Writer Writer() noexcept {
				return { *this };
			}

		private:
			/**
			 * @brief Single recv with flags.
			 * @param size Max bytes.
			 * @param flags recv flags.
			 * @return Buffer or error.
			 */
			ExpectedBuffer ReadOnce(const std::size_t& size, int flags) noexcept;

			/**
			 * @brief Non-blocking read helper.
			 * @param buffer Destination FIFO.
			 * @return Read result.
			 */
			Connection::Read::Result ReadNonBlocking(Buffer::FIFO& buffer) noexcept;

			/**
			 * @brief Shared receive loop.
			 * @param max_size Cap.
			 * @param out Append target.
			 * @param timeout_seconds Inter-chunk timeout.
			 * @param require_exact Peer close early is error when true.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid ReceiveLoop(const std::size_t& max_size, Buffer::DataType& out, const unsigned short& timeout_seconds, bool require_exact) noexcept;

			/**
			 * @brief Low-level write.
			 * @param data Source span.
			 * @param size Bytes to write.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Write(std::span<const std::byte> data, const std::size_t& size) noexcept;
	};
}
