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

#include <StormByte/buffer/fifo.hxx>
#include <StormByte/network/visibility.h>
#include <StormByte/serializable.hxx>

/**
 * @namespace Transport
 * @brief Application-layer messages (Packet, Frame) and on-wire layout.
 *
 * Distinct from IP-level protocols (IPv4/IPv6).
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Packet
	 * @brief Polymorphic wire packet: opcode + payload serialization hook.
	 *
	 * Derive and override @ref DoSerialize() for the payload (excluding opcode).
	 * @ref Serialize() writes opcode then payload.
	 *
	 * Opcode values must fit in @ref OpcodeType (unsigned short). Prefer
	 * non-negative enum values convertible to that range.
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			using OpcodeType = unsigned short;	///< Opcode storage type

			/**
			 * Copy constructor.
			 */
			Packet(const Packet& other) = default;

			/**
			 * Move constructor.
			 */
			Packet(Packet&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			virtual ~Packet() noexcept = default;

			/**
			 * Copy assignment.
			 */
			Packet& operator=(const Packet& other) = default;

			/**
			 * Move assignment.
			 */
			Packet& operator=(Packet&& other) noexcept = default;

			/**
			 * @return Stored opcode.
			 */
			inline const OpcodeType& Opcode() const noexcept {
				return m_opcode;
			}

			/**
			 * Serializes opcode followed by @ref DoSerialize() payload.
			 * @return Complete on-wire buffer.
			 */
			Buffer::FIFO Serialize() const noexcept;

			/**
			 * Opcodes at or above this value run payload through Buffer pipelines
			 * (e.g. compression) when framing.
			 */
			static constexpr unsigned short PROCESS_THRESHOLD = 10;

		protected:
			OpcodeType m_opcode;	///< Packet opcode

			/**
			 * @param opcode Packet opcode.
			 */
			constexpr Packet(const OpcodeType& opcode) noexcept:
			m_opcode(opcode) {}

			/**
			 * Payload-only serialization (no opcode).
			 * @return Payload bytes (may be empty).
			 */
			virtual Buffer::DataType DoSerialize() const noexcept = 0;
	};
}
