#include <StormByte/network/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/network/data/packet.hxx>
#include <StormByte/test_handlers.h>

#include <thread>
#include <format>

using namespace StormByte;

constexpr const char* host = "localhost";
constexpr const unsigned short port = 7070;
auto logger = std::make_shared<Logger>(std::cout, Logger::Level::Info);
auto handler = std::make_shared<Network::Connection::Handler>();

/**
 * @class TestPacket
 * @brief A custom packet class for sending and receiving test messages.
 */
class TestPacket : public Network::Data::Packet {
	public:
		explicit TestPacket(const std::string& message) : m_message(message) {}

		explicit TestPacket(const Buffers::Simple& buffer) {
			m_message = std::string(reinterpret_cast<const char*>(buffer.Data().data()), buffer.Size());
		}

		const std::string& GetMessage() const noexcept {
			return m_message;
		}

	protected:
		void PrepareBuffer() const noexcept override {
			if (m_buffer.Empty()) {
				m_buffer << m_message;
			}
		}

	private:
		std::string m_message; ///< The message contained in the packet.
};

/**
 * @class TestClient
 * @brief A client class with test-specific behavior.
 */
class TestClient : public Network::Client {
	public:
		TestClient(std::shared_ptr<const Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
			: Network::Client(handler, logger) {}

		bool SendTestMessage(const std::string& message) {
			if (!m_socket) {
				return false; // Socket is not initialized
			}

			TestPacket packet(message);
			return m_socket->Send(packet).has_value();
		}

		std::string ReceiveTestMessage() {
			if (!m_socket) {
				return {}; // Socket is not initialized
			}

			auto receive_result = m_socket->Receive();
			if (!receive_result.has_value()) {
				return {};
			}

			auto future_buffer = std::move(receive_result.value());
			Buffers::Simple buffer = future_buffer.get();
			TestPacket packet(buffer);
			return packet.GetMessage();
		}
};

/**
 * @class TestServer
 * @brief A server class with test-specific behavior.
 */
class TestServer : public Network::Server {
	public:
		TestServer(Network::Connection::Protocol protocol, std::shared_ptr<Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
			: Network::Server(protocol, handler, logger) {}

	private:
		StormByte::Network::ExpectedFutureBuffer ProcessClientMessage(
			StormByte::Network::Socket::Client& client,
			StormByte::Network::FutureBuffer& message) noexcept override {
			// Echo the received message back to the client
			return StormByte::Network::ExpectedFutureBuffer(std::move(message));
		}
};

/**
 * @brief Test case for client-server communication with specific behavior.
 */
int TestClientServerCommunication() {
	const std::string fn_name = "TestClientServerCommunication";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		TestServer server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Start(host, port).has_value());

		server_ready = true;

		// Wait for the server to process clients
		std::this_thread::sleep_for(std::chrono::seconds(5));

		server.Stop();
		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		TestClient client(handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port, Network::Connection::Protocol::IPv4).has_value());

		// Send a test message
		const std::string test_message = "Hello, Server!";
		ASSERT_TRUE(fn_name, client.SendTestMessage(test_message));

		// Receive the echoed message
		std::string received_message = client.ReceiveTestMessage();
		ASSERT_EQUAL(fn_name, test_message, received_message);

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

int main() {
	int result = 0;

	result += TestClientServerCommunication();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}