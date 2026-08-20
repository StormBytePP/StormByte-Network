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
#include <StormByte/expected.hxx>
#include <StormByte/logger/log.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/transport/packet.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif

#include <functional>
#include <memory>

/**
 * @namespace Network
 * @brief StormByte networking subsystem.
 */
namespace StormByte::Network {
	namespace Socket {
		class Socket;	///< Forward declaration
		class Client;	///< Forward declaration
	}

	namespace Connection {
		#ifdef UNIX
			using HandlerType = int;		///< Native socket handle (POSIX)
		#else
			using HandlerType = SOCKET;		///< Native socket handle (Winsock)
		#endif
	}

	using ExpectedBuffer = StormByte::Expected<Buffer::FIFO, ConnectionError>;				///< Receive buffer result
	using ExpectedVoid = StormByte::Expected<void, ConnectionError>;						///< Void operation result
	using ExpectedClient = StormByte::Expected<std::shared_ptr<Socket::Client>, ConnectionError>;	///< Accept result
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;	///< Wait-for-data result
	using PacketPointer = std::shared_ptr<Transport::Packet>;								///< Shared packet

	/**
	 * @brief Callback that builds a Packet from opcode + payload consumer.
	 */
	using DeserializePacketFunction = std::function<PacketPointer(
		Transport::Packet::OpcodeType,
		Buffer::Consumer,
		std::shared_ptr<Logger::Log>
	)>;
}
