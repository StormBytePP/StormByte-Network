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
#include <utility>

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

namespace Test {
	namespace Packet {
		enum class Opcode: unsigned short {
			C_MSG_ASKNAMELIST = Net::Transport::Packet::PROCESS_THRESHOLD,
			S_MSG_RESPONDNAMELIST,
			C_MSG_ASKRANDOMNUMBER,
			S_MSG_RESPONDRANDOMNUMBER,
			C_MSG_SENDLARGEDATA,
			S_MSG_REPLYLARGEDATAECHOED
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
				/** Build from size (fills with repeat char). */
				explicit LargeData(std::size_t size) noexcept
					: Generic(Opcode::C_MSG_SENDLARGEDATA),
					m_data(std::string(size, large_data_repeat_char)) {}

				/** Take ownership of existing data (deserialize / echo path). */
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
				Packet::LargeData request_packet(size);
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
						auto large_data_packet = std::static_pointer_cast<Packet::LargeData>(packet);
						// Move payload into the answer — no extra 20 MiB copy
						return std::make_shared<Packet::AnswerLargeDataEchoed>(large_data_packet->TakeData());
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

int main() {
	int result = 0;
	result += TestRequestNameList();
	result += TestRequestRandomNumber();
	result += TestRequestLargeDataEchoed();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
