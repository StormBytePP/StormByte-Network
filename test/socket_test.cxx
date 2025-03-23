#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/packet.hxx>
#include <thread>
#include <iostream>
#include <StormByte/test_handlers.h>

constexpr const char* host = "localhost";
constexpr const unsigned short port = 6060;
// Github have memory constraints, so we need to reduce the size of the data
constexpr const std::size_t long_data_size = 10000;

/**
 * @class HelloWorldPacket
 * @brief A simple packet containing a "Hello World!" message.
 */
class HelloWorldPacket : public StormByte::Network::Packet {
	public:
		HelloWorldPacket() : Packet(1) {
			m_data << std::string("Hello World!");
		}

		StormByte::Util::Buffer Data() const noexcept override {
			return m_data;
		}

	private:
		StormByte::Util::Buffer m_data;
};

/**
 * @class ReceivedPacket
 * @brief A packet containing received data.
 */
class ReceivedPacket : public StormByte::Network::Packet {
	public:
		ReceivedPacket(const StormByte::Util::Buffer& buff) : Packet(2), m_data(buff) {}

		StormByte::Util::Buffer Data() const noexcept override {
			return m_data;
		}

	private:
		StormByte::Util::Buffer m_data;
};

std::string SpanToString(const std::span<const std::byte>& span) {
	// Ensure span size is appropriate for a string
	return std::string(reinterpret_cast<const char*>(span.data()), span.size());
}

int TestServerClientCommunication() {
	const std::string fn_name = "TestServerClientCommunication";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace StormByte::Network::Socket;
	
		Server server(StormByte::Network::Connection::Protocol::IPv4, handler);
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
		StormByte::Util::Buffer buffer = future_buffer.get();
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
		using namespace StormByte::Network::Socket;
	
		Client client(StormByte::Network::Connection::Protocol::IPv4, handler);
		ASSERT_TRUE(fn_name, client.Connect(host, port));
	
		HelloWorldPacket packet;
		auto send_result = client.Send(packet);
		ASSERT_TRUE(fn_name, send_result);
	
		auto wait_result = client.WaitForData();
		ASSERT_TRUE(fn_name, wait_result);
	
		auto receive_result = client.Receive();
		ASSERT_TRUE(fn_name, receive_result);
	
		auto future_buffer = std::move(receive_result.value());
		StormByte::Util::Buffer buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());
	
		std::string expected = "Hello World!";
		ASSERT_EQUAL(fn_name, expected, SpanToString(buffer.Data()));
	
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

	auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

	const std::string large_data(long_data_size, 'A'); // 100,000 'A' characters

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace StormByte::Network::Socket;

		Server server(StormByte::Network::Connection::Protocol::IPv4, handler);
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
		StormByte::Util::Buffer buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());

		// Validate received data
		ASSERT_EQUAL(fn_name, large_data, SpanToString(buffer.Data()));

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
		using namespace StormByte::Network::Socket;

		Client client(StormByte::Network::Connection::Protocol::IPv4, handler);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		// Create and send data
		StormByte::Util::Buffer large_buffer;
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
		StormByte::Util::Buffer buffer = future_buffer.get();
		ASSERT_FALSE(fn_name, buffer.Empty());

		// Validate echoed data
		ASSERT_EQUAL(fn_name, large_data, SpanToString(buffer.Data()));

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

	auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

	// Start server thread
	std::thread server_thread([&]() -> int {
		using namespace StormByte::Network::Socket;

		Server server(StormByte::Network::Connection::Protocol::IPv4, handler);
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
		StormByte::Util::Buffer buffer_1 = future_buffer_1.get();
		ASSERT_FALSE(fn_name, buffer_1.Empty());

		const std::string expected_part_1 = "Hello";
		ASSERT_EQUAL(fn_name, expected_part_1, SpanToString(buffer_1.Data()));

		// Receive " World!"
		auto receive_result_2 = client.Receive(7); // Receive next 7 bytes
		ASSERT_TRUE(fn_name, receive_result_2);

		auto future_buffer_2 = std::move(receive_result_2.value());
		StormByte::Util::Buffer buffer_2 = future_buffer_2.get();
		ASSERT_FALSE(fn_name, buffer_2.Empty());

		const std::string expected_part_2 = " World!";
		ASSERT_EQUAL(fn_name, expected_part_2, SpanToString(buffer_2.Data()));

		server_processing_done.store(true); // Signal completion

		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		using namespace StormByte::Network::Socket;

		Client client(StormByte::Network::Connection::Protocol::IPv4, handler);
		ASSERT_TRUE(fn_name, client.Connect(host, port));

		// Create and send "Hello" and " World!" separately
		const std::string part_1 = "Hello";
		const std::string part_2 = " World!";
		StormByte::Util::Buffer buffer_1, buffer_2;
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

int main() {
	int result = 0;

	result += TestServerClientCommunication();
	result += TestLargeDataTransmission();
	result += TestPartialReceive();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
