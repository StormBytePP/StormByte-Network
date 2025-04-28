# StormByte

StormByte is a comprehensive, cross-platform C++ library aimed at easing system programming, configuration management, logging, and database handling tasks. This library provides a unified API that abstracts away the complexities and inconsistencies of different platforms (Windows, Linux).

## Features

- **Network**: Provides classes to handle portable network communication across Linux and Windows, including features such as asynchronous data handling, error management, and high-level socket APIs.

## Table of Contents

- [Repository](#repository)
- [Installation](#installation)
- [Modules](#modules)
	- [Base](https://dev.stormbyte.org/StormByte)
	- [Config](https://dev.stormbyte.org/StormByte-Config)
	- [Crypto](https://dev.stormbyte.org/StormByte-Crypto)
	- [Database](https://dev.stormbyte.org/StormByte-Database)
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
	* A no parameter constructor to be able to empty initialize it (and use internally the `Initialize` method below)
	* The second constructor can take whatever argument is needed and must initialize the internal `m_buffer`
* Override protected `Initialize` function member to be able to construct the Packet via its `reader` function.

This way, instead of sending raw messages over network you send/receive Packets instances, for example, a Packet which represents
a std::string could be implemented as follows:

```cpp
using namespace StormByte;

enum class OpCodes: unsigned short {
	TestMessage = 1
};

class TestMessagePacket: public Network::Packet {
	public:
		/* Default constructor */
		TestMessagePacket() noexcept:
		Packet(static_cast<unsigned short>(OpCodes::TestMessage)) {}

		/* Constructor with a message */
		TestMessagePacket(const std::string& msg) noexcept:
		Packet(static_cast<unsigned short>(OpCodes::TestMessage)), m_msg(msg) {
			Serializable<std::string> string_serial(m_msg);
			m_buffer << string_serial.Serialize();
		}

		~TestMessagePacket() noexcept override = default;

		const std::string& Message() const noexcept {
			return m_msg;
		}

	private:
		std::string m_msg;

		Expected<void, Network::PacketError> Initialize(Network::PacketReaderFunction reader) noexcept override {
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
			Serializable<std::string> string_serializable(m_msg);
			m_buffer << string_serializable.Serialize();
			return {};
		}
};
```

##### Creating a Custom Server

The `Server` class can be extended to define custom behavior for handling client messages. Below is an example of a simple echo server:

```cpp
#include <StormByte/network/server.hxx>
#include <StormByte/network/typedefs.hxx>
#include <StormByte/logger.hxx>

class EchoServer : public StormByte::Network::Server {
	public:
		EchoServer(StormByte::Network::Connection::Protocol protocol,
				std::shared_ptr<StormByte::Network::Connection::Handler> handler,
				std::shared_ptr<StormByte::Logger> logger) noexcept
			: Server(protocol, handler, logger) {}

	private:
		StormByte::Network::ExpectedFutureBuffer ProcessClientMessage(
			StormByte::Network::Socket::Client& client,
			StormByte::Network::FutureBuffer& message) noexcept override {
			// Echo the received message back to the client
			return StormByte::Network::ExpectedFutureBuffer(std::move(message));
		}
};

int main() {
	auto logger = std::make_shared<StormByte::Logger>(std::cout, StormByte::Logger::Level::Info);
	auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

	EchoServer server(StormByte::Network::Connection::Protocol::IPv4, handler, logger);
	if (!server.Start("localhost", 7070)) {
		std::cerr << "Failed to start the server" << std::endl;
		return 1;
	}

	std::cout << "Server is running..." << std::endl;

	// Run the server for 10 seconds
	std::this_thread::sleep_for(std::chrono::seconds(10));

	server.Stop();
	std::cout << "Server stopped." << std::endl;

	return 0;
}
```

##### Creating a Custom Client
The Client class can be extended to define custom behavior for sending and receiving messages. Below is an example of a client that sends a test message to the server:

```cpp
#include <StormByte/network/client.hxx>
#include <StormByte/network/typedefs.hxx>
#include <StormByte/logger.hxx>

class TestClient : public StormByte::Network::Client {
	public:
		TestClient(StormByte::Network::Connection::Protocol protocol,
				std::shared_ptr<StormByte::Network::Connection::Handler> handler,
				std::shared_ptr<StormByte::Logger> logger) noexcept
			: Client(protocol, handler, logger, nullptr) {}

		void SendTestMessage(const std::string& message) {
			StormByte::Network::Packet packet(1); // Example opcode
			packet << message;

			auto result = Send(packet);
			if (!result) {
				std::cerr << "Failed to send message: " << result.error()->what() << std::endl;
			} else {
				std::cout << "Message sent successfully!" << std::endl;
			}
		}
};

int main() {
	auto logger = std::make_shared<StormByte::Logger>(std::cout, StormByte::Logger::Level::Info);
	auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

	TestClient client(StormByte::Network::Connection::Protocol::IPv4, handler, logger);
	if (!client.Connect("localhost", 7070)) {
		std::cerr << "Failed to connect to the server" << std::endl;
		return 1;
	}

	client.SendTestMessage("Hello, Server!");
	client.Disconnect();

	return 0;
}
```