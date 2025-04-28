#pragma once

#include <StormByte/buffer/consumer.hxx>
#include <StormByte/buffer/producer.hxx>
#include <StormByte/buffer/simple.hxx>
#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>

#include <functional>
#include <future>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	class Packet;																														///< The packet class (forward declaration).
	namespace Socket {	
		class Socket;																													///< The socket class (forward declaration).																							
		class Client;																													///< The client socket class (forward declaration).
	}																																	///< The client socket class (forward declaration).
	using FutureBuffer = std::future<Buffer::Simple>;																					///< The future data type.
	using PromisedBuffer = std::promise<Buffer::Simple>;																				///< The promised data type.
	using ExpectedBuffer = StormByte::Expected<Buffer::Simple, ConnectionError>;														///< The expected buffer type.
	using ExpectedFutureBuffer = StormByte::Expected<FutureBuffer, ConnectionError>;													///< The expected future type.
	using ExpectedVoid = StormByte::Expected<void, ConnectionError>;																	///< The expected void type.
	using ExpectedClient = StormByte::Expected<Socket::Client, ConnectionError>;														///< The expected client type.
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;											///< The expected read result type.
	using ExpectedHandlerType = StormByte::Expected<Connection::Handler::Type, ConnectionError>;										///< The expected handler type.
	using FutureBufferProcessor = std::function<StormByte::Expected<FutureBuffer, ConnectionError>(Socket::Client&, FutureBuffer&)>;	///< The future data function type.
	using SharedConsumerBuffer = std::shared_ptr<Buffer::Consumer>;																	///< The shared consumer buffer type.
	using SharedProducerBuffer = std::shared_ptr<Buffer::Producer>;																	///< The shared producer buffer type.
	using ExpectedPacket = StormByte::Expected<std::shared_ptr<Packet>, PacketError>;													///< The expected packet type.
	using PacketInstanceFunction = std::function<std::shared_ptr<Packet>(const unsigned short&)>;										///< The packet instance function type.
	using PacketReaderFunction = std::function<ExpectedBuffer(const size_t&)>;															///< The packet reader function type.
}