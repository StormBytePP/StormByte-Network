# Changelog

All notable changes to **StormByte-Network** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Summary]

StormByte Network is the C++26 networking layer of the StormByte suite.

Inherit `Client` or `Server`, define packets, and attach Buffer pipelines.
IPv4 and IPv6, framed request/response, POSIX and Winsock stay behind the public API.

## [1.0.0] - 2026-09-04

Initial public release of StormByte-Network.

### Added

- **Endpoint abstraction** (`Client` / `Server`) for inheritance-oriented application protocols
    - Pluggable `DeserializePacketFunction` for domain packet construction
    - Overridable `InputPipeline()` / `OutputPipeline()` (`Buffer::Pipeline`) for compression, encryption, etc.
    - Request/response via framed `Transport::Packet` (`Send` / `Reply`)
- **Client**
    - Connect over IPv4/IPv6 (`Connection::Protocol`)
    - Status query and clean disconnect
- **Server**
    - Listen/accept loop on a dedicated thread
    - Per-client worker threads and UUID-keyed client map
    - Pure virtual `ProcessClientPacket()` for application logic
    - Safe shutdown (stop accept, join workers, disconnect peers)
- **Transport layer**
    - `Packet` base: opcode + `DoSerialize()` payload hook
    - `PROCESS_THRESHOLD` to optionally pipeline payloads
    - `Frame` on-wire layout: opcode, payload size, payload
    - Pipeline processing on frame input/output when the threshold is met
- **Sockets**
    - Cross-platform TCP client/server (POSIX + Windows Winsock)
    - Non-blocking I/O, configurable SO_SNDBUF/SO_RCVBUF, TCP_NODELAY
    - Chunked send/receive with timeouts and exact-size `ReceiveInto`
    - `WaitForData` via epoll (Linux), poll (other UNIX), WSA events (Windows)
    - Peer shutdown detection, peek, and lightweight ping
    - `Reader` / `Writer` adapters implementing `Buffer::ExternalReader` / `ExternalWriter`
- **Connection helpers**
    - Singleton `Handler` (WSAStartup/cleanup, last-error helpers)
    - `Info` hostname resolution and sockaddr metadata
    - Connection status and read/write result enums with string helpers
- **Exceptions**: `ConnectionError`, `ConnectionClosed`, `PacketError`, `FrameError`
- CMake build, tests, LGPL-3.0-or-later

### Notes

- `Client` and `Server` are designed to be **subclassed**, not used as generic drop-in types without derivation.
- Public API surface is stable for the 1.x series; private socket/connection types remain implementation details.

[1.0.0]: https://github.com/StormBytePP/StormByte-Network/releases/tag/1.0.0
