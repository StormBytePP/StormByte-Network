#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/writer.hxx>

#include <algorithm>

using namespace StormByte::Network::Socket;

bool Writer::IsWritable() const noexcept {
	return m_writable && !m_error;
}

bool Writer::Write(const Buffer::DataType& data) noexcept {
	return Write(data.size(), data);
}

bool Writer::Write(Buffer::DataType&& data) noexcept {
	if (!IsWritable())
		return false;
	if (data.empty())
		return true;
	return m_client.get().Send(std::move(data)).has_value();
}

bool Writer::Write(std::size_t count, const Buffer::DataType& data) noexcept {
	if (!IsWritable())
		return false;

	const std::size_t n = std::min(count, data.size());
	if (n == 0)
		return true;

	Buffer::DataType slice(data.begin(),
		data.begin() + static_cast<std::ptrdiff_t>(n));
	return m_client.get().Send(std::move(slice)).has_value();
}

bool Writer::Write(std::size_t count, Buffer::DataType&& data) noexcept {
	if (!IsWritable())
		return false;

	const std::size_t n = std::min(count, data.size());
	if (n == 0)
		return true;

	if (n == data.size())
		return Write(std::move(data));

	return Write(n, static_cast<const Buffer::DataType&>(data));
}

void Writer::Close() noexcept {
	m_writable = false;
}

void Writer::SetError() noexcept {
	m_error    = true;
	m_writable = false;
}
