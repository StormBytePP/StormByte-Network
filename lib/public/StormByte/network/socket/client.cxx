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
#include <chrono>
#include <format>
#include <future>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Adjust BUFFER_SIZE as needed.
constexpr const std::size_t BUFFER_SIZE = 4096;

using namespace StormByte::Network::Socket;

Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler)
	: Socket(protocol, handler) {
	std::cout << "Client created with protocol: " << Connection::ToString(protocol) << std::endl;
}

StormByte::Expected<void, StormByte::Network::ConnectionError>
Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	std::cout << "Connecting to " << hostname << ":" << port << std::endl;

	if (m_status != Connection::Status::Disconnected) {
		std::cerr << "Client is already connected" << std::endl;
		return StormByte::Unexpected<ConnectionError>("Client is already connected");
	}

	m_status = Connection::Status::Connecting;

	auto expected_socket = CreateSocket();
	if (!expected_socket) {
		std::cerr << "Failed to create socket: " << expected_socket.error()->what() << std::endl;
		return StormByte::Unexpected(expected_socket.error());
	}

	m_handle = std::make_unique<Connection::Handler::Type>(expected_socket.value());

	auto expected_conn_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
	if (!expected_conn_info) {
		std::cerr << "Failed to resolve host: " << expected_conn_info.error()->what() << std::endl;
		return StormByte::Unexpected<ConnectionError>(expected_conn_info.error()->what());
	}

	m_conn_info =
		std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));

#ifdef WINDOWS
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == SOCKET_ERROR) {
#else
	if (::connect(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr())) == -1) {
#endif
		std::cerr << "Failed to connect: " << m_conn_handler->LastError() << std::endl;
		return StormByte::Unexpected<ConnectionError>(m_conn_handler->LastError());
	}

	InitializeAfterConnect();
	std::cout << "Successfully connected to " << hostname << ":" << port << std::endl;
	return {};
}

//
// Modified Send function that accepts a std::span<const std::byte>
//
StormByte::Expected<void, StormByte::Network::ConnectionError>
Client::Send(std::span<const std::byte> data) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	std::size_t total_bytes_sent = 0;

	// Loop until all data is sent.
	while (!data.empty()) {
		std::size_t chunk_size = std::min(BUFFER_SIZE, data.size());
		std::span<const std::byte> chunk = data.subspan(0, chunk_size);

		const ssize_t written = ::send(*m_handle,
										reinterpret_cast<const char*>(chunk.data()),
										chunk.size(), 0);

		if (written <= 0) {
			return StormByte::Unexpected<ConnectionError>(std::format(
				"Failed to write: {} (error code: {})",
				m_conn_handler->LastError(),
				m_conn_handler->LastErrorCode()));
		}

		total_bytes_sent += written;
		// Advance the span by the number of bytes written.
		data = data.subspan(written);
	}

	std::cout << "All data sent successfully! Total bytes sent: " << total_bytes_sent << " bytes" << std::endl;
	return {};
}

// Optional: Overload to support sending from a Packet object, if Packet::Data()
// can be changed to return a std::span<const std::byte>.
StormByte::Expected<void, StormByte::Network::ConnectionError>
Client::Send(const Packet& packet) noexcept {
	std::span<const std::byte> data = packet.Data();
	return Send(data);
}

//
// Receive methods (unchanged)
//
StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError>
Client::Receive() noexcept {
	return Receive(0);
}

StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError>
Client::Receive(const std::size_t& max_size) noexcept {
	std::cout << "Initiating receive operation with max_size: " << max_size << " bytes" << std::endl;

	try {
		auto promise = std::make_shared<std::promise<Util::Buffer>>();
		auto future = promise->get_future();

		std::thread([this, promise, max_size]() {
			std::cout << "Read thread started. Max size: " << max_size << " bytes" << std::endl;
			try {
				Read(*promise, max_size);
				std::cout << "Read thread completed successfully." << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Exception in Read thread: " << e.what() << std::endl;
				promise->set_exception(std::make_exception_ptr(
					ConnectionError(std::format("Exception during read: {}", e.what()))));
			}
		}).detach();

		std::cout << "Receive operation initiated successfully. Waiting for future..." << std::endl;
		return std::move(future);
	} catch (const std::exception& e) {
		std::cerr << "Failed to initiate receive operation: " << e.what() << std::endl;
		return StormByte::Unexpected<ConnectionError>(std::format("Receive failed: {}", e.what()));
	}
}

void Client::Read(std::promise<Util::Buffer>& promise, std::size_t max_size) noexcept {
	std::cout << "Starting to read data with max_size: " << max_size << " bytes" << std::endl;

	Util::Buffer buffer;
	char internal_buffer[BUFFER_SIZE];
	std::size_t total_bytes_read = 0;

	while (true) {
		const std::size_t bytes_to_read = (max_size > 0) ? std::min(BUFFER_SIZE, max_size - total_bytes_read) : BUFFER_SIZE;
		const ssize_t valread = recv(*m_handle, internal_buffer, bytes_to_read, 0);

		if (valread > 0) {
			std::cout << "Chunk received. Size: " << valread << " bytes" << std::endl;
			buffer << std::string(internal_buffer, static_cast<std::size_t>(valread));
			total_bytes_read += valread;
			if (max_size > 0 && total_bytes_read >= max_size) {
				std::cout << "Reached requested max_size: " << total_bytes_read << " bytes. Exiting loop." << std::endl;
				break;
			}
		} else if (valread == 0) {
			std::cout << "Connection closed by peer. Exiting read loop." << std::endl;
			break;
		} else {
#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
#else
			if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
#endif
				std::cout << "Socket would block, retrying..." << std::endl;
				if (max_size == 0 && total_bytes_read > 0) {
					std::cout << "No additional data expected and socket would block. Exiting loop." << std::endl;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			} else {
				std::cerr << "Read error: " << m_conn_handler->LastError() << std::endl;
				promise.set_exception(std::make_exception_ptr(
					ConnectionError(std::format("Receive failed: {}", m_conn_handler->LastError()))));
				return;
			}
		}
	}

	std::cout << "Total data received: " << buffer.Size() << " bytes" << std::endl;
	promise.set_value(buffer);
}

//
// Modified Write function that accepts a std::span<const std::byte>
//
StormByte::Expected<void, StormByte::Network::ConnectionError>
Client::Write(std::span<const std::byte> data, const std::size_t& size) noexcept {
	std::cout << "Starting to write data..." << std::endl;

	if (m_status != Connection::Status::Connected) {
		std::cerr << "Failed to write: Client is not connected" << std::endl;
		return StormByte::Unexpected<ConnectionError>("Failed to write: Client is not connected");
	}

	// If the passed-in size is larger than the span, adjust accordingly.
	std::size_t bytes_to_write = std::min(size, data.size());
	std::size_t total_written = 0;

	while (total_written < bytes_to_write) {
		auto current_data = data.subspan(total_written);
		const ssize_t written = ::send(*m_handle,
										reinterpret_cast<const char*>(current_data.data()),
										current_data.size(), 0);
		if (written <= 0) {
			std::cerr << "Write failed: " << m_conn_handler->LastError()
					<< " (code: " << m_conn_handler->LastErrorCode() << ")" << std::endl;
			return StormByte::Unexpected<ConnectionError>(std::format(
				"Write failed: {} (error code: {})",
				m_conn_handler->LastError(),
				m_conn_handler->LastErrorCode()));
		}
		total_written += written;
	}

	std::cout << "Write of size " << bytes_to_write << " bytes completed successfully" << std::endl;
	return {};
}
