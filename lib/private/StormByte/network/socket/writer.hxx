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

#include <StormByte/buffer/external.hxx>
#include <StormByte/network/visibility.h>

#include <functional>

/**
 * @brief Socket wrappers of the Network module.
 */
namespace StormByte::Network::Socket {
	class Client;

	/**
	 * @class Writer
	 * @brief Buffer::ExternalWriter adapter over Client.
	 */
	class STORMBYTE_NETWORK_PRIVATE Writer final: public Buffer::ExternalWriter {
		public:
			/**
			 * @brief Non-owning client reference.
			 * @param client Client.
			 */
			inline Writer(Client& client) noexcept
				: m_client(client) {}

			/**
			 * @brief Copy constructor.
			 */
			Writer(const Writer& other) noexcept = default;

			/**
			 * @brief Move constructor.
			 */
			Writer(Writer&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			~Writer() noexcept override = default;

			/**
			 * @brief Copy assignment.
			 */
			Writer& operator=(const Writer& other) noexcept = default;

			/**
			 * @brief Move assignment.
			 */
			Writer& operator=(Writer&& other) noexcept = default;

			/**
			 * @brief Clone.
			 * @return Pointer.
			 */
			inline PointerType Clone() const noexcept override {
				return MakePointer<Writer>(*this);
			}

			/**
			 * @brief Move into a new pointer.
			 * @return Pointer.
			 */
			inline PointerType Move() noexcept override {
				return MakePointer<Writer>(std::move(*this));
			}

			/**
			 * @brief Whether writes are still allowed.
			 * @return false after Close or SetError.
			 */
			bool IsWritable() const noexcept override;

			/**
			 * @brief Send all of @p data.
			 * @param data Bytes.
			 * @return true on success.
			 */
			bool Write(const Buffer::DataType& data) noexcept override;

			/**
			 * @brief Send all of @p data (move).
			 * @param data Bytes.
			 * @return true on success.
			 */
			bool Write(Buffer::DataType&& data) noexcept override;

			/**
			 * @brief Send up to @p count bytes from @p data.
			 * @param count Max bytes.
			 * @param data Source.
			 * @return true on success.
			 */
			bool Write(std::size_t count, const Buffer::DataType& data) noexcept override;

			/**
			 * @brief Send up to @p count bytes from @p data (rvalue).
			 * @param count Max bytes.
			 * @param data Source.
			 * @return true on success.
			 */
			bool Write(std::size_t count, Buffer::DataType&& data) noexcept override;

			/**
			 * @brief Disallow further writes (local flag).
			 */
			void Close() noexcept override;

			/**
			 * @brief Mark permanent error (local flag).
			 */
			void SetError() noexcept override;

			using Buffer::ExternalWriter::Write;

		private:
			std::reference_wrapper<Client> m_client;	///< Client socket
			bool m_writable = true;						///< Writable flag
			bool m_error = false;						///< Error flag
	};
}
