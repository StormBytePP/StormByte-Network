#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/buffer.hxx>

#include <functional>
#include <future>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {		
	namespace Socket { class Client; }																									///< The client socket class (forward declaration).
	using FutureBuffer = std::future<Buffer>;																						///< The future data type.
	using PromisedBuffer = std::promise<Buffer>;																					///< The promised data type.
	using ExpectedFutureBuffer = StormByte::Expected<FutureBuffer, ConnectionError>;													///< The expected future type.
	using ExpectedVoid = StormByte::Expected<void, ConnectionError>;																	///< The expected void type.
	using ExpectedClient = StormByte::Expected<Socket::Client, ConnectionError>;														///< The expected client type.
	using ExpectedReadResult = StormByte::Expected<Connection::Read::Result, ConnectionClosed>;											///< The expected read result type.
	using ExpectedHandlerType = StormByte::Expected<Connection::Handler::Type, ConnectionError>;										///< The expected handler type.
	using FutureBufferProcessor = std::function<StormByte::Expected<FutureBuffer, ConnectionError>(Socket::Client&, FutureBuffer&)>;	///< The future data function type.
}