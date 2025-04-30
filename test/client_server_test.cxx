#include <StormByte/network/client.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/serializable.hxx>
#include <StormByte/test_handlers.h>

#include <thread>
#include <format>

using namespace StormByte;

constexpr const char* host = "localhost";
constexpr const unsigned short port = 7070;
auto logger = std::make_shared<Logger>(std::cout, Logger::Level::Info);

enum class OpCodes: unsigned short {
	TestMessage = 1,
};

class TestMessagePacket: public Network::Packet {
	public:
		TestMessagePacket() noexcept:
		Packet(static_cast<unsigned short>(OpCodes::TestMessage)) {}

		TestMessagePacket(const std::string& msg) noexcept:
		Packet(static_cast<unsigned short>(OpCodes::TestMessage)), m_msg(msg) {}

		~TestMessagePacket() noexcept override = default;

		const std::string& Message() const noexcept {
			return m_msg;
		}

	private:
		std::string m_msg;

		Expected<void, Network::PacketError> Deserialize(Network::PacketReaderFunction reader) noexcept override {
			// Opcode is already read in the constructor

			// Read the size of the message
			Buffer::Simple buffer;
			auto expected_size_buffer = reader(sizeof(std::size_t));
			if (!expected_size_buffer) {
				return StormByte::Unexpected<Network::PacketError>("Failed to read message size");
			}
			auto expected_size_serial = Serializable<std::size_t>::Deserialize(expected_size_buffer.value());
			if (!expected_size_serial) {
				return StormByte::Unexpected<Network::PacketError>("Failed to deserialize message size");
			}
			buffer << expected_size_serial.value();
			auto str_buffer = reader(expected_size_serial.value());
			if (!str_buffer) {
				return StormByte::Unexpected<Network::PacketError>("Failed to read message");
			}
			buffer << str_buffer.value();
			auto string_serial = Serializable<std::string>::Deserialize(buffer);
			if (!string_serial) {
				return StormByte::Unexpected<Network::PacketError>("Failed to deserialize message");
			}
			m_msg = string_serial.value();

			return {};
		}

		Buffer::Consumer Serialize() const noexcept override {
			Buffer::Producer buffer(Packet::Serialize());
			Serializable<std::string> string_serial(m_msg);
			buffer << string_serial.Serialize();
			buffer << Buffer::Status::ReadOnly;
			return buffer.Consumer();
		}
};

auto packet_instance_function = [](const unsigned short& opcode) -> std::shared_ptr<Network::Packet> {
	switch (static_cast<OpCodes>(opcode)) {
		case OpCodes::TestMessage:
			return std::make_shared<TestMessagePacket>();
		default:
			return nullptr;
	}
};

auto handler = std::make_shared<Network::Connection::Handler>(packet_instance_function);

class TestClient: public StormByte::Network::Client {
	public:
		TestClient(const Network::Connection::Protocol& protocol, std::shared_ptr<Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
		:Client(protocol, handler, logger) {}

		Network::ExpectedVoid SendTestMessage(const std::string& message) {
			TestMessagePacket packet(message);
			return Send(packet);
		}

		StormByte::Expected<std::string, Network::PacketError> ReceiveTestMessage() {
			auto expected_packet = Receive();
			if (!expected_packet) {
				return StormByte::Unexpected(expected_packet.error());
			}
			auto test_message_packet = std::dynamic_pointer_cast<TestMessagePacket>(expected_packet.value());
			if (!test_message_packet) {
				return StormByte::Unexpected<Network::PacketError>("Failed to cast packet");
			}
			return test_message_packet->Message();
		}
};

class TestServer : public Network::Server {
	public:
		TestServer(Network::Connection::Protocol protocol, std::shared_ptr<Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
			: Network::Server(protocol, handler, logger) {}

	private:
		StormByte::Expected<void, Network::PacketError> ProcessClientPacket(
			Network::Socket::Client& client,
			const Network::Packet& packet) noexcept override {
			// Echo the received message back to the client
			auto expected_send = Send(client, packet);
			if (!expected_send) {
				return StormByte::Unexpected<Network::PacketError>(expected_send.error()->what());
			}
			return {};
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
		ASSERT_TRUE(fn_name, server.Connect(host, port).has_value());

		server_ready = true;

		// Wait for the server to process clients
		std::this_thread::sleep_for(std::chrono::seconds(5));

		server.Disconnect();
		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	while (!server_ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		TestClient client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port).has_value());

		// Send a test message
		const std::string test_message = "Hello, Server!";
		auto expected_send = client.SendTestMessage(test_message);
		ASSERT_TRUE(fn_name, expected_send.has_value());
		auto expected_receive = client.ReceiveTestMessage();
		ASSERT_TRUE(fn_name, expected_receive.has_value());
		ASSERT_EQUAL(fn_name, test_message, expected_receive.value());

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