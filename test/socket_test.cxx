#include <StormByte/network/data/packet.hxx>
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/test_handlers.h>

#include <iostream>
#include <thread>
#include <format>

using namespace StormByte;

constexpr const char* host = "localhost";
constexpr const unsigned short port = 6060;
#if defined(GITHUB_WORKFLOW) || defined(WINDOWS)
constexpr const std::size_t long_data_size = 10000;
#else
constexpr const std::size_t long_data_size = 50000;
#endif
const std::string large_data(long_data_size, 'A');
auto logger = std::make_shared<Logger>(std::cout, Logger::Level::Info);
auto handler = std::make_shared<Network::Connection::Handler>();

/**
 * @class HelloWorldPacket
 * @brief A simple packet containing a "Hello World!" message.
 */
class HelloWorldPacket : public Network::Data::Packet {
	public:
		HelloWorldPacket() : Packet() {}

		void PrepareBuffer() const noexcept override {
			if (m_buffer.Empty()) {
				m_buffer << std::string("Hello World!");
			}
		}
};

/**
 * @class ReceivedPacket
 * @brief A packet containing received data.
 */
class ReceivedPacket : public Network::Data::Packet {
	public:
		ReceivedPacket(const StormByte::Buffers::Simple& buff) : Packet() {
			m_buffer = buff;
		}

		void PrepareBuffer() const noexcept override {}

	private:
		StormByte::Buffers::Simple m_data;
};

int TestServerClientCommunication() {
	const std::string fn_name = "TestServerClientCommunication";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;
	
		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));
	
		server_ready = true;
		auto wait_result = server.WaitForData();
		ASSERT_TRUE(fn_name, wait_result);
	
		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);
	
		Client client = std::move(accept_result.value());
		auto receive_result = client.Receive();
		ASSERT_TRUE(fn_name, receive_result);
	
		auto future_buffer = std::move(receive_result.value());
		StormByte::Buffers::Simple buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());
	
		auto packet = ReceivedPacket(buffer);
		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);
	
		client.Disconnect();
		server_completed = true;
	
		return 0; // Ensure a return value
	});	

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;
	
		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));
	
		HelloWorldPacket packet;
		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);
	
		auto wait_result = client.WaitForData();
		ASSERT_TRUE(fn_name, wait_result);
	
		auto receive_result = client.Receive();
		ASSERT_TRUE(fn_name, receive_result);
	
		auto future_buffer = std::move(receive_result.value());
		StormByte::Buffers::Simple buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());
	
		std::string expected = "Hello World!";
		ASSERT_EQUAL(fn_name, expected, std::string(reinterpret_cast<const char*>(buffer.Data().data()), buffer.Size()));
	
		client_completed = true;
	
		return 0; // Ensure a return value
	});	

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_completed);
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int TestLargeDataTransmission() {
	const std::string fn_name = "TestLargeDataTransmission";

	std::atomic<bool> server_processing_done(false); // Synchronization flag
	bool server_ready = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		// Wait for client connection
		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);

		Client client = std::move(accept_result.value());

		// Receive data from the client
		auto receive_result = client.Receive();
		ASSERT_TRUE(fn_name, receive_result);

		auto future_buffer = std::move(receive_result.value());
		StormByte::Buffers::Simple buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());

		if (large_data.size() != buffer.Data().size()) {
			std::cerr << std::format("Received data does not match expected data: had {} and got {}", buffer.Data().size(), large_data.size()) << std::endl;
			server_processing_done.store(true);
			return 1;
		}
		// Validate received data
		//ASSERT_EQUAL(fn_name, large_data, SpanToString(buffer.Data()));

		// Send the data back to the client
		auto packet = ReceivedPacket(buffer);
		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);

		client.Disconnect();
		server_processing_done.store(true); // Signal completion

		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;

		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		// Create and send data
		StormByte::Buffers::Simple large_buffer;
		large_buffer << large_data;
		ReceivedPacket packet(large_buffer);

		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);

		// Wait for server response
		auto wait_result = client.WaitForData();
		ASSERT_TRUE(fn_name, wait_result);

		auto receive_result = client.Receive();
		ASSERT_TRUE(fn_name, receive_result);

		auto future_buffer = std::move(receive_result.value());
		StormByte::Buffers::Simple buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());

		// Validate echoed data
		if (large_data.size() != buffer.Data().size()) {
			std::cerr << std::format("Received data does not match expected data: had {} and got {}", buffer.Data().size(), large_data.size()) << std::endl;
			server_processing_done.store(true);
			return 1;
		}

		// Wait for server to finish processing
		while (!server_processing_done.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		client_completed = true;

		return 0;
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_processing_done.load());
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int TestPartialReceive() {
	const std::string fn_name = "TestPartialReceive";

	std::atomic<bool> server_processing_done(false); // Synchronization flag
	bool server_ready = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		// Wait for client connection
		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);

		Client client = std::move(accept_result.value());

		// Receive "Hello"
		auto receive_result_1 = client.Receive(5); // Receive first 5 bytes
		ASSERT_TRUE(fn_name, receive_result_1);

		auto future_buffer_1 = std::move(receive_result_1.value());
		StormByte::Buffers::Simple buffer_1 = future_buffer_1.get();
		ASSERT_FALSE(fn_name, buffer_1.Empty());

		const std::string expected_part_1 = "Hello";
		ASSERT_EQUAL(fn_name, expected_part_1, std::string(reinterpret_cast<const char*>(buffer_1.Data().data()), buffer_1.Size()));

		// Receive " World!"
		auto receive_result_2 = client.Receive(7); // Receive next 7 bytes
		ASSERT_TRUE(fn_name, receive_result_2);

		auto future_buffer_2 = std::move(receive_result_2.value());
		StormByte::Buffers::Simple buffer_2 = future_buffer_2.get();
		ASSERT_FALSE(fn_name, buffer_2.Empty());

		const std::string expected_part_2 = " World!";
		ASSERT_EQUAL(fn_name, expected_part_2, std::string(reinterpret_cast<const char*>(buffer_2.Data().data()), buffer_2.Size()));

		server_processing_done.store(true); // Signal completion

		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;

		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		// Create and send "Hello" and " World!" separately
		const std::string part_1 = "Hello";
		const std::string part_2 = " World!";
		StormByte::Buffers::Simple buffer_1, buffer_2;
		buffer_1 << part_1;
		buffer_2 << part_2;

		ReceivedPacket packet_1(buffer_1);
		auto send_result_1 = client.Send(packet_1);
		ASSERT_TRUE(fn_name, send_result_1);

		// Simulate a slight delay between sending the two parts
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ReceivedPacket packet_2(buffer_2);
		auto send_result_2 = client.Send(packet_2);
		ASSERT_TRUE(fn_name, send_result_2);

		// Wait for server to finish processing
		while (!server_processing_done.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		client_completed = true;

		return 0;
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_processing_done.load());
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int TestWaitForDataTimeout() {
	const std::string fn_name = "TestWaitForDataTimeout";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		// Wait for client connection and simulate data unavailability
		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);

		Client client = std::move(accept_result.value());

		// Ensure the client calls WaitForData but no data is sent
		std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate no data for timeout
		server_completed = true;

		return 0; // Ensure a return value
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;

		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		// Use a timeout of 1 second (1,000,000 microseconds) to test WaitForData
		const long long timeout_microseconds = 1000000;
		auto wait_result = client.WaitForData(timeout_microseconds);

		ASSERT_TRUE(fn_name, wait_result.has_value());
		ASSERT_TRUE(fn_name, Network::Connection::Read::Result::Timeout == wait_result.value());

		client_completed = true;

		return 0; // Ensure a return value
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_completed);
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int TestClientDisconnectMidSend() {
	const std::string fn_name = "TestClientDisconnectMidSend";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		// Wait for client connection
		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);

		Client client = std::move(accept_result.value());

		// Try to receive data from client
		auto receive_result = client.Receive();

		// Client may have disconnected during data sending
		if (!receive_result.has_value()) {
			ASSERT_EQUAL(fn_name, receive_result.error()->what(), std::string("Connection closed by peer"));
		} else {
			auto future_buffer = std::move(receive_result.value());
			StormByte::Buffers::Simple buffer = future_buffer.get();
			ASSERT_TRUE(fn_name, !buffer.Empty()); // Data should be incomplete due to client disconnect
		}		

		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;

		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		const std::string partial_data = "Partial Data...";
		StormByte::Buffers::Simple buffer;
		buffer << partial_data;

		// Send partial data
		ReceivedPacket packet(buffer);
		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);

		// Simulate client disconnecting mid-communication
		client.Disconnect();
		client_completed = true;

		return 0;
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_completed);
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int TestSimultaneousConnections() {
	const std::string fn_name = "TestSimultaneousConnections";

	constexpr int client_count = 5;
	std::atomic<int> connected_clients(0);
	bool server_ready = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		// Accept multiple client connections
		for (int i = 0; i < client_count; ++i) {
			auto accept_result = server.Accept();
			ASSERT_TRUE(fn_name, accept_result);

			// Disconnect clients immediately after connection
			Client client = std::move(accept_result.value());
			client.Disconnect();
			++connected_clients;
		}

		return 0;
	});

	// Wait for the server to be ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start multiple client threads
	std::vector<std::thread> client_threads;
	for (int i = 0; i < client_count; ++i) {
		client_threads.emplace_back([&, i]() -> int {
			using namespace Network::Socket;

			Client client(Network::Connection::Protocol::IPv4, handler, logger);
			ASSERT_TRUE(fn_name, client.Connect(host, port));
			client.Disconnect();
			return 0;
		});
	}

	// Join client threads
	for (auto& thread : client_threads) {
		thread.join();
	}

	// Join server thread
	server_thread.join();

	// Final assertions
	ASSERT_EQUAL(fn_name, client_count, connected_clients.load());

	RETURN_TEST(fn_name, 0);
}

int TestInvalidHostname() {
	using namespace Network::Socket;

	const std::string fn_name = "TestInvalidHostname";

	Client client(Network::Connection::Protocol::IPv4, handler, logger);
	auto result = client.Connect("invalid.hostname", port);

	ASSERT_FALSE(fn_name, result); // Connection should fail

	RETURN_TEST(fn_name, 0);
}

int TestDataFragmentation() {
	const std::string fn_name = "TestDataFragmentation";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace Network::Socket;

		Server server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Listen(host, port));

		server_ready = true;

		auto accept_result = server.Accept();
		ASSERT_TRUE(fn_name, accept_result);

		Client client = std::move(accept_result.value());

		// Receive fragmented data
		const std::string expected_data = "Hello Fragmented World!";
		StormByte::Buffers::Simple received_buffer;

		while (received_buffer.Size() < expected_data.size()) {
			auto receive_result = client.Receive(5); // Receive chunks of 5 bytes
			ASSERT_TRUE(fn_name, receive_result);

			auto future_buffer = std::move(receive_result.value());
			StormByte::Buffers::Simple chunk_buffer = future_buffer.get();
			received_buffer << chunk_buffer;
		}

		ASSERT_EQUAL(fn_name, expected_data, std::string(reinterpret_cast<const char*>(received_buffer.Data().data()), received_buffer.Size()));
		server_completed = true;
		return 0;
	});

	// Wait for the server to be ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace Network::Socket;

		Client client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		const std::string fragmented_data = "Hello Fragmented World!";
		for (std::size_t i = 0; i < fragmented_data.size(); i += 5) {
			std::string fragment = fragmented_data.substr(i, 5);
			StormByte::Buffers::Simple buffer;
			buffer << fragment;

			ReceivedPacket packet(buffer);
			auto send_result = client.Send(packet);
			ASSERT_TRUE(fn_name, send_result);
		}

		client_completed = true;
		return 0;
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	// Final assertions
	ASSERT_TRUE(fn_name, server_completed);
	ASSERT_TRUE(fn_name, client_completed);

	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestServerClientCommunication();
	result += TestLargeDataTransmission();
	result += TestPartialReceive();
	result += TestWaitForDataTimeout();
	result += TestClientDisconnectMidSend();
	result += TestSimultaneousConnections();
	result += TestInvalidHostname();
	result += TestDataFragmentation();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
