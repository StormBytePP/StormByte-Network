#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/writer.hxx>

using namespace StormByte::Network::Socket;

bool Writer::Write(Buffer::DataType&& data) noexcept {
	return m_client.get().Send(data).has_value();
}