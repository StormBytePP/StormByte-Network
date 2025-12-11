#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/reader.hxx>

using namespace StormByte::Network::Socket;

bool Reader::Read(std::size_t bytes, Buffer::DataType& out) noexcept {
	auto expected_buffer = m_client.get().Receive(bytes);
	if (!expected_buffer) {
		return false;
	}
	expected_buffer->Extract(0, out);
	return true;
}