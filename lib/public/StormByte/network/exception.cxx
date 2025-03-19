#include <StormByte/network/exception.hxx>

#include <format>

using namespace StormByte::Network;

Exception::Exception(const std::string& message): StormByte::Exception(message) {}

InvalidAddress::InvalidAddress(const std::string& address): Exception(std::format("The address {} is invalid", address)) {}