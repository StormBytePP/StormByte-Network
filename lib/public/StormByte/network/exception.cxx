#include <StormByte/network/exception.hxx>

#include <format>

using namespace StormByte::Network;

Exception::Exception(const std::string& message): StormByte::Exception(message) {}

ConnectionError::ConnectionError(const std::string& message): Exception(std::format("Connection error: {}", message)) {}

ConnectionClosed::ConnectionClosed(): Exception("Connection closed by client") {}