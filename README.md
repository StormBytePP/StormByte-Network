# StormByte-Network

![Multiplatform](https://img.shields.io/badge/Linux%20%7C%20Windows%20%7C%20macOS-Supported-1793D1)
![C++26](https://img.shields.io/badge/C%2B%2B-26-00599C?logo=c%2B%2B&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.28+-064F8C?logo=cmake&logoColor=white)
![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)
[![CI](https://github.com/StormBytePP/StormByte-Network/actions/workflows/ci.yml/badge.svg)](https://github.com/StormBytePP/StormByte-Network/actions/workflows/ci.yml)
[![Sponsor](https://img.shields.io/badge/Sponsor-GitHub-ea4aaa?logo=github-sponsors&logoColor=white)](https://github.com/sponsors/StormBytePP)

StormByte-Network is the C++26 networking module of the [StormByte](https://dev.stormbyte.org/StormByte) suite.

It is not a thin socket wrapper. You inherit `Client` or `Server`, define packets, and attach Buffer pipelines. POSIX and Winsock, framing, accept loops and per-client workers stay private.

## Table of Contents

- [Repository](#repository)
- [Installation](#installation)
- [Why StormByte-Network](#why-stormbyte-network)
- [Features](#features)
- [Dependencies](#dependencies)
- [The rest of the suite](#the-rest-of-the-suite)
- [Public API](#public-api)
- [Examples](#examples)
	- [A client](#a-client)
	- [A server](#a-server)
	- [A packet](#a-packet)
- [Design notes](#design-notes)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)

## Repository

- [StormByte-Network](https://github.com/StormBytePP/StormByte-Network)

## Installation

```bash
git clone https://github.com/StormBytePP/StormByte-Network.git
cd StormByte-Network
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build
```

## Why StormByte-Network

| Goal | How it is achieved |
|------|--------------------|
| **Inherit, don't wrap sockets** | `Client` / `Server` are abstract application endpoints. |
| **Framed messages** | `Transport::Packet` + private `Frame` (opcode, size, payload). |
| **Buffer as I/O** | Pipelines on inbound/outbound payloads; Reader/Writer adapters. |
| **IPv4 and IPv6** | `Connection::Protocol`. |
| **Cross-platform** | POSIX and Winsock behind `Socket` / `Handler`. |

## Features

- Abstract `Endpoint`, `Client`, `Server`
- Packet factory (`DeserializePacketFunction`)
- Connection status, read/write results
- Request/response (`Send`) and fire-and-forget (`Reply`)
- Server accept thread + one worker per client
- Optional payload processing for opcodes ≥ `Packet::PROCESS_THRESHOLD`

## Dependencies

| Dependency | Role |
|------------|------|
| StormByte (base) | Expected, exceptions, visibility |
| StormByte-Buffer | FIFO, Pipeline, Consumer, External I/O |
| StormByte-Logger | Diagnostics |

## The rest of the suite

- [Base](https://dev.stormbyte.org/StormByte) — foundation: Expected, exceptions, visibility, helpers
- [Buffer](https://dev.stormbyte.org/StormByte-Buffer) — FIFO, pipelines, consumers and producers
- [Config](https://dev.stormbyte.org/StormByte-Config) — typed configuration trees
- [Crypto](https://dev.stormbyte.org/StormByte-Crypto) — hash, compress, encrypt, sign, key agreement
- [Database](https://dev.stormbyte.org/StormByte-Database) — one API over SQLite, PostgreSQL and MariaDB
- [Logger](https://dev.stormbyte.org/StormByte-Logger) — levels, redaction, threaded sinks
- [Multimedia](https://dev.stormbyte.org/StormByte-Multimedia) — FFmpeg-backed media engine
- **Network** (this repository)
- [System](https://dev.stormbyte.org/StormByte-System) — process, platform and system helpers

## Public API

Under `StormByte::Network`:

| Type | Role |
|------|------|
| `Client` | Inherit; implement pipelines; call `Send` |
| `Server` | Inherit; implement `ProcessClientPacket` |
| `Transport::Packet` | Inherit; implement `DoSerialize` |
| `Connection::Protocol` | IPv4 / IPv6 |
| `Connection::Status` | Lifecycle |
| `Exception` / `ConnectionError` / `ConnectionClosed` | Errors |

Sockets, frames and Winsock bootstrap are private.

## Examples

### A client

```cpp
#include <StormByte/network/client.hxx>

class AppClient : public StormByte::Network::Client {
public:
	AppClient(const StormByte::Network::DeserializePacketFunction& fn,
	          std::shared_ptr<StormByte::Logger::Log> log)
		: Client(fn, log) {}

protected:
	StormByte::Buffer::Pipeline InputPipeline() const noexcept override {
		return {};
	}
	StormByte::Buffer::Pipeline OutputPipeline() const noexcept override {
		return {};
	}
};
```

### A server

```cpp
#include <StormByte/network/server.hxx>

class AppServer : public StormByte::Network::Server {
public:
	using Server::Server;

protected:
	StormByte::Buffer::Pipeline InputPipeline() const noexcept override { return {}; }
	StormByte::Buffer::Pipeline OutputPipeline() const noexcept override { return {}; }

	StormByte::Network::PacketPointer ProcessClientPacket(
		const std::string& uuid,
		StormByte::Network::PacketPointer packet) noexcept override {
		(void)uuid;
		return packet;
	}
};
```

### A packet

```cpp
#include <StormByte/network/transport/packet.hxx>

class PingPacket : public StormByte::Network::Transport::Packet {
public:
	PingPacket() : Packet(1) {}

protected:
	StormByte::Buffer::DataType DoSerialize() const noexcept override {
		return {};
	}
};
```

## Design notes

- One connection is not a thread-safe multiplex. The server isolates clients on their own threads.
- `Connect` on `Server` means bind + listen + accept loop.
- Frame layout uses host `size_t` for payload length. Same architecture on both ends.
- Pipelines run only when the opcode is at or above `PROCESS_THRESHOLD`.

## Testing

Enable tests in CMake (`ENABLE_TEST`) and run CTest from the build tree.

## Contributing

Issues on GitHub. No wiki, no discussions.

## License

GNU Lesser General Public License v3 or later.

See [https://www.gnu.org/licenses/lgpl-3.0.html](https://www.gnu.org/licenses/lgpl-3.0.html).
