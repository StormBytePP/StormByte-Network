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

#include <iostream>

constexpr const std::size_t BUFFER_SIZE = 4096;

using namespace StormByte::Network::Socket;

Client::Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler)
	: Socket(protocol, handler) {
	std::cout << "Client created with protocol: " << Connection::ToString(protocol) << std::endl;
}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
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

	m_conn_info = std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));

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

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Send(const Packet& packet) noexcept {
	if (m_status != Connection::Status::Connected) {
		return StormByte::Unexpected<ConnectionError>("Failed to send: Client is not connected");
	}

	const Util::Buffer& buffer = packet.Data();
	std::cout << "Total data size to send: " << buffer.Size() << " bytes" << std::endl;

	std::size_t total_bytes_sent = 0;

	while (!buffer.End()) {
		// Always use CHUNK_SIZE for sending, unless less data is left
		const std::size_t available_size = buffer.Size() - buffer.Position();
		const std::size_t chunk_size = std::min(BUFFER_SIZE, available_size);

		// Read a chunk of data
		auto chunk = buffer.Read(chunk_size);
		if (!chunk) {
			return StormByte::Unexpected<ConnectionError>(
				std::format("Failed to send: {}", chunk.error()->what()));
		}

		// Send the chunk
		const ssize_t written = send(*m_handle, reinterpret_cast<const char*>(chunk->data()), chunk->size(), 0);

		if (written <= 0) {
			return StormByte::Unexpected<ConnectionError>(std::format("Failed to write: {} (error code: {})",
																	m_conn_handler->LastError(),
																	m_conn_handler->LastErrorCode()));
		}

		total_bytes_sent += written;
		std::cout << "Chunk sent successfully. Total bytes sent: " << total_bytes_sent << "/" << buffer.Size() << " bytes" << std::endl;
	}

	std::cout << "All data sent successfully!" << std::endl;
	return {};
}

StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError> Client::Receive() noexcept {
	return Receive(0);
}

StormByte::Expected<std::future<StormByte::Util::Buffer>, StormByte::Network::ConnectionError> Client::Receive(const std::size_t& max_size) noexcept {
	std::cout << "Initiating receive operation with max_size: " << max_size << " bytes" << std::endl;

	try {
		// Create promise and future for asynchronous read
		auto promise = std::make_shared<std::promise<Util::Buffer>>();
		auto future = promise->get_future();

		// Start asynchronous read in a detached thread
		std::thread([this, promise, max_size]() {
			std::cout << "Read thread started. Max size: " << max_size << std::endl;

			try {
				Read(*promise, max_size); // Call the Read function with max_size
				std::cout << "Read thread completed successfully." << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Exception in Read thread: " << e.what() << std::endl;
				promise->set_exception(std::make_exception_ptr(ConnectionError(std::format("Exception during read: {}", e.what()))));
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
		// Calculate the number of bytes to read in this iteration
		const std::size_t bytes_to_read = (max_size > 0) ? std::min(BUFFER_SIZE, max_size - total_bytes_read) : BUFFER_SIZE;

		// Attempt to read from the socket
		const ssize_t valread = recv(*m_handle, internal_buffer, bytes_to_read, 0);

		if (valread > 0) {
			// Successfully read data
			std::cout << "Chunk received. Size: " << valread << " bytes" << std::endl;
			buffer << std::string(internal_buffer, valread);
			total_bytes_read += valread;

			// Exit if we've reached max_size (only if max_size > 0)
			if (max_size > 0 && total_bytes_read >= max_size) {
				std::cout << "Reached requested max_size: " << total_bytes_read << " bytes. Exiting loop." << std::endl;
				break;
			}
		} else if (valread == 0) {
			// Connection was closed by the peer
			std::cout << "Connection closed by peer. Exiting read loop." << std::endl;
			break;
		} else {
#ifdef WINDOWS
			if (m_conn_handler->LastErrorCode() == WSAEWOULDBLOCK) {
#else
			if (m_conn_handler->LastErrorCode() == EAGAIN || m_conn_handler->LastErrorCode() == EWOULDBLOCK) {
#endif
				// Socket would block, retry
				std::cout << "Socket would block, retrying..." << std::endl;

				// Exit the loop if total_bytes_read is already sufficient and no further data is available
				if (max_size == 0 && total_bytes_read > 0) {
					std::cout << "No additional data expected and socket would block. Exiting loop." << std::endl;
					break;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			} else {
				// An error occurred
				std::cerr << "Read error: " << m_conn_handler->LastError() << std::endl;
				promise.set_exception(std::make_exception_ptr(ConnectionError(
					std::format("Receive failed: {}", m_conn_handler->LastError()))));
				return;
			}
		}
	}

	// Log the total size of data received
	std::cout << "Total data received: " << buffer.Size() << " bytes" << std::endl;

	// Set the promise with the accumulated buffer
	promise.set_value(buffer);
}

StormByte::Expected<void, StormByte::Network::ConnectionError> Client::Write(const Util::Buffer& buffer, const std::size_t& size) noexcept {
	std::cout << "Starting to write data..." << std::endl;

	if (m_status != Connection::Status::Connected) {
		std::cerr << "Failed to write: Client is not connected" << std::endl;
		return StormByte::Unexpected<ConnectionError>("Failed to write: Client is not connected");
	}

	auto data = buffer.Read(size);
	if (!data) {
		std::cerr << "Failed to read buffer data for write: " << data.error()->what() << std::endl;
		return StormByte::Unexpected<ConnectionError>("Failed to write: Buffer read overflow");
	}

	const char* raw_data = reinterpret_cast<const char*>(data->data());
	const ssize_t written = send(*m_handle, raw_data, size, 0);

	if (written != static_cast<ssize_t>(size)) {
		std::cerr << "Write failed: " << m_conn_handler->LastError() << " (code: " << m_conn_handler->LastErrorCode() << ")" << std::endl;
		return StormByte::Unexpected<ConnectionError>(std::format("Write failed: {} (error code: {})",
																m_conn_handler->LastError(),
																m_conn_handler->LastErrorCode()));
	}

	std::cout << "Write of size " << size << " bytes completed successfully" << std::endl;
	return {};
}
