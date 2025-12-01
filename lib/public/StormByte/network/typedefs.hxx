#pragma once

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/buffer/producer.hxx>
#include <StormByte/buffer/fifo.hxx>
#include <StormByte/expected.hxx>
#include <StormByte/network/protocol.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif
#include <functional>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {																		///< The connection info class (forward declaration).
	class Packet;																					///< The packet class (forward declaration).
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
	using ExpectedClient = StormByte::Expected<Socket::Client, ConnectionError>;					///< The expected client type.
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;		///< The expected read result type.
	using SharedConsumerBuffer = std::shared_ptr<Buffer::Consumer>;									///< The shared consumer buffer type.
	using SharedProducerBuffer = std::shared_ptr<Buffer::Producer>;									///< The shared producer buffer type.
	using ExpectedPacket = StormByte::Expected<std::shared_ptr<Packet>, PacketError>;				///< The expected packet type.
	using PacketReaderFunction = std::function<ExpectedBuffer(const size_t&)>;						///< The packet reader function type.
}