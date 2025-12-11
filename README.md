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

The **StormByte Network** module provides a robust, cross-platform C++ library for building client-server applications. It abstracts away platform-specific socket handling (Linux and Windows) and provides a high-level, type-safe packet-based communication system. The library uses modern C++ features including `Expected` types for error handling, move semantics, and RAII for resource management.

#### Features

- **Cross-platform socket abstraction**: Works seamlessly on both Linux and Windows
- **Type-safe packet communication**: Define custom packet types with automatic serialization
- **Asynchronous event handling**: Non-blocking I/O with configurable timeouts
- **Connection management**: Automatic client tracking and lifecycle management for servers
- **Pipeline support**: Optional preprocessing/postprocessing (compression, encryption)
- **Thread-safe logging**: Integrated with StormByte Logger for diagnostics
- **UUID-based client identification**: Each connection is uniquely identified
- **MTU discovery**: Automatic path MTU detection for optimal packet sizing
- **Error handling**: Uses `Expected<T, E>` pattern to avoid exceptions in performance-critical paths

#### Architecture

The library is built around three main base classes that you extend to implement your application logic:

1. **`Packet`** - Base class for all network messages. Each packet has an opcode (message type identifier) and serializes its data.
3. **`Server` / `Client`** - High-level connection endpoints. Override `ProcessClientPacket()` in Server or use `Send()`/`Receive()` in Client to handle communication.

#### Examples

##### Minimal example (based on test/client_server_test.cxx)

The example below shows the minimal pieces needed with the new API: packet types derived from `Transport::Packet`, a deserializer function, a small `Client` helper that uses `Send()` as a request/response primitive and a `Server` that replies from `ProcessClientPacket()`.

```cpp
#include <StormByte/network/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/serializable.hxx>
#include <StormByte/logger/threaded_log.hxx>

namespace Test {
	enum class Opcode : unsigned short { C_ASK_NAMES = 1, S_REPLY_NAMES };

	// Small packet pair: request contains a size, reply contains vector<string>
	namespace Packet {
		class AskNameList : public StormByte::Network::Transport::Packet {
		public:
			AskNameList(std::size_t n) : Transport::Packet(static_cast<Transport::Packet::OpcodeType>(Opcode::C_ASK_NAMES)), m_n(n) {}
			StormByte::Buffer::DataType DoSerialize() const noexcept override {
				return StormByte::Serializable<std::size_t>(m_n).Serialize();
			}
			std::size_t GetAmount() const noexcept { return m_n; }
		private: std::size_t m_n;
		};

		class AnswerNameList : public StormByte::Network::Transport::Packet {
		public:
			AnswerNameList(const std::vector<std::string>& names) : Transport::Packet(static_cast<Transport::Packet::OpcodeType>(Opcode::S_REPLY_NAMES)), m_names(names) {}
			StormByte::Buffer::DataType DoSerialize() const noexcept override { return StormByte::Serializable<std::vector<std::string>>(m_names).Serialize(); }
			const std::vector<std::string>& GetNames() const noexcept { return m_names; }
		private: std::vector<std::string> m_names;
		};
	}

	// Deserializer used by Client/Server constructors
	inline StormByte::Network::DeserializePacketFunction MakeDeserializer() {
		return [](StormByte::Network::Transport::Packet::OpcodeType opcode, StormByte::Buffer::Consumer consumer, StormByte::Logger::Log& /*logger*/) -> StormByte::Network::PacketPointer {
			StormByte::Buffer::DataType data;
			consumer.ExtractUntilEoF(data);
			switch(static_cast<Opcode>(opcode)) {
				case Opcode::C_ASK_NAMES: {
					auto n = StormByte::Serializable<std::size_t>::Deserialize(data);
					if (!n) return nullptr;
					return std::make_shared<Packet::AskNameList>(*n);
				}
				case Opcode::S_REPLY_NAMES: {
					auto names = StormByte::Serializable<std::vector<std::string>>::Deserialize(data);
					if (!names) return nullptr;
					return std::make_shared<Packet::AnswerNameList>(*names);
				}
				default: return nullptr;
			}
		};
	}

	// Simplified client wrapper
	class Client : public StormByte::Network::Client {
	public:
		Client(const StormByte::Logger::ThreadedLog& logger) noexcept : StormByte::Network::Client(MakeDeserializer(), logger) {}
		StormByte::Buffer::Pipeline InputPipeline() const noexcept override { return {}; }
		StormByte::Buffer::Pipeline OutputPipeline() const noexcept override { return {}; }

		auto RequestNames(std::size_t n) noexcept -> StormByte::Expected<std::vector<std::string>, StormByte::Network::Exception> {
			Packet::AskNameList req(n);
			auto resp = Send(req);
			if (!resp) return StormByte::Unexpected<StormByte::Network::Exception>("send/receive failed");
			auto ans = std::dynamic_pointer_cast<Packet::AnswerNameList>(resp);
			if (!ans) return StormByte::Unexpected<StormByte::Network::Exception>("unexpected response");
			return ans->GetNames();
		}
	};

	// Simplified server
	class Server : public StormByte::Network::Server {
	public:
		Server(const StormByte::Logger::ThreadedLog& logger) noexcept : StormByte::Network::Server(MakeDeserializer(), logger) {}
		StormByte::Buffer::Pipeline InputPipeline() const noexcept override { return {}; }
		StormByte::Buffer::Pipeline OutputPipeline() const noexcept override { return {}; }

	private:
		StormByte::Network::PacketPointer ProcessClientPacket(const std::string& /*uuid*/, StormByte::Network::PacketPointer packet) noexcept override {
			switch(static_cast<Opcode>(packet->Opcode())) {
				case Opcode::C_ASK_NAMES: {
					auto ask = std::dynamic_pointer_cast<Packet::AskNameList>(packet);
					if (!ask) return nullptr;
					std::vector<std::string> names;
					for (std::size_t i = 0; i < ask->GetAmount(); ++i) names.push_back("Name_" + std::to_string(i+1));
					return std::make_shared<Packet::AnswerNameList>(names);
				}
				default: return nullptr;
			}
		}
	};
}

// Usage (sketch):
// - create ThreadedLog
// - start Server and call Connect(protocol, host, port)
// - start Client and call Connect(protocol, host, port)
// - call Client::RequestNames(n) which uses Send() under the hood

```

Explanation:

- The deserializer converts opcode + payload bytes into concrete `Packet` objects and is passed to both `Client` and `Server` constructors.
- `Client::Send()` is used as a synchronous request/response helper in this simplified pattern (the test wraps Send into higher-level helpers).
- `Server::ProcessClientPacket()` inspects the opcode and can return a `PacketPointer` to send back immediately (or `nullptr` when no reply is needed).

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests for any enhancements or bug fixes.

## License

This project is licensed under GPL v3 License - see the [LICENSE](LICENSE) file for details.