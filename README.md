# StormByte

StormByte is a comprehensive, cross-platform C++ library aimed at easing system programming, configuration management, logging, and database handling tasks. This library provides a unified API that abstracts away the complexities and inconsistencies of different platforms (Windows, Linux).

## Features

- **Network**: Provides classes to handle portable network communication across Linux and Windows, including features such as asynchronous data handling, error management, and high-level socket APIs.
- **Encryption**: Offers robust encryption and hashing functionality, including AES, RSA, ECC, and SHA-256.

## Table of Contents

- [Repository](#repository)
- [Installation](#installation)
- [Modules](#modules)
	- [Base](https://dev.stormbyte.org/StormByte)
	- [Config](https://dev.stormbyte.org/StormByte-Config)
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

### Encryption

The `Encryption` module provides robust cryptographic functions, including support for AES, RSA, ECC, and SHA-256. These functions are designed to ensure secure data transmission and storage.

#### Supported Algorithms:
- **AES**: Symmetric encryption with password-based key derivation.
- **RSA**: Asymmetric encryption for secure key exchange.
- **ECC**: Elliptic Curve Cryptography for high-performance encryption.
- **SHA-256**: Secure hashing algorithm for integrity verification.

#### Example Usage

##### AES Encryption and Decryption
```cpp
#include <StormByte/network/data/encryption/aes.hxx>
#include <iostream>

int main() {
	using namespace StormByte::Network::Data::Encryption;

	std::string password = "StrongPassword";
	std::string message = "This is a sensitive message";

	// Encrypt the message
	auto encryptedResult = AES::Encrypt(message, password);
	if (encryptedResult) {
		auto encryptedBuffer = encryptedResult.get();
		std::cout << "Encrypted message: " << encryptedBuffer << std::endl;

		// Decrypt the message
		auto decryptedResult = AES::Decrypt(encryptedBuffer, password);
		if (decryptedResult) {
			auto decryptedMessage = decryptedResult.get();
			std::cout << "Decrypted message: " << decryptedMessage << std::endl;
		} else {
			std::cerr << "Decryption failed: " << decryptedResult.error()->what() << std::endl;
		}
	} else {
		std::cerr << "Encryption failed: " << encryptedResult.error()->what() << std::endl;
	}

	return 0;
}
```

##### RSA Encryption and Decryption
```cpp
#include <StormByte/network/data/encryption/rsa.hxx>
#include <iostream>

int main() {
	using namespace StormByte::Network::Data::Encryption;

	int keyStrength = 2048;
	std::string message = "Sensitive data";

	// Generate RSA key pair
	auto keyPairResult = RSA::GenerateKeyPair(keyStrength);
	if (keyPairResult) {
		auto keyPair = keyPairResult.get();
		std::cout << "Private Key: " << keyPair.Private << std::endl;
		std::cout << "Public Key: " << keyPair.Public << std::endl;

		// Encrypt the message
		auto encryptedResult = RSA::Encrypt(message, keyPair.Public);
		if (encryptedResult) {
			auto encryptedBuffer = encryptedResult.get();
			std::cout << "Encrypted message: " << encryptedBuffer << std::endl;

			// Decrypt the message
			auto decryptedResult = RSA::Decrypt(encryptedBuffer, keyPair.Private);
			if (decryptedResult) {
				auto decryptedMessage = decryptedResult.get();
				std::cout << "Decrypted message: " << decryptedMessage << std::endl;
			} else {
				std::cerr << "Decryption failed: " << decryptedResult.error()->what() << std::endl;
			}
		} else {
			std::cerr << "Encryption failed: " << encryptedResult.error()->what() << std::endl;
		}
	} else {
		std::cerr << "Key pair generation failed: " << keyPairResult.error()->what() << std::endl;
	}

	return 0;
}
```

##### ECC Encryption and Decryption
```cpp
#include <StormByte/network/data/encryption/ecc.hxx>
#include <iostream>

int main() {
	using namespace StormByte::Network::Data::Encryption;

	int curveId = 256; // Using P-256 curve
	std::string message = "Highly sensitive data";

	// Generate ECC key pair
	auto keyPairResult = ECC::GenerateKeyPair(curveId);
	if (keyPairResult) {
		auto keyPair = keyPairResult.get();
		std::cout << "Private Key: " << keyPair.Private << std::endl;
		std::cout << "Public Key: " << keyPair.Public << std::endl;

		// Encrypt the message
		auto encryptedResult = ECC::Encrypt(message, keyPair.Public);
		if (encryptedResult) {
			auto encryptedBuffer = encryptedResult.get();
			std::cout << "Encrypted message: " << encryptedBuffer << std::endl;

			// Decrypt the message
			auto decryptedResult = ECC::Decrypt(encryptedBuffer, keyPair.Private);
			if (decryptedResult) {
				auto decryptedMessage = decryptedResult.get();
				std::cout << "Decrypted message: " << decryptedMessage << std::endl;
			} else {
				std::cerr << "Decryption failed: " << decryptedResult.error()->what() << std::endl;
			}
		} else {
			std::cerr << "Encryption failed: " << encryptedResult.error()->what() << std::endl;
		}
	} else {
		std::cerr << "Key pair generation failed: " << keyPairResult.error()->what() << std::endl;
	}

	return 0;
}
```

##### SHA-256 Hashing
```cpp
#include <StormByte/network/data/encryption/sha256.hxx>
#include <iostream>

int main() {
	using namespace StormByte::Network::Data::Encryption;

	std::string data = "Data to be hashed";

	// Hash the data
	auto hashResult = SHA256::Hash(data);
	if (hashResult) {
		std::string hash = hashResult.get();
		std::cout << "SHA-256 Hash: " << hash << std::endl;
	} else {
		std::cerr << "Hashing failed: " << hashResult.error()->what() << std::endl;
	}

	return 0;
}
```