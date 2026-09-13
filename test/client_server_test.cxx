/*
* Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
*
* This file is part of StormByte-Network.
*
* StormByte-Network is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License version 3
* or later, as published by the Free Software Foundation.
*
* StormByte-Network is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with StormByte-Network. If not, see
* <https://www.gnu.org/licenses/lgpl-3.0.html>.
*/

#include <StormByte/network/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/serializable.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/test_handlers.h>
#include <StormByte/system.hxx>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <thread>
#include <random>
#include <utility>
#ifdef UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
// Namespace aliases and commonly used types to reduce verbosity
namespace SB = StormByte;
namespace Net = SB::Network;
namespace Buf = SB::Buffer;
namespace SBLog = SB::Logger;
namespace Transport = Net::Transport;
template<typename T>
using Serializable = SB::Serializable<T>;
template<typename T>
using NetExpected = SB::Expected<T, Net::Exception>;
using Buf::DataType;
using Buf::Consumer;
using Buf::Producer;
using Buf::ExternalReader;
using Buf::ExternalWriter;
using Buf::ExecutionMode;
using SBLog::ThreadedLog;
using Buf::Pipeline;
using namespace StormByte::Logger;
using namespace StormByte::Network;
std::shared_ptr<Log> logger = std::make_shared<ThreadedLog>(std::cout, Level::Info, "[%L] [T%i] %T:");
constexpr const unsigned short timeout = 5; // 5 seconds
constexpr const std::size_t large_data_size = 20 * 1024 * 1024; // 20 MB
constexpr const char large_data_repeat_char = 'x';
constexpr const char* HOST = "localhost";
constexpr const unsigned short PORT = 7080;
#ifdef WINDOWS
using RawSocket = SOCKET;
constexpr RawSocket invalid_raw_socket = INVALID_SOCKET;
#else
using RawSocket = int;
constexpr RawSocket invalid_raw_socket = -1;
#endif

RawSocket ConnectRawSocket() {
	RawSocket socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_handle == invalid_raw_socket) {
		return invalid_raw_socket;
	}
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
#ifdef WINDOWS
		closesocket(socket_handle);
#else
		close(socket_handle);
#endif
		return invalid_raw_socket;
	}
	if (::connect(socket_handle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
#ifdef WINDOWS
		closesocket(socket_handle);
#else
		close(socket_handle);
#endif
		return invalid_raw_socket;
	}
	return socket_handle;
}

void CloseRawSocket(RawSocket socket_handle) noexcept {
#ifdef WINDOWS
	closesocket(socket_handle);
#else
	close(socket_handle);
#endif
}

bool SendRawBytes(RawSocket socket_handle, std::span<const std::byte> data) {
	while (!data.empty()) {
#ifdef WINDOWS
		const int sent = ::send(socket_handle, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()), 0);
#else
		const ssize_t sent = ::send(socket_handle, data.data(), data.size(), 0);
#endif
		if (sent <= 0) {
			return false;
		}
		data = data.subspan(static_cast<std::size_t>(sent));
	}
	return true;
}

bool ReceiveRawBytes(RawSocket socket_handle, std::span<std::byte> data) {
	while (!data.empty()) {
#ifdef WINDOWS
		const int received = ::recv(socket_handle, reinterpret_cast<char*>(data.data()), static_cast<int>(data.size()), 0);
#else
		const ssize_t received = ::recv(socket_handle, data.data(), data.size(), 0);
#endif
		if (received <= 0) {
			return false;
		}
		data = data.subspan(static_cast<std::size_t>(received));
	}
	return true;
}
namespace Test {
	namespace Packet {
		enum class Opcode: unsigned short {
			C_MSG_ASKNAMELIST = Net::Transport::Packet::PROCESS_THRESHOLD,
			S_MSG_RESPONDNAMELIST,
			C_MSG_ASKRANDOMNUMBER,
			S_MSG_RESPONDRANDOMNUMBER,
			C_MSG_SENDLARGEDATA,
			S_MSG_REPLYLARGEDATAECHOED,
			C_MSG_PING,
			S_MSG_PONG,
			C_MSG_ECHOTEXT,
			S_MSG_REPLYTEXT,
			C_MSG_SUMNUMBERS,
			S_MSG_REPLYSUM
		};
		class Generic: public Transport::Packet {
			public:
				Generic(const enum Opcode& opcode): Transport::Packet(static_cast<Transport::Packet::OpcodeType>(opcode)) {}
		};
		class AskNameList: public Generic {
			public:
				AskNameList(const std::size_t& amount): Generic(Opcode::C_MSG_ASKNAMELIST), m_amount(amount) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::size_t>(m_amount).Serialize();
				}
				std::size_t GetAmount() const noexcept {
					return m_amount;
				}
			private:
				std::size_t m_amount;
		};
		class AnswerNameList: public Generic {
			public:
				AnswerNameList(const std::vector<std::string>& names): Generic(Opcode::S_MSG_RESPONDNAMELIST), m_names(names) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::vector<std::string>>(m_names).Serialize();
				}
				const std::vector<std::string>& GetNames() const noexcept {
					return m_names;
				}
			private:
				std::vector<std::string> m_names;
		};
		class AskRandomNumber: public Generic {
			public:
				AskRandomNumber(): Generic(Opcode::C_MSG_ASKRANDOMNUMBER) {}
				DataType DoSerialize() const noexcept override {
					return {};
				}
		};
		class AnswerRandomNumber: public Generic {
			public:
				AnswerRandomNumber(const int& number): Generic(Opcode::S_MSG_RESPONDRANDOMNUMBER), m_number(number) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<int>(m_number).Serialize();
				}
				int GetNumber() const noexcept {
					return m_number;
				}
			private:
				int m_number;
		};
		class LargeData: public Generic {
			public:
				explicit LargeData(std::string data) noexcept
					: Generic(Opcode::C_MSG_SENDLARGEDATA),
					m_data(std::move(data)) {}

				DataType DoSerialize() const noexcept override {
					return Serializable<std::string>(m_data).Serialize();
				}

				const std::string& GetData() const noexcept {
					return m_data;
				}

				/** Move payload out (server echo without extra copy). */
				std::string TakeData() noexcept {
					return std::move(m_data);
				}

			private:
				std::string m_data;
		};

		class AnswerLargeDataEchoed: public Generic {
			public:
				explicit AnswerLargeDataEchoed(std::string data) noexcept
					: Generic(Opcode::S_MSG_REPLYLARGEDATAECHOED),
					m_data(std::move(data)) {}

				DataType DoSerialize() const noexcept override {
					return Serializable<std::string>(m_data).Serialize();
				}

				const std::string& GetData() const noexcept {
					return m_data;
				}

				std::string TakeData() noexcept {
					return std::move(m_data);
				}

			private:
				std::string m_data;
		};
		class Ping: public Generic {
			public:
				Ping(): Generic(Opcode::C_MSG_PING) {}
				DataType DoSerialize() const noexcept override {
					return {};
				}
		};
		class Pong: public Generic {
			public:
				Pong(): Generic(Opcode::S_MSG_PONG) {}
				DataType DoSerialize() const noexcept override {
					return {};
				}
		};
		class EchoText: public Generic {
			public:
				explicit EchoText(std::string text) noexcept:
					Generic(Opcode::C_MSG_ECHOTEXT), m_text(std::move(text)) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::string>(m_text).Serialize();
				}
				const std::string& GetText() const noexcept {
					return m_text;
				}
			private:
				std::string m_text;
		};
		class ReplyText: public Generic {
			public:
				explicit ReplyText(std::string text) noexcept:
					Generic(Opcode::S_MSG_REPLYTEXT), m_text(std::move(text)) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::string>(m_text).Serialize();
				}
				const std::string& GetText() const noexcept {
					return m_text;
				}
			private:
				std::string m_text;
		};
		class SumNumbers: public Generic {
			public:
				explicit SumNumbers(std::vector<int> numbers) noexcept:
					Generic(Opcode::C_MSG_SUMNUMBERS), m_numbers(std::move(numbers)) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::vector<int>>(m_numbers).Serialize();
				}
				const std::vector<int>& GetNumbers() const noexcept {
					return m_numbers;
				}
			private:
				std::vector<int> m_numbers;
		};
		class ReplySum: public Generic {
			public:
				explicit ReplySum(const int& sum) noexcept:
					Generic(Opcode::S_MSG_REPLYSUM), m_sum(sum) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<int>(m_sum).Serialize();
				}
				int GetSum() const noexcept {
					return m_sum;
				}
			private:
				int m_sum;
		};
	}

	DeserializePacketFunction DeserializeFunction() {
		return [](Transport::Packet::OpcodeType opcode, Consumer consumer, std::shared_ptr<Log> logger) -> PacketPointer {
			(void)logger;
			DataType data;
			consumer.ExtractUntilEoF(data);
			switch(static_cast<Packet::Opcode>(opcode)) {
				case Packet::Opcode::C_MSG_ASKNAMELIST: {
					auto expected_amount = Serializable<std::size_t>::Deserialize(data);
					if (!expected_amount) {
						return nullptr;
					}
					return std::make_shared<Packet::AskNameList>(*expected_amount);
				}
				case Packet::Opcode::S_MSG_RESPONDNAMELIST: {
					auto expected_names = Serializable<std::vector<std::string>>::Deserialize(data);
					if (!expected_names) {
						return nullptr;
					}
					return std::make_shared<Packet::AnswerNameList>(*expected_names);
				}
				case Packet::Opcode::C_MSG_ASKRANDOMNUMBER: {
					return std::make_shared<Packet::AskRandomNumber>();
				}
				case Packet::Opcode::S_MSG_RESPONDRANDOMNUMBER: {
					auto expected_number = Serializable<int>::Deserialize(data);
					if (!expected_number) {
						return nullptr;
					}
					return std::make_shared<Packet::AnswerRandomNumber>(*expected_number);
				}
				case Packet::Opcode::C_MSG_SENDLARGEDATA: {
					// Real payload (moved into packet) — no second synthetic 20 MiB string
					auto expected_data = Serializable<std::string>::Deserialize(data);
					if (!expected_data) {
						return nullptr;
					}
					return std::make_shared<Packet::LargeData>(std::move(*expected_data));
				}
				case Packet::Opcode::S_MSG_REPLYLARGEDATAECHOED: {
					auto expected_data = Serializable<std::string>::Deserialize(data);
					if (!expected_data) {
						return nullptr;
					}
					return std::make_shared<Packet::AnswerLargeDataEchoed>(std::move(*expected_data));
				}
				case Packet::Opcode::C_MSG_PING:
					return std::make_shared<Packet::Ping>();
				case Packet::Opcode::S_MSG_PONG:
					return std::make_shared<Packet::Pong>();
				case Packet::Opcode::C_MSG_ECHOTEXT: {
					auto expected_text = Serializable<std::string>::Deserialize(data);
					if (!expected_text) {
						return nullptr;
					}
					return std::make_shared<Packet::EchoText>(std::move(*expected_text));
				}
				case Packet::Opcode::S_MSG_REPLYTEXT: {
					auto expected_text = Serializable<std::string>::Deserialize(data);
					if (!expected_text) {
						return nullptr;
					}
					return std::make_shared<Packet::ReplyText>(std::move(*expected_text));
				}
				case Packet::Opcode::C_MSG_SUMNUMBERS: {
					auto expected_numbers = Serializable<std::vector<int>>::Deserialize(data);
					if (!expected_numbers) {
						return nullptr;
					}
					return std::make_shared<Packet::SumNumbers>(std::move(*expected_numbers));
				}
				case Packet::Opcode::S_MSG_REPLYSUM: {
					auto expected_sum = Serializable<int>::Deserialize(data);
					if (!expected_sum) {
						return nullptr;
					}
					return std::make_shared<Packet::ReplySum>(*expected_sum);
				}
				default:
					return nullptr;
			}
		};
	}

	using ExpectedNameList = NetExpected<std::vector<std::string>>;
	using ExpectedRandomNumber = NetExpected<int>;
	using ExpectedLargeData = NetExpected<std::string>;

	/**
	 * @brief XOR transform stage compatible with the new Pipeline PipeFunction signature.
	 *
	 * Uses ExternalReader / ExternalWriter (not Consumer / Producer directly).
	 * Chunked Extract so intermediate LockFreeRings can stream under Async / Parallel Process.
	 */
	Buf::Pipeline::PipeFunction CreateXorPipe() noexcept {
		return [](ExternalReader& in, ExternalWriter& out, std::shared_ptr<Log> log) {
			log << Level::Debug << "XOR Pipe: Starting..." << std::endl;
			constexpr std::size_t max_chunk = 10 * 1024 * 1024;

			while (!in.EoF()) {
				DataType data;

				// Blocks until ≥1 byte or EoF/error (no yield spin)
				if (!in.Extract(1, data) || data.empty()) {
					if (in.EoF())
						break;
					continue;
				}

				// Non-blocking grab of the rest of the current burst (capped)
				const std::size_t extra = std::min(in.AvailableBytes(), max_chunk - data.size());
				if (extra > 0) {
					DataType more;
					if (in.Extract(extra, more) && !more.empty()) {
						data.insert(data.end(),
							std::make_move_iterator(more.begin()),
							std::make_move_iterator(more.end()));
					}
				}

				for (auto& b : data)
					b ^= std::byte{0xAB};

				if (!out.Write(std::move(data))) {
					log << Level::Error << "XOR Pipe: Write failed" << std::endl;
					out.SetError();
					return;
				}
			}

			out.Close();
			log << Level::Debug << "XOR Pipe: Finished." << std::endl;
		};
	}

	class Client: public Net::Client {
		public:
			Client(std::shared_ptr<Log> logger) noexcept:
			Net::Client(DeserializeFunction(), logger) {}
			~Client() noexcept = default;

			Pipeline InputPipeline() const noexcept override {
				Pipeline pipeline;
				pipeline.AddPipe(CreateXorPipe());
				return pipeline;
			}
			Pipeline OutputPipeline() const noexcept override {
				Pipeline pipeline;
				pipeline.AddPipe(CreateXorPipe());
				return pipeline;
			}

			ExpectedNameList RequestNameList(const std::size_t& amount) noexcept {
				Packet::AskNameList request_packet(amount);
				auto received_packet = Send(request_packet);
				if (!received_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestNameList: failed to send/receive AskNameList packet");
				}

				std::shared_ptr<Packet::AnswerNameList> namelist_packet = std::dynamic_pointer_cast<Packet::AnswerNameList>(received_packet);
				if (!namelist_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestNameList: received unexpected packet opcode ({})", received_packet->Opcode());
				}
				return namelist_packet->GetNames();
			}

			ExpectedRandomNumber RequestRandomNumber() noexcept {
				Packet::AskRandomNumber request_packet;
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestRandomNumber: failed to send AskRandomNumber packet");
				}

				std::shared_ptr<Packet::AnswerRandomNumber> answer_packet = std::dynamic_pointer_cast<Packet::AnswerRandomNumber>(response_packet);
				if (!answer_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestRandomNumber: received unexpected packet opcode ({})", response_packet->Opcode());
				}
				return answer_packet->GetNumber();
			}

			ExpectedLargeData RequestLargeDataEcho(const std::size_t& size) noexcept {
				Packet::LargeData request_packet(std::string(size, large_data_repeat_char));
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestLargeDataSize: failed to send LargeData packet");
				}

				std::shared_ptr<Packet::AnswerLargeDataEchoed> answer_packet = std::dynamic_pointer_cast<Packet::AnswerLargeDataEchoed>(response_packet);
				if (!answer_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestLargeDataSize: received unexpected packet opcode ({})", response_packet->Opcode());
				}
				// Move data out of the packet so the shared_ptr can die without retaining 20 MiB
				return answer_packet->TakeData();
			}

			bool RequestPing() noexcept {
				Packet::Ping request_packet;
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return false;
				}
				return std::dynamic_pointer_cast<Packet::Pong>(response_packet) != nullptr;
			}

			NetExpected<std::string> RequestEchoText(std::string text) noexcept {
				Packet::EchoText request_packet(std::move(text));
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestEchoText: failed to send/receive packet");
				}
				auto answer_packet = std::dynamic_pointer_cast<Packet::ReplyText>(response_packet);
				if (!answer_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestEchoText: received unexpected packet opcode ({})", response_packet->Opcode());
				}
				return answer_packet->GetText();
			}

			NetExpected<int> RequestSum(std::vector<int> numbers) noexcept {
				Packet::SumNumbers request_packet(std::move(numbers));
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestSum: failed to send/receive packet");
				}
				auto answer_packet = std::dynamic_pointer_cast<Packet::ReplySum>(response_packet);
				if (!answer_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestSum: received unexpected packet opcode ({})", response_packet->Opcode());
				}
				return answer_packet->GetSum();
			}
	};

	class Server: public Net::Server {
		public:
			Server(std::shared_ptr<Log> logger) noexcept:
			Net::Server(DeserializeFunction(), logger) {}
			~Server() noexcept = default;

			Pipeline InputPipeline() const noexcept override {
				Pipeline pipeline;
				pipeline.AddPipe(CreateXorPipe());
				return pipeline;
			}
			Pipeline OutputPipeline() const noexcept override {
				Pipeline pipeline;
				pipeline.AddPipe(CreateXorPipe());
				return pipeline;
			}

		private:
			PacketPointer ProcessClientPacket(const std::string& client_uuid, PacketPointer packet) noexcept override {
				(void)client_uuid;
				switch(static_cast<Packet::Opcode>(packet->Opcode())) {
					case Packet::Opcode::C_MSG_ASKNAMELIST: {
						auto ask_packet = std::dynamic_pointer_cast<Packet::AskNameList>(packet);
						if (!ask_packet) {
							return nullptr;
						}
						std::size_t amount = ask_packet->GetAmount();

						std::vector<std::string> names;
						for (std::size_t i = 0; i < amount; ++i) {
							names.push_back("Name_" + std::to_string(i + 1));
						}
						return std::make_shared<Packet::AnswerNameList>(names);
					}
					case Packet::Opcode::C_MSG_ASKRANDOMNUMBER: {
						static thread_local std::mt19937 gen{[](){
							std::random_device rd;
							unsigned int seed = rd();
							if (seed == 0) {
								seed = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
							}
							return seed;
						}()};
						std::uniform_int_distribution<int> dist(0, 99);
						int random_number = dist(gen);
						return std::make_shared<Packet::AnswerRandomNumber>(random_number);
					}
					case Packet::Opcode::C_MSG_SENDLARGEDATA: {
						auto large_data_packet = std::dynamic_pointer_cast<Packet::LargeData>(packet);
						if (!large_data_packet) {
							return nullptr;
						}
						// Move payload into the answer — no extra 20 MiB copy
						return std::make_shared<Packet::AnswerLargeDataEchoed>(large_data_packet->TakeData());
					}
					case Packet::Opcode::C_MSG_PING:
						return std::make_shared<Packet::Pong>();
					case Packet::Opcode::C_MSG_ECHOTEXT: {
						auto text_packet = std::dynamic_pointer_cast<Packet::EchoText>(packet);
						if (!text_packet) {
							return nullptr;
						}
						return std::make_shared<Packet::ReplyText>(text_packet->GetText());
					}
					case Packet::Opcode::C_MSG_SUMNUMBERS: {
						auto numbers_packet = std::dynamic_pointer_cast<Packet::SumNumbers>(packet);
						if (!numbers_packet) {
							return nullptr;
						}
						const auto& numbers = numbers_packet->GetNumbers();
						const int sum = std::accumulate(numbers.begin(), numbers.end(), 0);
						return std::make_shared<Packet::ReplySum>(sum);
					}
					default:
						return nullptr;
				}
				return {};
			}
	};
}

int TestRequestNameList() {
	const std::string fn_name = "TestRequestNameList";

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(logger);
	if (!client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	const std::size_t amount = 3;
	auto names_expected = client.RequestNameList(amount);
	if (!names_expected) {
		logger << Level::Error << fn_name << ": RequestNameList failed: " << names_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	auto names = names_expected.value();
	std::string all_names;
	ASSERT_TRUE(fn_name, names.size() == amount);
	for (std::size_t i = 0; i < amount; ++i) {
		all_names += names[i] + " ";
		ASSERT_TRUE(fn_name, names[i] == ("Name_" + std::to_string(i + 1)));
	}
	logger << Level::Info << fn_name << ": Received names: " << all_names << std::endl;

	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestRequestRandomNumber() {
	const std::string fn_name = "TestRequestRandomNumber";

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(logger);
	if (!client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	auto number_expected = client.RequestRandomNumber();
	if (!number_expected) {
		logger << Level::Error << fn_name << ": RequestRandomNumber failed: " << number_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	int n = number_expected.value();
	ASSERT_TRUE(fn_name, n >= 0 && n < 100);
	logger << Level::Info << fn_name << ": Received random number: " << n << std::endl;
	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestRequestLargeDataEchoed() {
	const std::string fn_name = "TestRequestLargeDataEchoed";

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(logger);
	if (!client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	auto data_expected = client.RequestLargeDataEcho(large_data_size);
	if (!data_expected) {
		logger << Level::Error << fn_name << ": RequestLargeDataEcho failed: " << data_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	// Single 20 MiB buffer: size + content check without a second reference string
	const std::string& data = data_expected.value();
	ASSERT_EQUAL(fn_name, data.size(), large_data_size);
	ASSERT_TRUE(fn_name, data.find_first_not_of(large_data_repeat_char) == std::string::npos);

	logger << Level::Info << fn_name << ": Received large data size: " << humanreadable_bytes << data.size()
		<< nohumanreadable << std::endl;
	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestRequestAdditionalCommands() {
	const std::string fn_name = "TestRequestAdditionalCommands";

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(logger);
	if (!client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	ASSERT_TRUE(fn_name, client.RequestPing());

	const std::string text = "StormByte network command with spaces and UTF-8: cafe";
	auto echoed_text = client.RequestEchoText(text);
	ASSERT_TRUE(fn_name, echoed_text.has_value());
	ASSERT_TRUE(fn_name, echoed_text.value() == text);

	const std::vector<int> numbers{ -100, 0, 1, 2, 42, 1000 };
	auto sum = client.RequestSum(numbers);
	ASSERT_TRUE(fn_name, sum.has_value());
	ASSERT_EQUAL(fn_name, sum.value(), 945);

	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestClientDisconnectKeepsServerAlive() {
	const std::string fn_name = "TestClientDisconnectKeepsServerAlive";

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client first_client(logger);
	if (!first_client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": first client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	ASSERT_TRUE(fn_name, first_client.RequestPing());
	first_client.Disconnect();

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client second_client(logger);
	if (!second_client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": second client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	ASSERT_TRUE(fn_name, second_client.RequestPing());
	second_client.Disconnect();

	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestFragmentedAndBatchedFrames() {
	const std::string fn_name = "TestFragmentedAndBatchedFrames";
	constexpr std::size_t frame_header_size = sizeof(Transport::Packet::OpcodeType) + sizeof(std::size_t);

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	const RawSocket socket_handle = ConnectRawSocket();
	if (socket_handle == invalid_raw_socket) {
		logger << Level::Error << fn_name << ": raw socket connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	auto make_wire_frame = [](const Test::Packet::Opcode opcode, const DataType& payload) {
		DataType frame = Serializable<Transport::Packet::OpcodeType>(
			static_cast<Transport::Packet::OpcodeType>(opcode)).Serialize();
		const DataType payload_size = Serializable<std::size_t>(payload.size()).Serialize();
		frame.insert(frame.end(), payload_size.begin(), payload_size.end());
		frame.insert(frame.end(), payload.begin(), payload.end());
		return frame;
	};

	auto receive_frame = [&](const Test::Packet::Opcode expected_opcode, const std::string* expected_text = nullptr) -> bool {
		DataType header(frame_header_size);
		if (!ReceiveRawBytes(socket_handle, std::span<std::byte>(header.data(), header.size()))) {
			return false;
		}
		auto opcode = Serializable<Transport::Packet::OpcodeType>::Deserialize(header);
		auto payload_size = Serializable<std::size_t>::Deserialize(
			DataType(header.begin() + sizeof(Transport::Packet::OpcodeType), header.end()));
		if (!opcode || !payload_size || *opcode != static_cast<Transport::Packet::OpcodeType>(expected_opcode)) {
			return false;
		}
		if (*payload_size == 0) {
			return expected_text == nullptr;
		}
		if (expected_text == nullptr) {
			return false;
		}
		DataType payload(*payload_size);
		if (!ReceiveRawBytes(socket_handle, std::span<std::byte>(payload.data(), payload.size()))) {
			return false;
		}
		for (auto& byte: payload) {
			byte ^= std::byte{0xAB};
		}
		auto text = Serializable<std::string>::Deserialize(payload);
		return text && *text == *expected_text;
	};

	const DataType ping_data = make_wire_frame(Test::Packet::Opcode::C_MSG_PING, {});
	ASSERT_TRUE(fn_name, SendRawBytes(socket_handle, std::span<const std::byte>(ping_data.data(), 1)));
	ASSERT_TRUE(fn_name, SendRawBytes(socket_handle, std::span<const std::byte>(ping_data.data() + 1, ping_data.size() - 1)));
	ASSERT_TRUE(fn_name, receive_frame(Test::Packet::Opcode::S_MSG_PONG));

	const std::string text = "fragmented payload";
	DataType text_payload = Serializable<std::string>(text).Serialize();
	for (auto& byte: text_payload) {
		byte ^= std::byte{0xAB};
	}
	const DataType text_data = make_wire_frame(Test::Packet::Opcode::C_MSG_ECHOTEXT, text_payload);
	const std::size_t split = frame_header_size + 2;
	ASSERT_TRUE(fn_name, SendRawBytes(socket_handle, std::span<const std::byte>(text_data.data(), split)));
	ASSERT_TRUE(fn_name, SendRawBytes(socket_handle, std::span<const std::byte>(text_data.data() + split, text_data.size() - split)));
	ASSERT_TRUE(fn_name, receive_frame(Test::Packet::Opcode::S_MSG_REPLYTEXT, &text));

	DataType batched;
	batched.insert(batched.end(), ping_data.begin(), ping_data.end());
	batched.insert(batched.end(), ping_data.begin(), ping_data.end());
	ASSERT_TRUE(fn_name, SendRawBytes(socket_handle, std::span<const std::byte>(batched.data(), batched.size())));
	ASSERT_TRUE(fn_name, receive_frame(Test::Packet::Opcode::S_MSG_PONG));
	ASSERT_TRUE(fn_name, receive_frame(Test::Packet::Opcode::S_MSG_PONG));

	CloseRawSocket(socket_handle);
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;
	result += TestRequestNameList();
	result += TestRequestRandomNumber();
	result += TestRequestLargeDataEchoed();
	result += TestRequestAdditionalCommands();
	result += TestClientDisconnectKeepsServerAlive();
	result += TestFragmentedAndBatchedFrames();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
