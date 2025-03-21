#include <StormByte/network/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/test_handlers.h>

using namespace StormByte::Network;

int main() {
	std::shared_ptr<Connection::Handler> handler = std::make_shared<Connection::Handler>();
	Server server(handler);
	auto result = server.Connect("localhost", 9090, Connection::Protocol::IPv4);
	if (result) {
		std::cout << "Server connected successfully" << std::endl;
	}
	else {
		std::cout << "Server " << result.error()->what() << std::endl;
	}
	Client client(handler);
	auto client_result = client.Connect("localhost", 9090, Connection::Protocol::IPv4);
	if (client_result) {
		std::cout << "Client connected successfully" << std::endl;
	}
	else
		std::cout << "Client " << client_result.error()->what() << std::endl;
	//client.Disconnect();
	//server.Disconnect();
	return 0;
}