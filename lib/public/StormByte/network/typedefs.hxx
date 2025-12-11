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

#include <memory>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {																		///< The connection info class (forward declaration).
	namespace Socket {
		class Socket;																				///< The socket class (forward declaration).
		class Client;																				///< The client socket class (forward declaration).
	}
	namespace Connection {
		#ifdef LINUX
			using HandlerType = int; 																///< The type of the socket.
		#else
			using HandlerType = SOCKET; 															///< The type of the socket.
		#endif
	}
	using ExpectedBuffer = StormByte::Expected<Buffer::FIFO, ConnectionError>;						///< The expected buffer type.
	using ExpectedVoid = StormByte::Expected<void, ConnectionError>;								///< The expected void type.
	using ExpectedClient = StormByte::Expected<std::shared_ptr<Socket::Client>, ConnectionError>;	///< The expected client type.
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;		///< The expected read result type.
	using PacketPointer = std::shared_ptr<Transport::Packet>;										///< The packet pointer type.
	using DeserializePacketFunction = std::function<PacketPointer(
		Transport::Packet::OpcodeType,
		Buffer::Consumer,
		Logger::Log&
	)>; 																							///< The deserialize packet function type.
}