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
