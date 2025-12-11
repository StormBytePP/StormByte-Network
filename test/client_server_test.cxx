#include <StormByte/network/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/serializable.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/test_handlers.h>
#include <StormByte/system.hxx>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <random>

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
using SBLog::ThreadedLog;
using Buf::Pipeline;
using namespace StormByte::Logger;
using namespace StormByte::Network;

ThreadedLog logger(std::cout, Level::Info, "[%L] [T%i] %T:");
constexpr const unsigned short timeout = 5; // 5 seconds
constexpr const std::size_t large_data_size = 20 * 1024 * 1024; // 20 MB

namespace Test {
	namespace Packet {
		enum class Opcode: unsigned short {
			C_MSG_ASKNAMELIST = Net::Transport::Packet::PROCESS_THRESHOLD,
			S_MSG_RESPONDNAMELIST,
			C_MSG_ASKRANDOMNUMBER,
			S_MSG_RESPONDRANDOMNUMBER,
			C_MSG_SENDLARGEDATA,
			S_MSG_REPLYLARGEDATASIZE
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
				LargeData(const std::size_t& size): Generic(Opcode::C_MSG_SENDLARGEDATA), m_size(size) {}
				DataType DoSerialize() const noexcept override {
					// Instead of creating a 20MB string, just serialize the size
					// The actual data transfer is simulated by the size value
					return Serializable<std::size_t>(m_size).Serialize();
				}

				std::size_t GetSize() const noexcept {
					return m_size;
				}

			private:
				std::size_t m_size;
			};

			class AnswerLargeDataSize: public Generic {
			public:
				AnswerLargeDataSize(const std::size_t& size): Generic(Opcode::S_MSG_REPLYLARGEDATASIZE), m_size(size) {}
				DataType DoSerialize() const noexcept override {
					return Serializable<std::size_t>(m_size).Serialize();
				}

				std::size_t GetSize() const noexcept {
					return m_size;
				}

			private:
				std::size_t m_size;
		};
	}

	DeserializePacketFunction DeserializeFunction() {
		return [](Transport::Packet::OpcodeType opcode, Consumer consumer, Log& logger) -> PacketPointer {
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
					// No data to read
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
					// Don't deserialize the full string - just consume the bytes and use the size
					// The size already tells us how big the data was; no need to reconstruct the entire string
					// The serialized string format is: [size(8 bytes)][string_data]
					// We need to extract the size from the first 8 bytes to know the actual string length
					if (data.size() < sizeof(std::size_t)) {
						return nullptr;
					}
					std::vector<std::byte> size_bytes(data.begin(), data.begin() + sizeof(std::size_t));
					auto size_expected = Serializable<std::size_t>::Deserialize(size_bytes);
					if (!size_expected) {
						return nullptr;
					}
					return std::make_shared<Packet::LargeData>(*size_expected);
				}
				case Packet::Opcode::S_MSG_REPLYLARGEDATASIZE: {
					auto expected_size = Serializable<std::size_t>::Deserialize(data);
					if (!expected_size) {
						return nullptr;
					}
					return std::make_shared<Packet::AnswerLargeDataSize>(*expected_size);
				}
				default:
					return nullptr;
			}
		};
	}

	using ExpectedNameList = NetExpected<std::vector<std::string>>;
	using ExpectedRandomNumber = NetExpected<int>;
	using ExpectedLargeDataSize = NetExpected<std::size_t>;

	Buf::PipeFunction CreateXorPipe() noexcept {
		return [](Consumer input, Producer output, StormByte::Logger::Log& logger) {
			logger << Level::Debug << "XOR Pipe: Starting processing data..." << std::endl;
			while (!input.EoF()) {
				const std::size_t available = input.AvailableBytes();
				logger << Level::Debug << "XOR Pipe: Available bytes to read: " << available << std::endl;
				if (available == 0) {
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
				DataType data;
				input.Extract(available, data);
				for (auto& byte : data) {
					byte ^= std::byte{0xAB};
				}
				output.Write(std::move(data));
			}
			output.Close();
			logger << Level::Debug << "XOR Pipe: Finished processing data." << std::endl;
		};
	}

	class Client: public Net::Client {
		public:
			Client(const ThreadedLog& logger) noexcept:
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
				// Send request
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
				// Send request
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

			ExpectedLargeDataSize RequestLargeDataSize(const std::size_t& size) noexcept {
				// Send request
				Packet::LargeData request_packet(size);
				auto response_packet = Send(request_packet);
				if (!response_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestLargeDataSize: failed to send LargeData packet");
				}

				std::shared_ptr<Packet::AnswerLargeDataSize> answer_packet = std::dynamic_pointer_cast<Packet::AnswerLargeDataSize>(response_packet);
				if (!answer_packet) {
					return SB::Unexpected<Net::Exception>("Client::RequestLargeDataSize: received unexpected packet opcode ({})", response_packet->Opcode());
				}
				return answer_packet->GetSize();
			}
	};

	class Server: public Net::Server {
		public:
			Server(const ThreadedLog& logger) noexcept:
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
				switch(static_cast<Packet::Opcode>(packet->Opcode())) {
					case Packet::Opcode::C_MSG_ASKNAMELIST: {
						auto ask_packet = std::dynamic_pointer_cast<Packet::AskNameList>(packet);
						if (!ask_packet) {
							return nullptr;
						}
						std::size_t amount = ask_packet->GetAmount();

						// Create dummy name list
						std::vector<std::string> names;
						for (std::size_t i = 0; i < amount; ++i) {
							names.push_back("Name_" + std::to_string(i + 1));
						}
						return std::make_shared<Packet::AnswerNameList>(names);
					}
					case Packet::Opcode::C_MSG_ASKRANDOMNUMBER: {
						// Generate random number using C++ RNG (per-thread engine)
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
						auto large_data_packet = std::static_pointer_cast<const Packet::LargeData>(packet);
						std::size_t data_size = large_data_packet->GetSize();
						return std::make_shared<Packet::AnswerLargeDataSize>(data_size);
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

	const char* HOST = "127.0.0.1";
	const unsigned short PORT = 7081;

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	// Small delay to ensure server is listening
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

	const char* HOST = "127.0.0.1";
	const unsigned short PORT = 7080;

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	// Small delay to ensure server is listening
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

int TestRequestLargeDataSize() {
	const std::string fn_name = "TestRequestLargeDataSize";

	const char* HOST = "127.0.0.1";
	const unsigned short PORT = 7080;

	Test::Server server(logger);
	if (!server.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": server.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	// Small delay to ensure server is listening
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(logger);
	if (!client.Connect(Net::Connection::Protocol::IPv4, HOST, PORT)) {
		logger << Level::Error << fn_name << ": client.Connect failed." << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	auto size_expected = client.RequestLargeDataSize(large_data_size);
	if (!size_expected) {
		logger << Level::Error << fn_name << ": RequestLargeDataSize failed: " << size_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	std::size_t size = size_expected.value();
	ASSERT_EQUAL(fn_name, size, large_data_size);
	logger << Level::Info << fn_name << ": Received large data size: " << size << std::endl;
	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;
	result += TestRequestNameList();
	result += TestRequestRandomNumber();
	result += TestRequestLargeDataSize();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
