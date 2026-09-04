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
	 * @class Reader
	 * @brief Buffer::ExternalReader adapter over Client.
	 *
	 * Socket I/O is destructive: Read and Extract are equivalent; Peek fails.
	 */
	class STORMBYTE_NETWORK_PRIVATE Reader final: public Buffer::ExternalReader {
		public:
			/**
			 * @brief Non-owning client reference.
			 * @param client Client.
			 */
			inline Reader(Client& client) noexcept
				: m_client(client) {}

			/**
			 * @brief Copy constructor.
			 */
			Reader(const Reader& other) noexcept = default;

			/**
			 * @brief Move constructor.
			 */
			Reader(Reader&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			~Reader() noexcept override = default;

			/**
			 * @brief Copy assignment.
			 */
			Reader& operator=(const Reader& other) noexcept = default;

			/**
			 * @brief Move assignment.
			 */
			Reader& operator=(Reader&& other) noexcept = default;

			/**
			 * @brief Clone.
			 * @return Pointer.
			 */
			inline PointerType Clone() const noexcept override {
				return MakePointer<Reader>(*this);
			}

			/**
			 * @brief Move into a new pointer.
			 * @return Pointer.
			 */
			inline PointerType Move() noexcept override {
				return MakePointer<Reader>(std::move(*this));
			}

			/**
			 * @brief Always 0 (no cheap available-byte count).
			 * @return 0.
			 */
			std::size_t AvailableBytes() const noexcept override;

			/**
			 * @brief Same limitation as AvailableBytes.
			 * @return true.
			 */
			bool Empty() const noexcept override;

			/**
			 * @brief Whether the connection is no longer readable.
			 * @return true at EoF.
			 */
			bool EoF() const noexcept override;

			/**
			 * @brief Whether further reads may succeed.
			 * @return true if readable.
			 */
			bool IsReadable() const noexcept override;

			/**
			 * @brief Receive up to @p bytes into @p out.
			 * @param bytes Requested size.
			 * @param out Destination.
			 * @return true on success.
			 */
			bool Read(std::size_t bytes, Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Same as Read (destructive receive).
			 * @param count Requested size.
			 * @param out Destination.
			 * @return true on success.
			 */
			bool Extract(std::size_t count, Buffer::DataType& out) noexcept override;

			/**
			 * @brief Not supported on this path.
			 * @return Always false.
			 */
			bool Peek(std::size_t count, Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Read until peer close or error.
			 * @param out Destination.
			 */
			void ReadUntilEoF(Buffer::DataType& out) const noexcept override;

			/**
			 * @brief Same as ReadUntilEoF.
			 * @param out Destination.
			 */
			void ExtractUntilEoF(Buffer::DataType& out) noexcept override;

		private:
			std::reference_wrapper<Client> m_client;	///< Client socket
	};
}
