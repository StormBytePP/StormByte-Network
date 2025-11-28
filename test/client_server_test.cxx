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
			Buffer::FIFO buffer;
			auto expected_size_buffer = reader(sizeof(std::size_t));
			if (!expected_size_buffer) {
				return StormByte::Unexpected<Network::PacketError>("Failed to read message size");
			}
			auto expected_size_data = expected_size_buffer.value().Extract();
			if (!expected_size_data) {
				return StormByte::Unexpected<Network::PacketError>("Failed to extract size data");
			}
			auto expected_size_serial = Serializable<std::size_t>::Deserialize(expected_size_data.value());
			if (!expected_size_serial) {
				return StormByte::Unexpected<Network::PacketError>("Failed to deserialize message size");
			}
			buffer.Write(expected_size_data.value());
			auto str_buffer = reader(expected_size_serial.value());
			if (!str_buffer) {
				return StormByte::Unexpected<Network::PacketError>("Failed to read message");
			}
			auto str_data = str_buffer.value().Extract();
			if (!str_data) {
				return StormByte::Unexpected<Network::PacketError>("Failed to extract string data");
			}
			buffer.Write(str_data.value());
			auto buffer_data = buffer.Extract();
			if (!buffer_data) {
				return StormByte::Unexpected<Network::PacketError>("Failed to extract buffer data");
			}
			auto string_serial = Serializable<std::string>::Deserialize(buffer_data.value());
			if (!string_serial) {
				return StormByte::Unexpected<Network::PacketError>("Failed to deserialize message");
			}
			m_msg = string_serial.value();

			return {};
		}

		Buffer::Consumer Serialize() const noexcept override {
			Buffer::Consumer base_consumer = Packet::Serialize();
			auto base_data = base_consumer.Extract();
			Buffer::Producer buffer;
			if (base_data) {
				buffer.Write(base_data.value());
			}
			Serializable<std::string> string_serial(m_msg);
			buffer.Write(string_serial.Serialize());
			buffer.Close();
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

std::vector<std::byte> FlipBytes(const std::vector<std::byte>& data) {
	std::vector<std::byte> flipped_data;
	flipped_data.reserve(data.size());
	for (const auto& byte : data) {
		flipped_data.push_back(~byte);
	}
	return flipped_data;
}

void FlipBytes(Buffer::Consumer in, Buffer::Producer out) {
	// Process until the input is closed and no bytes remain
	while (in.IsWritable() || in.AvailableBytes() > 0) {
		// If no data is currently available, wait briefly unless closed
		if (in.AvailableBytes() == 0) {
			if (!in.IsWritable()) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// Extract as much as is currently available
		auto expected_data = in.Extract(in.AvailableBytes());
		if (!expected_data) {
			// On error, stop processing
			break;
		}
		if (expected_data.value().empty()) {
			if (!in.IsWritable()) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		auto flipped_data = FlipBytes(expected_data.value());
		out.Write(flipped_data);
	}
	out.Close();
}

class TestSimulatedEncryptedClient : public TestClient {
	public:
		TestSimulatedEncryptedClient(const Network::Connection::Protocol& protocol, std::shared_ptr<Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
		: TestClient(protocol, handler, logger) {
			m_input_pipeline.AddPipe([](Buffer::Consumer in, Buffer::Producer out) {
				FlipBytes(in, out);
			});
			m_output_pipeline.AddPipe([](Buffer::Consumer in, Buffer::Producer out) {
				FlipBytes(in, out);
			});
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

class TestSimulatedEncryptedServer : public TestServer {
	public:
		TestSimulatedEncryptedServer(Network::Connection::Protocol protocol, std::shared_ptr<Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
		: TestServer(protocol, handler, logger) {
			m_input_pipeline.AddPipe([](Buffer::Consumer in, Buffer::Producer out) {
				FlipBytes(in, out);
			});
			m_output_pipeline.AddPipe([](Buffer::Consumer in, Buffer::Producer out) {
				FlipBytes(in, out);
			});
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
	std::mutex mtx;
	std::condition_variable cv_ready;
	std::condition_variable cv_done;

	// Start server thread
	std::thread server_thread([&]() -> int {
		TestServer server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Connect(host, port).has_value());
		{
			std::lock_guard<std::mutex> lk(mtx);
			server_ready = true;
		}
		cv_ready.notify_all();

		// Wait until client finishes
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv_done.wait(lk, [&]{ return client_completed; });
		}

		server.Disconnect();
		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	{
		std::unique_lock<std::mutex> lk(mtx);
		cv_ready.wait(lk, [&]{ return server_ready; });
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
		{
			std::lock_guard<std::mutex> lk(mtx);
			client_completed = true;
		}
		cv_done.notify_all();
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

/**
 * @brief Test case for client-server communication with encryption simulated.
 */
int TestClientServerSimulatedEncryptedCommunication() {
	const std::string fn_name = "TestClientServerSimulatedEncryptedCommunication";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;
	std::mutex mtx;
	std::condition_variable cv_ready;
	std::condition_variable cv_done;

	// Start server thread
	std::thread server_thread([&]() -> int {
		TestSimulatedEncryptedServer server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Connect(host, port).has_value());
		{
			std::lock_guard<std::mutex> lk(mtx);
			server_ready = true;
		}
		cv_ready.notify_all();

		// Wait until client finishes
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv_done.wait(lk, [&]{ return client_completed; });
		}

		server.Disconnect();
		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	{
		std::unique_lock<std::mutex> lk(mtx);
		cv_ready.wait(lk, [&]{ return server_ready; });
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		TestSimulatedEncryptedClient client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port).has_value());

		// Send a test message
		const std::string test_message = "Hello, Server!";
		auto expected_send = client.SendTestMessage(test_message);
		ASSERT_TRUE(fn_name, expected_send.has_value());
		auto expected_receive = client.ReceiveTestMessage();
		ASSERT_TRUE(fn_name, expected_receive.has_value());
		ASSERT_EQUAL(fn_name, test_message, expected_receive.value());

		client.Disconnect();
		{
			std::lock_guard<std::mutex> lk(mtx);
			client_completed = true;
		}
		cv_done.notify_all();
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

int TestClientServerSimulatedEncryptedLargeCommunication() {
	// If 4096 then this test stalls and not finish
	const std::string fn_name = "TestClientServerSimulatedEncryptedLargeCommunication";

	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;
	std::mutex mtx;
	std::condition_variable cv_ready;
	std::condition_variable cv_done;

	// Start server thread
	std::thread server_thread([&]() -> int {
		TestSimulatedEncryptedServer server(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, server.Connect(host, port).has_value());
		{
			std::lock_guard<std::mutex> lk(mtx);
			server_ready = true;
		}
		cv_ready.notify_all();

		// Wait until client finishes
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv_done.wait(lk, [&]{ return client_completed; });
		}

		server.Disconnect();
		server_completed = true;
		return 0;
	});

	// Wait until the server is ready
	{
		std::unique_lock<std::mutex> lk(mtx);
		cv_ready.wait(lk, [&]{ return server_ready; });
	}

	// Start client thread
	std::thread client_thread([&]() -> int {
		TestSimulatedEncryptedClient client(Network::Connection::Protocol::IPv4, handler, logger);
		ASSERT_TRUE(fn_name, client.Connect(host, port).has_value());

		// Send a test message
		const std::string test_message = "Hello, Server!";
		auto expected_send = client.SendTestMessage(test_message);
		ASSERT_TRUE(fn_name, expected_send.has_value());
		auto expected_receive = client.ReceiveTestMessage();
		ASSERT_TRUE(fn_name, expected_receive.has_value());
		ASSERT_EQUAL(fn_name, test_message, expected_receive.value());

		client.Disconnect();
		{
			std::lock_guard<std::mutex> lk(mtx);
			client_completed = true;
		}
		cv_done.notify_all();
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
	result += TestClientServerSimulatedEncryptedCommunication();
	result += TestClientServerSimulatedEncryptedLargeCommunication();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}