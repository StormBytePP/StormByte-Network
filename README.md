Here’s the updated README based on your feedback:

---

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
	- [Database](https://dev.stormbyte.org/StormByte-Database)
	- [Logger](https://dev.stormbyte.org/StormByte-Logger)
	- [Multimedia](https://dev.stormbyte.org/StormByte-Multimedia)
	- **Network**
	- [System](https://dev.stormbyte.org/StormByte-System)
- [Contributing](#contributing)
- [License](#license)

## Repository

You can visit the code repository at [GitHub](https://github.com/StormBytePP/StormByte-Network)

## Installation

### Prerequisites

Ensure you have the following installed:

- C++23 compatible compiler
- CMake 3.12 or higher

### Building

To build the library, follow these steps:

```sh
git clone https://github.com/StormBytePP/StormByte-Network.git
cd StormByte-Network
mkdir build
cd build
cmake ..
make
```

## Modules

StormByte Library is composed of several modules:

### Network

#### Overview

The `Network` module of StormByte provides an intuitive API to manage networking tasks. It includes classes and methods for working with sockets and asynchronous data handling.

#### Features
- **Socket Abstraction**: Unified interface for handling TCP/IP sockets.
- **Error Handling**: Provides detailed error messages for network operations.
- **Cross-Platform**: Works seamlessly on both Linux and Windows.
- **Data Transmission**: Supports partial and large data transmission.
- **Timeouts and Disconnections**: Handles timeout scenarios and detects client/server disconnections.

#### Usage

The `Socket` classes in this module are abstractions that handle low-level networking tasks like socket creation, data transmission, and receiving data. While not intended for direct use, they serve as a foundation for higher-level client/server classes currently under development to simplify communication workflows.

For example, to setup a client socket:
```cpp
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <iostream>

int main() {
    using namespace StormByte::Network::Socket;

    // Example setup for a Client
    auto handler = std::make_shared<StormByte::Network::Connection::Handler>();
    Client client(StormByte::Network::Connection::Protocol::IPv4, handler);

    // Connecting to a server
    auto connect_result = client.Connect("localhost", 6060);
    if (!connect_result) {
        std::cerr << "Failed to connect: " << connect_result.error()->what() << std::endl;
        return -1;
    }

    // Waiting for data with a timeout
    auto wait_result = client.WaitForData(500000); // 500 ms timeout
    if (wait_result.has_value()) {
        if (wait_result.value() == StormByte::Network::Connection::Read::Result::Timeout) {
            std::cout << "No data received within the timeout period." << std::endl;
        } else {
            std::cout << "Data is available!" << std::endl;
        }
    } else {
        std::cerr << "Error waiting for data: " << wait_result.error()->what() << std::endl;
    }

    client.Disconnect();
    return 0;
}
```

Or to setup a server socket
```cpp
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <iostream>

int main() {
    using namespace StormByte::Network::Socket;

    // Create a handler for network operations
    auto handler = std::make_shared<StormByte::Network::Connection::Handler>();

    // Initialize the Server socket
    Server server(StormByte::Network::Connection::Protocol::IPv4, handler);

    // Configure the server to listen on a specific host and port
    const std::string hostname = "localhost";
    const unsigned short port = 6060;

    auto listen_result = server.Listen(hostname, port);
    if (!listen_result) {
        std::cerr << "Server failed to listen on " << hostname << ":" << port
                  << " - Error: " << listen_result.error()->what() << std::endl;
        return -1;
    }

    std::cout << "Server is listening on " << hostname << ":" << port << std::endl;

    // Wait for an incoming connection
    auto accept_result = server.Accept();
    if (!accept_result) {
        std::cerr << "Error while accepting a client connection: "
                  << accept_result.error()->what() << std::endl;
        return -1;
    }

    // Connection established, handle the client socket
    Client client = std::move(accept_result.value());
    std::cout << "Client connected!" << std::endl;

    // Wait for data from the client
    auto wait_result = client.WaitForData(500000); // 500 ms timeout
    if (wait_result.has_value()) {
        if (wait_result.value() == StormByte::Network::Connection::Read::Result::Timeout) {
            std::cout << "No data received within the timeout period." << std::endl;
        } else {
            std::cout << "Data is available from the client." << std::endl;

            // Attempt to receive data
            auto receive_result = client.Receive();
            if (receive_result) {
                auto future_buffer = std::move(receive_result.value());
                auto buffer = future_buffer.get();
                std::cout << "Received data: "
                          << std::string(reinterpret_cast<const char*>(buffer.Data().data()), buffer.Size())
                          << std::endl;
            } else {
                std::cerr << "Failed to receive data from client: "
                          << receive_result.error()->what() << std::endl;
            }
        }
    } else {
        std::cerr << "Error waiting for client data: "
                  << wait_result.error()->what() << std::endl;
    }

    // Disconnect the client socket
    client.Disconnect();
    std::cout << "Client disconnected." << std::endl;

    return 0;
}
```

This structure is ideal for developers looking to integrate low-level networking into their applications while awaiting upcoming client/server abstractions.

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests for any enhancements or bug fixes.

## License

This project is licensed under the GPL v3 License - see the [LICENSE](LICENSE) file for details.
