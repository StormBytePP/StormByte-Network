#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/packet.hxx>
#include <thread>
#include <iostream>
#include <StormByte/test_handlers.h>

constexpr const char* host = "localhost";
constexpr const unsigned short port = 6060;

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

int main() {
	int result = 0;

	result += TestServerClientCommunication();

	if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}
