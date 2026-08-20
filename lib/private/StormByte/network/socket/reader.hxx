/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte.
 *
 * StormByte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StormByte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <StormByte/buffer/external.hxx>
#include <StormByte/network/visibility.h>

#include <functional>

/**
 * @namespace Socket
 * @brief Low-level socket wrappers.
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
			 * @param client Non-owning client reference.
			 */
			inline Reader(Client& client) noexcept
				: m_client(client) {}

			Reader(const Reader& other) noexcept = default;
			Reader(Reader&& other) noexcept = default;
			~Reader() noexcept override = default;

			Reader& operator=(const Reader& other) noexcept = default;
			Reader& operator=(Reader&& other) noexcept = default;

			inline PointerType Clone() const noexcept override {
				return MakePointer<Reader>(*this);
			}

			inline PointerType Move() noexcept override {
				return MakePointer<Reader>(std::move(*this));
			}

			/**
			 * @return Always 0 (no cheap available-byte count).
			 */
			std::size_t AvailableBytes() const noexcept override;

			/**
			 * @return true (same limitation as AvailableBytes).
			 */
			bool Empty() const noexcept override;

			/**
			 * @return true when the connection is no longer readable.
			 */
			bool EoF() const noexcept override;

			/**
			 * @return true if further reads may succeed.
			 */
			bool IsReadable() const noexcept override;

			/**
			 * Receives up to @p bytes into @p out.
			 * @param bytes Requested size.
			 * @param out Destination.
			 * @return true on success.
			 */
			bool Read(std::size_t bytes, Buffer::DataType& out) const noexcept override;

			/**
			 * Same as Read (destructive receive).
			 * @param count Requested size.
			 * @param out Destination.
			 * @return true on success.
			 */
			bool Extract(std::size_t count, Buffer::DataType& out) noexcept override;

			/**
			 * Not supported without MSG_PEEK plumbing on this path.
			 * @return Always false.
			 */
			bool Peek(std::size_t count, Buffer::DataType& out) const noexcept override;

			/**
			 * Reads until peer close or error.
			 * @param out Destination.
			 */
			void ReadUntilEoF(Buffer::DataType& out) const noexcept override;

			/**
			 * Same as ReadUntilEoF.
			 * @param out Destination.
			 */
			void ExtractUntilEoF(Buffer::DataType& out) noexcept override;

		private:
			std::reference_wrapper<Client> m_client;	///< Client socket
	};
}
