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

using namespace StormByte;
using namespace StormByte::Network;

int TestRequestNameList();
int TestRequestRandomNumber();

namespace Test {
	namespace Packets {
		enum class Opcode: unsigned short {
			C_MSG_ASKNAMELIST,
			S_MSG_RESPONDNAMELIST,
			C_MSG_ASKRANDOMNUMBER,
			S_MSG_RESPONDRANDOMNUMBER
		};

		class Generic: public Network::Packet {
			public:
				Generic(const enum Opcode& opcode): Network::Packet(static_cast<unsigned short>(opcode)) {}
		};

		class AskNameList: public Generic {
			public:
				AskNameList(const std::size_t& amount): Generic(Opcode::C_MSG_ASKNAMELIST), m_amount(amount) {}
				Buffer::FIFO DoSerialize() const noexcept override {
					return StormByte::Serializable<std::size_t>(m_amount).Serialize();
				}

			private:
				std::size_t m_amount;
		};

		class AnswerNameList: public Generic {
			public:
				AnswerNameList(const std::vector<std::string>& names): Generic(Opcode::S_MSG_RESPONDNAMELIST), m_names(names) {}
				Buffer::FIFO DoSerialize() const noexcept override {
					return StormByte::Serializable<std::vector<std::string>>(m_names).Serialize();
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
				Buffer::FIFO DoSerialize() const noexcept override {
					return Buffer::FIFO();
				}
		};

		class AnswerRandomNumber: public Generic {
			public:
				AnswerRandomNumber(const int& number): Generic(Opcode::S_MSG_RESPONDRANDOMNUMBER), m_number(number) {}
				Buffer::FIFO DoSerialize() const noexcept override {
					return StormByte::Serializable<int>(m_number).Serialize();
				}

				int GetNumber() const noexcept {
					return m_number;
				}

			private:
				int m_number;
		};
	}

	class Codec: public Network::Codec {
		public:
			Codec(const Logger::ThreadedLog& log) noexcept: Network::Codec(log) {}

			PointerType Clone() const override {
				return MakePointer<Codec>(*this);
			}
			PointerType Move() override {
				return MakePointer<Codec>(std::move(*this));
			}

			Network::ExpectedPacket DoEncode(const unsigned short& opcode, const std::size_t& size, const StormByte::Buffer::Consumer& consumer) const noexcept override {
				switch(static_cast<Packets::Opcode>(opcode)) {
					case Packets::Opcode::C_MSG_ASKNAMELIST: {
						auto expected_data = consumer.Read(size);
						if (!expected_data) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to read AskNameList data ({})", expected_data.error()->what());
						}
						auto expected_amount = StormByte::Serializable<std::size_t>::Deserialize(expected_data.value());
						if (!expected_amount) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to deserialize AskNameList amount ({})", expected_amount.error()->what());
						}
						return std::make_shared<Packets::AskNameList>(*expected_amount);
					}
					case Packets::Opcode::S_MSG_RESPONDNAMELIST: {
						auto expected_data = consumer.Read(size);
						if (!expected_data) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to read AnswerNameList data ({})", expected_data.error()->what());
						}
						auto expected_names = StormByte::Serializable<std::vector<std::string>>::Deserialize(expected_data.value());
						if (!expected_names) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to deserialize AnswerNameList names ({})", expected_names.error()->what());
						}
						return std::make_shared<Packets::AnswerNameList>(*expected_names);
					}
					case Packets::Opcode::C_MSG_ASKRANDOMNUMBER: {
						// No data to read
						return std::make_shared<Packets::AskRandomNumber>();
					}
					case Packets::Opcode::S_MSG_RESPONDRANDOMNUMBER: {
						auto expected_data = consumer.Read(size);
						if (!expected_data) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to read AnswerRandomNumber data ({})", expected_data.error()->what());
						}
						auto expected_number = StormByte::Serializable<int>::Deserialize(expected_data.value());
						if (!expected_number) {
							return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: failed to deserialize AnswerRandomNumber number ({})", expected_number.error()->what());
						}
						return std::make_shared<Packets::AnswerRandomNumber>(*expected_number);
					}
					default:
						return StormByte::Unexpected<Network::PacketError>("Codec::DoEncode: unknown packet opcode ({})", opcode);
				}
			}
	};

	using ExpectedNameList = StormByte::Expected<std::vector<std::string>, StormByte::Network::Exception>;
	using ExpectedRandomNumber = StormByte::Expected<int, StormByte::Network::Exception>;

	class Client: public Network::Client {
		public:
			Client(const enum Network::Protocol& protocol, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
			Network::Client(protocol, Test::Codec(logger), timeout, logger) {}
			~Client() noexcept = default;

			ExpectedNameList RequestNameList(const std::size_t& amount) noexcept {
				// Send request
				Packets::AskNameList request_packet(amount);
				auto send_expected = Send(request_packet);
				if (!send_expected) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestNameList: failed to send AskNameList packet ({})", send_expected.error()->what());
				}

				// Receive response
				auto receive_expected = Receive();
				if (!receive_expected) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestNameList: failed to receive AnswerNameList packet ({})", receive_expected.error()->what());
				}
				auto& received_packet = receive_expected.value();
				if (received_packet->Opcode() != static_cast<unsigned short>(Packets::Opcode::S_MSG_RESPONDNAMELIST)) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestNameList: received unexpected packet opcode ({})", received_packet->Opcode());
				}
				auto answer_packet = std::static_pointer_cast<Packets::AnswerNameList>(received_packet);
				return answer_packet->GetNames();
			}

			ExpectedRandomNumber RequestRandomNumber() noexcept {
				// Send request
				Packets::AskRandomNumber request_packet;
				auto send_expected = Send(request_packet);
				if (!send_expected) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestRandomNumber: failed to send AskRandomNumber packet ({})", send_expected.error()->what());
				}

				// Receive response
				auto receive_expected = Receive();
				if (!receive_expected) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestRandomNumber: failed to receive AnswerRandomNumber packet ({})", receive_expected.error()->what());
				}
				auto& received_packet = receive_expected.value();
				if (received_packet->Opcode() != static_cast<unsigned short>(Packets::Opcode::S_MSG_RESPONDRANDOMNUMBER)) {
					return StormByte::Unexpected<Network::Exception>("Client::RequestRandomNumber: received unexpected packet opcode ({})", received_packet->Opcode());
				}
				auto answer_packet = std::static_pointer_cast<Packets::AnswerRandomNumber>(received_packet);
				return answer_packet->GetNumber();
			}
	};

	class Server: public Network::Server {
		public:
			Server(const enum Network::Protocol& protocol, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
			Network::Server(protocol, Test::Codec(logger), timeout, logger) {}
			~Server() noexcept = default;

		private:
			Expected<void, Network::PacketError> ProcessClientPacket(const std::string& client_uuid, const Network::Packet& packet) noexcept override {
				switch(static_cast<Packets::Opcode>(packet.Opcode())) {
					case Packets::Opcode::C_MSG_ASKNAMELIST: {
						auto ask_packet = static_cast<const Packets::AskNameList&>(packet);
						// Deserialize amount from the AskNameList packet
						auto serialized = ask_packet.DoSerialize();
						auto extracted = serialized.Extract();
						if (!extracted) {
							return StormByte::Unexpected<Network::PacketError>("Server::ProcessClientPacket: failed to extract AskNameList data");
						}
						auto expected_amount = StormByte::Serializable<std::size_t>::Deserialize(extracted.value());
						if (!expected_amount) {
							return StormByte::Unexpected<Network::PacketError>("Server::ProcessClientPacket: failed to deserialize AskNameList amount ({})", expected_amount.error()->what());
						}
						std::size_t amount = *expected_amount;

						// Create dummy name list
						std::vector<std::string> names;
						for (std::size_t i = 0; i < amount; ++i) {
							names.push_back("Name_" + std::to_string(i + 1));
						}
						Packets::AnswerNameList answer_packet(names);
						auto send_expected = Send(client_uuid, answer_packet);
						if (!send_expected) {
							return StormByte::Unexpected<Network::PacketError>("Server::ProcessClientPacket: failed to send AnswerNameList packet ({})", send_expected.error()->what());
						}
						break;
					}
					case Packets::Opcode::C_MSG_ASKRANDOMNUMBER: {
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
						Packets::AnswerRandomNumber answer_packet(random_number);
						auto send_expected = Send(client_uuid, answer_packet);
						if (!send_expected) {
							return StormByte::Unexpected<Network::PacketError>("Server::ProcessClientPacket: failed to send AnswerRandomNumber packet ({})", send_expected.error()->what());
						}
						break;
					}
					default:
						return StormByte::Unexpected<Network::PacketError>("Server::ProcessClientPacket: received unknown packet opcode ({})", packet.Opcode());
				}
				return {};
			}
	};
}

Logger::ThreadedLog logger(std::cout, Logger::Level::LowLevel, "[%L] [T%i] %T:");
constexpr const unsigned short timeout = 5;

int TestRequestNameList() {
	const std::string fn_name = "TestRequestNameList";

	const char* HOST = "127.0.0.1";
	const unsigned short PORT = 7081;

	Test::Server server(Protocol::IPv4, timeout, logger);
	auto listen_res = server.Connect(HOST, PORT);
	if (!listen_res) {
		logger << Logger::Level::Error << fn_name << ": server.Connect failed: " << listen_res.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	// Small delay to ensure server is listening
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(Protocol::IPv4, timeout, logger);
	auto connect_res = client.Connect(HOST, PORT);
	if (!connect_res) {
		logger << Logger::Level::Error << fn_name << ": client.Connect failed: " << connect_res.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}

	const std::size_t amount = 3;
	auto names_expected = client.RequestNameList(amount);
	if (!names_expected) {
		logger << Logger::Level::Error << fn_name << ": RequestNameList failed: " << names_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	auto names = names_expected.value();
	std::string all_names;
	ASSERT_TRUE(fn_name, names.size() == amount);
	for (std::size_t i = 0; i < amount; ++i) {
		all_names += names[i] + " ";
		ASSERT_TRUE(fn_name, names[i] == ("Name_" + std::to_string(i + 1)));
	}
	logger << Logger::Level::Info << fn_name << ": Received names: " << all_names << std::endl;

	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int TestRequestRandomNumber() {
	const std::string fn_name = "TestRequestRandomNumber";

	const char* HOST = "127.0.0.1";
	const unsigned short PORT = 7080;

	Test::Server server(Protocol::IPv4, timeout, logger);
	auto listen_res = server.Connect(HOST, PORT);
	ASSERT_TRUE(fn_name, listen_res.has_value());

	// Small delay to ensure server is listening
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Test::Client client(Protocol::IPv4, timeout, logger);
	auto connect_res = client.Connect(HOST, PORT);
	ASSERT_TRUE(fn_name, connect_res.has_value());

	auto number_expected = client.RequestRandomNumber();
	if (!number_expected) {
		logger << Logger::Level::Error << fn_name << ": RequestRandomNumber failed: " << number_expected.error()->what() << std::endl;
		RETURN_TEST(fn_name, 1);
	}
	int n = number_expected.value();
	ASSERT_TRUE(fn_name, n >= 0 && n < 100);
	logger << Logger::Level::Info << fn_name << ": Received random number: " << n << std::endl;

	client.Disconnect();
	server.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;
	result += TestRequestNameList();
	result += TestRequestRandomNumber();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
