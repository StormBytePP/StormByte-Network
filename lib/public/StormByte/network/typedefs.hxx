#pragma once

#include <StormByte/buffers/consumer.hxx>
#include <StormByte/buffers/producer.hxx>
#include <StormByte/buffers/simple.hxx>
#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/exception.hxx>

#include <functional>
#include <future>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	class Packet;																														///< The packet class (forward declaration).
	namespace Socket { class Client; }																									///< The client socket class (forward declaration).
	using FutureBuffer = std::future<Buffers::Simple>;																					///< The future data type.
	using PromisedBuffer = std::promise<Buffers::Simple>;																				///< The promised data type.
	using ExpectedFutureBuffer = StormByte::Expected<FutureBuffer, ConnectionError>;													///< The expected future type.
	using ExpectedVoid = StormByte::Expected<void, ConnectionError>;																	///< The expected void type.
	using ExpectedClient = StormByte::Expected<Socket::Client, ConnectionError>;														///< The expected client type.
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;											///< The expected read result type.
	using ExpectedHandlerType = StormByte::Expected<Connection::Handler::Type, ConnectionError>;										///< The expected handler type.
	using FutureBufferProcessor = std::function<StormByte::Expected<FutureBuffer, ConnectionError>(Socket::Client&, FutureBuffer&)>;	///< The future data function type.
	using SharedConsumerBuffer = std::shared_ptr<Buffers::Consumer>;																	///< The shared consumer buffer type.
	using SharedProducerBuffer = std::shared_ptr<Buffers::Producer>;																	///< The shared producer buffer type.
	using ExpectedPacket = StormByte::Expected<std::shared_ptr<Packet>, PacketError>;													///< The expected packet type.
	using PacketInstanceFunction = std::function<std::shared_ptr<Packet>(const unsigned short&)>;										///< The packet instance function type.
}