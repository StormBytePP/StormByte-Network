#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/reader.hxx>

using namespace StormByte::Network::Socket;

std::size_t Reader::AvailableBytes() const noexcept {
	// No portable cheap “bytes in socket buffer” in this adapter yet.
	return 0;
}

bool Reader::Empty() const noexcept {
	return AvailableBytes() == 0;
}

bool Reader::EoF() const noexcept {
	// Treat non-readable connection as end-of-stream.
	return !IsReadable();
}

bool Reader::IsReadable() const noexcept {
	return m_client.get().Status() != Connection::Status::Disconnected;
}

bool Reader::Read(std::size_t bytes, Buffer::DataType& out) const noexcept {
	auto expected_buffer = m_client.get().Receive(bytes);
	if (!expected_buffer)
		return false;
	expected_buffer->Extract(0, out);
	return true;
}

bool Reader::Extract(std::size_t count, Buffer::DataType& out) noexcept {
	// Socket receive already consumes kernel data.
	return Read(count, out);
}

bool Reader::Peek(std::size_t /*count*/, Buffer::DataType& /*out*/) const noexcept {
	// Not supported without Client-level MSG_PEEK support.
	return false;
}

void Reader::ReadUntilEoF(Buffer::DataType& out) const noexcept {
	constexpr std::size_t kChunk = 4096;
	while (IsReadable()) {
		Buffer::DataType chunk;
		if (!Read(kChunk, chunk) || chunk.empty())
			break;
		out.insert(out.end(), chunk.begin(), chunk.end());
	}
}

void Reader::ExtractUntilEoF(Buffer::DataType& out) noexcept {
	ReadUntilEoF(out);
}
