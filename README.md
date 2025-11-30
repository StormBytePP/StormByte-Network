# StormByte

StormByte is a comprehensive, cross-platform C++ library aimed at easing system programming, configuration management, logging, and database handling tasks. This library provides a unified API that abstracts away the complexities and inconsistencies of different platforms (Windows, Linux).

## Features

- **Network**: Provides classes to handle portable network communication across Linux and Windows, including features such as asynchronous data handling, error management, and high-level socket APIs.

## Table of Contents

- [Repository](#repository)
- [Installation](#installation)
- [Modules](#modules)
	- [Base](https://dev.stormbyte.org/StormByte)
	- [Buffer](https://dev.stormbyte.org/StormByte-Buffer)
	- [Config](https://dev.stormbyte.org/StormByte-Config)
	- [Crypto](https://dev.stormbyte.org/StormByte-Crypto)
	- [Database](https://dev.stormbyte.org/StormByte-Database)
	- [Logger](https://github.com/StormBytePP/StormByte-Logger.git)
	- [Multimedia](https://dev.stormbyte.org/StormByte-Multimedia)
	- **Network**
	- [System](https://dev.stormbyte.org/StormByte-System)
- [Contributing](#contributing)
- [License](#license)

## Modules

### Network

#### Overview

The `Network` module of StormByte provides an intuitive API to manage networking tasks. It includes classes and methods for working with sockets and asynchronous data handling.

#### Features
- **Socket Abstraction**: Unified interface for handling TCP/IP sockets.
- **Error Handling**: Provides detailed error messages for network operations.
- **Cross-Platform**: Works seamlessly on both Linux and Windows.
- **Data Transmission**: Supports partial and large data transmission.
- **Timeouts and Disconnections**: Handles timeout scenarios and detects client/server disconnections.

#### Examples

##### Creating custom Packets
In order to ease the client/server communication, user can create their own Packets to be sent/received from client to server (and viceversa). In order to do so, users need to inherit from `StormByte::Network::Packet` class and focus only on 2 methods:

* Constructors:
	* A no parameter constructor to be able to empty initialize it
	* The second constructor can take whatever argument is needed and must initialize the derived instance
* Override `Serialize` and `Deserialize` function members to be able to construct and output the Packet.

This way, instead of sending raw messages over network you send/receive Packets instances, for example, a Packet which represents
a std::string could be implemented as follows:

```cpp
#include <StormByte/network/packet.hxx>

enum class OpCodes: unsigned short {
	TestMessage = 1,
};

class TestMessagePacket: public StormByte::Network::Packet {
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

		Expected<void, Network::PacketError> Deserialize(StormByte::Network::PacketReaderFunction reader) noexcept override {
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
```

##### Creating a Custom Server

The `Server` class can be extended to define custom behavior for handling client messages. Below is an example of a simple echo server:

```cpp
#include <StormByte/network/server.hxx>

class TestServer : public StormByte::Network::Server {
	public:
		TestServer(StormByte::Network::Connection::Protocol protocol, std::shared_ptr<StormByte::Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
		: StormByte::Network::Server(protocol, handler, logger) {}

	private:
		StormByte::Expected<void, StormByte::Network::PacketError> ProcessClientPacket(
			StormByte::Network::Socket::Client& client,
			const StormByte::Network::Packet& packet) noexcept override {
			// Echo the received message back to the client
			auto expected_send = Send(client, packet);
			if (!expected_send) {
				return StormByte::Unexpected<StormByte::Network::PacketError>(expected_send.error()->what());
			}
			return {};
		}
};
```

##### Creating a Custom Client
The Client class can be extended to define custom behavior for sending and receiving messages. Below is an example of a client that sends a test message to the server:

```cpp
#include <StormByte/network/client.hxx>

class TestClient: public StormByte::Network::Client {
	public:
		TestClient(const StormByte::Network::Connection::Protocol& protocol, std::shared_ptr<StormByte::Network::Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept
		:Client(protocol, handler, logger) {}

		Network::ExpectedVoid SendTestMessage(const std::string& message) {
			TestMessagePacket packet(message);
			return Send(packet);
		}

		StormByte::Expected<std::string, StormByte::Network::PacketError> ReceiveTestMessage() {
			auto expected_packet = Receive();
			if (!expected_packet) {
				return StormByte::Unexpected(expected_packet.error());
			}
			auto test_message_packet = std::dynamic_pointer_cast<TestMessagePacket>(expected_packet.value());
			if (!test_message_packet) {
				return StormByte::Unexpected<StormByte::Network::PacketError>("Failed to cast packet");
			}
			return test_message_packet->Message();
		}
};
```

##### Usage Example
Providing the previous class definitions, a simple Client/Server communication where the server just echoes the same packet back to the client can be implemented as follows:

```cpp
#include <iostream>
#include <thread>

int main() {
	bool server_ready = false;
	bool server_completed = false;
	bool client_completed = false;

	// Start server thread
	std::thread server_thread([&]() -> int {
		TestServer server(Network::Connection::Protocol::IPv4, handler, logger);
		if (!server.Connect(host, port).has_value())
			return 1;

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
		if (!client.Connect(host, port).has_value())
			return 1;

		// Send a test message
		const std::string test_message = "Hello, Server!";
		auto expected_send = client.SendTestMessage(test_message);
		if (!expected_send) {
			std::cerr << "Send packet failed: " << expected_send.error()->what() << std::endl;
			return 1;
		}

		auto expected_receive = client.ReceiveTestMessage();
		if (!expected_receive) {
			std::cerr << "Receive packet failed: " << expected_receive.error()->what() << std::endl;
			return 1;
		}

		if (test_message == expected_receive.value())
			std::cout << "Message sent and echoed back successfully!" << std::endl;
		else
			std::err << "Received message do not match to sent message!" << std::endl;

		client.Disconnect();
		client_completed = true;
		return 0;
	});

	// Join threads
	server_thread.join();
	client_thread.join();

	return 0;
}
```