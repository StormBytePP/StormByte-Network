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
- **Codec system**: Extensible encoding/decoding framework for wire protocols
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
2. **`Codec`** - Handles encoding/decoding between raw bytes and `Packet` objects. This is where you define your wire protocol.
3. **`Server` / `Client`** - High-level connection endpoints. Override `ProcessClientPacket()` in Server or use `Send()`/`Receive()` in Client to handle communication.

#### Examples

##### Basic Packet Definition

First, define your packet types by inheriting from `Network::Packet`:

```cpp
#include <StormByte/network/packet.hxx>

namespace MyApp {
    // Define opcodes for your protocol
    enum class Opcode : unsigned short {
        REQUEST_NAME_LIST,
        RESPONSE_NAME_LIST,
        REQUEST_DATA,
        RESPONSE_DATA
    };

    // Base packet class with opcode
    class BasePacket : public StormByte::Network::Packet {
    public:
        BasePacket(Opcode opcode) 
            : Network::Packet(static_cast<unsigned short>(opcode)) {}
    };

    // Request packet - asks server for a list of names
    class RequestNameList : public BasePacket {
    public:
        RequestNameList(std::size_t count) 
            : BasePacket(Opcode::REQUEST_NAME_LIST)
            , m_count(count) {}
        
        // Serialize packet data to bytes
        Buffer::FIFO DoSerialize() const noexcept override {
            return StormByte::Serializable<std::size_t>(m_count).Serialize();
        }

    private:
        std::size_t m_count;
    };

    // Response packet - returns list of names from server
    class ResponseNameList : public BasePacket {
    public:
        ResponseNameList(const std::vector<std::string>& names)
            : BasePacket(Opcode::RESPONSE_NAME_LIST)
            , m_names(names) {}
        
        Buffer::FIFO DoSerialize() const noexcept override {
            return StormByte::Serializable<std::vector<std::string>>(m_names).Serialize();
        }
        
        const std::vector<std::string>& GetNames() const { return m_names; }

    private:
        std::vector<std::string> m_names;
    };
}
```

**Why inherit from `Packet`?** The `Packet` base class provides the fundamental wire protocol structure. By overriding `DoSerialize()`, you define how your packet's data is converted to bytes for network transmission. The framework handles framing, buffering, and transport automatically.

##### Implementing a Codec

The `Codec` class is responsible for decoding received bytes into your packet types:

```cpp
#include <StormByte/network/codec.hxx>

namespace MyApp {
    class MyCodec : public StormByte::Network::Codec {
    public:
        MyCodec(const Logger::ThreadedLog& log) noexcept 
            : Network::Codec(log) {}
        
        // Required: Create a copy of this codec
        PointerType Clone() const override {
            return MakePointer<MyCodec>(*this);
        }
        
        // Required: Move this codec
        PointerType Move() override {
            return MakePointer<MyCodec>(std::move(*this));
        }
        
        // Decode raw bytes into a Packet object
        Network::ExpectedPacket DoEncode(
            const unsigned short& opcode,
            const std::size_t& size,
            const StormByte::Buffer::Consumer& consumer
        ) const noexcept override {
            switch (static_cast<Opcode>(opcode)) {
                case Opcode::REQUEST_NAME_LIST: {
                    // Read 'size' bytes from the consumer
                    auto data = consumer.Read(size);
                    if (!data) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to read packet data");
                    }
                    
                    // Deserialize the count
                    auto count = StormByte::Serializable<std::size_t>::Deserialize(
                        data.value());
                    if (!count) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to deserialize count");
                    }
                    
                    return std::make_shared<RequestNameList>(*count);
                }
                
                case Opcode::RESPONSE_NAME_LIST: {
                    auto data = consumer.Read(size);
                    if (!data) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to read packet data");
                    }
                    
                    auto names = StormByte::Serializable<std::vector<std::string>>::
                        Deserialize(data.value());
                    if (!names) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to deserialize names");
                    }
                    
                    return std::make_shared<ResponseNameList>(*names);
                }
                
                default:
                    return StormByte::Unexpected<Network::PacketError>(
                        "Unknown opcode");
            }
        }
    };
}
```

**Why inherit from `Codec`?** The `Codec` acts as a factory that converts raw network bytes into strongly-typed `Packet` objects. This separation allows you to define your wire protocol (how opcodes map to packet types, how data is framed) independently from the transport layer. The framework handles reading the correct number of bytes and calls your `DoEncode()` method to construct the appropriate packet type.

##### Implementing a Server

The `Server` class processes incoming client packets:

```cpp
#include <StormByte/network/server.hxx>

namespace MyApp {
    class MyServer : public StormByte::Network::Server {
    public:
        MyServer(
            const Network::Protocol& protocol,
            unsigned short timeout,
            const Logger::ThreadedLog& logger
        ) noexcept 
            : Network::Server(protocol, MyCodec(logger), timeout, logger) {}
        
    private:
        // Override this to handle incoming packets from clients
        Expected<void, Network::PacketError> ProcessClientPacket(
            const std::string& client_uuid,
            const Network::Packet& packet
        ) noexcept override {
            switch (static_cast<Opcode>(packet.Opcode())) {
                case Opcode::REQUEST_NAME_LIST: {
                    // Cast to the specific packet type
                    auto& request = static_cast<const RequestNameList&>(packet);
                    
                    // Extract the count from the packet
                    auto serialized = request.DoSerialize();
                    auto extracted = serialized.Extract();
                    if (!extracted) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to extract request data");
                    }
                    
                    auto count = StormByte::Serializable<std::size_t>::
                        Deserialize(extracted.value());
                    if (!count) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to deserialize count");
                    }
                    
                    // Generate the requested names
                    std::vector<std::string> names;
                    for (std::size_t i = 0; i < *count; ++i) {
                        names.push_back("Name_" + std::to_string(i + 1));
                    }
                    
                    // Send response back to the client
                    ResponseNameList response(names);
                    auto result = Send(client_uuid, response);
                    if (!result) {
                        return StormByte::Unexpected<Network::PacketError>(
                            "Failed to send response");
                    }
                    break;
                }
                
                default:
                    return StormByte::Unexpected<Network::PacketError>(
                        "Unknown packet type");
            }
            return {};
        }
    };
}
```

**Why inherit from `Server`?** The `Server` base class handles all the complex socket management: listening for connections, accepting clients, managing multiple concurrent connections, and dispatching received packets. By overriding `ProcessClientPacket()`, you implement your application's business logic while the framework handles all network I/O, buffering, and client lifecycle management. Each client is identified by a UUID, allowing you to maintain per-client state and send targeted responses.

##### Implementing a Client

The `Client` class sends requests and receives responses:

```cpp
#include <StormByte/network/client.hxx>

namespace MyApp {
    class MyClient : public StormByte::Network::Client {
    public:
        MyClient(
            const Network::Protocol& protocol,
            unsigned short timeout,
            const Logger::ThreadedLog& logger
        ) noexcept 
            : Network::Client(protocol, MyCodec(logger), timeout, logger) {}
        
        // Custom method to request names from server
        Expected<std::vector<std::string>, Network::Exception> 
        RequestNames(std::size_t count) noexcept {
            // Create and send request
            RequestNameList request(count);
            auto send_result = Send(request);
            if (!send_result) {
                return StormByte::Unexpected<Network::Exception>(
                    "Failed to send request");
            }
            
            // Wait for response
            auto receive_result = Receive();
            if (!receive_result) {
                return StormByte::Unexpected<Network::Exception>(
                    "Failed to receive response");
            }
            
            // Verify it's the right packet type
            auto& packet = receive_result.value();
            if (packet->Opcode() != static_cast<unsigned short>(Opcode::RESPONSE_NAME_LIST)) {
                return StormByte::Unexpected<Network::Exception>(
                    "Unexpected response opcode");
            }
            
            // Cast and extract data
            auto response = std::static_pointer_cast<ResponseNameList>(packet);
            return response->GetNames();
        }
    };
}
```

**Why inherit from `Client`?** The `Client` base class provides connection management and synchronous/asynchronous communication primitives. By inheriting from it, you gain access to `Send()` and `Receive()` methods that handle all the low-level details of packet framing, buffering, and network I/O. You add domain-specific methods (like `RequestNames()`) that compose these primitives into meaningful application-level operations, keeping your business logic clean and testable.

##### Putting It All Together

```cpp
#include <StormByte/logger/threaded_log.hxx>
#include <iostream>

int main() {
    // Create a logger
    Logger::ThreadedLog logger(std::cout, Logger::Level::Info, "[%L] %T:");
    
    // Create and start server
    MyApp::MyServer server(Network::Protocol::IPv4, 5, logger);
    auto listen_result = server.Connect("127.0.0.1", 8080);
    if (!listen_result) {
        std::cerr << "Server failed to start: " 
                  << listen_result.error()->what() << std::endl;
        return 1;
    }
    
    // Create and connect client
    MyApp::MyClient client(Network::Protocol::IPv4, 5, logger);
    auto connect_result = client.Connect("127.0.0.1", 8080);
    if (!connect_result) {
        std::cerr << "Client failed to connect: " 
                  << connect_result.error()->what() << std::endl;
        return 1;
    }
    
    // Request 5 names from server
    auto names_result = client.RequestNames(5);
    if (!names_result) {
        std::cerr << "Request failed: " 
                  << names_result.error()->what() << std::endl;
        return 1;
    }
    
    // Print received names
    for (const auto& name : names_result.value()) {
        std::cout << "Received: " << name << std::endl;
    }
    
    // Cleanup
    client.Disconnect();
    server.Disconnect();
    
    return 0;
}
```

#### Key Concepts

- **Packet**: Represents a single message with an opcode and payload. Override `DoSerialize()` to define how your data becomes bytes.
  
- **Codec**: Translates between raw bytes and Packet objects. Override `DoEncode()` to define your wire protocol and create the appropriate Packet instances based on opcode.

- **Server**: Listens for connections and processes client packets. Override `ProcessClientPacket()` to implement your server-side logic and use `Send(client_uuid, packet)` to respond to specific clients.

- **Client**: Connects to a server and exchanges packets. Use `Send(packet)` to send requests and `Receive()` to get responses synchronously.

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests for any enhancements or bug fixes.

## License

This project is licensed under GPL v3 License - see the [LICENSE](LICENSE) file for details.