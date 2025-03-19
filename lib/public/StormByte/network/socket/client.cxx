#include <StormByte/network/socket/client.hxx>

#ifdef LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <algorithm>
#include <format>
#include <thread>
#include <utility>
#include <stdexcept>

constexpr const std::size_t BUFFER_SIZE = 4096;

using namespace StormByte::Network::Socket;

Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler): Socket(protocol, handler) {}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	if (m_status != Connection::Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	m_status = Connection::Status::Connecting;

	auto expected_socket = CreateSocket();
	if (!expected_socket)
		return StormByte::Unexpected(expected_socket.error());

	m_handle = std::make_unique<Connection::Handler::Type>(expected_socket.value());

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
	if (!expected_conn_info)
		return StormByte::Unexpected<ConnectionError>(expected_conn_info.error()->what());

	m_conn_info = std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));

	#ifdef WINDOWS
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == SOCKET_ERROR) {
	#else
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == -1) {
	#endif
		return StormByte::Unexpected<ConnectionError>(m_conn_handler->LastError());
	}

	InitializeAfterConnect();

	return {};
}

StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError> Client::Receive() noexcept {
	return Receive(0);
}

StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError> Client::Receive(const std::size_t& max_size) noexcept {
	try {
		auto promise = std::make_shared<std::promise<Util::Buffer>>();
		auto future = promise->get_future();

		std::thread([this, promise, max_size]() {
			Read(*promise, max_size); // Pass max_size to limit the read
		}).detach();

		return StormByte::Expected<std::future<Util::Buffer>, ConnectionError>{std::move(future)};
	} catch (const std::exception& e) {
		return StormByte::Unexpected<ConnectionError>(std::format("Receive failed: {}", e.what()));
	}
}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Send(const Packet& packet) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	// Retrieve the Util::Buffer from the Packet
	const Util::Buffer& buffer = packet.Data();

	while (!buffer.End()) {
		// Calculate the correct chunk size based on remaining data
		const std::size_t available_size = buffer.Size() - buffer.Position();
		const std::size_t chunk_size = std::min(BUFFER_SIZE, available_size);

		// Read a chunk of data
		auto chunk = buffer.Read(chunk_size);
		if (!chunk) {
			// If Read fails due to BufferOverflow or any other issue, report it as an error
			return StormByte::Unexpected<ConnectionError>(
				std::format("Failed to send: {}", chunk.error()->what()));
		}

		// Use the Write function to send the chunk
		auto result = Write(chunk.value(), chunk->size());
		if (!result) {
			return result; // Propagate the Write error
		}
	}

	return {}; // All chunks sent successfully
}

StormByte::Network::Connection::Read::Result Client::ReadNonBlocking(Util::Buffer& buffer) noexcept {
	if (m_status != Connection::Status::Connected) {
		return Connection::Read::Result::Failed;
	}

	char internal_buffer[BUFFER_SIZE];

	int valread = recv(*m_handle, internal_buffer, BUFFER_SIZE, 0);

	if (valread > 0) {
		buffer << std::string(internal_buffer, valread);
		return Connection::Read::Result::Success;
	} else if (valread == 0) {
		return Connection::Read::Result::Closed;
	} else {
#ifdef WINDOWS
		if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
#else
		if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
#endif
			return Connection::Read::Result::WouldBlock;
		} else {
			return Connection::Read::Result::Failed;
		}
	}
}

void Client::Read(std::promise<Util::Buffer>& promise, std::size_t) noexcept {
	Util::Buffer buffer;
	int retry_count = 0;
	char internal_buffer[BUFFER_SIZE];
	constexpr int MAX_RETRIES = 10; // Limit retries to avoid indefinite waiting

	while (retry_count < MAX_RETRIES) {
		int valread = recv(*m_handle, internal_buffer, BUFFER_SIZE, 0);

		if (valread > 0) {
			buffer << std::string(internal_buffer, valread);
			promise.set_value(buffer);
			return;
		} else if (valread == 0) {
			promise.set_value(buffer);
			return;
		} else {
	#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
	#else
			if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
	#endif
				retry_count++;
				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Add a delay between retries
				continue;
			} else {
				promise.set_exception(std::make_exception_ptr(ConnectionError(
					std::format("Receive failed: {}", m_conn_handler->LastError()))));
				return;
			}
		}
	}

	promise.set_exception(std::make_exception_ptr(ConnectionError("Maximum retries reached: Server did not respond.")));
}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Write(const Util::Buffer& buffer, const std::size_t& size) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to write: Client is not connected");
	}

	// Read the specified size from the buffer
	StormByte::Expected<std::span<const std::byte>, Util::BufferOverflow> data = buffer.Read(size);
	if (!data) {
		return StormByte::Unexpected<ConnectionError>("Failed to write: Buffer read overflow");
	}

	// Extract the raw pointer and cast to const char*
	const char* raw_data = reinterpret_cast<const char*>(data->data());
	const std::size_t to_write = (size > 0) ? size : data->size();

	// Use send to write the data
	const ssize_t written = send(*m_handle, raw_data, to_write, 0);

	if (written == static_cast<ssize_t>(to_write)) {
		return {};
	} else {
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to write: {} (error code: {})",
											m_conn_handler->LastError(),
											m_conn_handler->LastErrorCode()));
	}
}