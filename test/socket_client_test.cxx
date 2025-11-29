// Client test using the project's Network::Socket API
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/logger.hxx>
#include <StormByte/buffer/fifo.hxx>
#include <StormByte/network/packet.hxx>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

using namespace StormByte;
using namespace StormByte::Network;

static std::atomic<Socket::Client*> g_client_ptr{nullptr};

void sigint_handler(int) {
    auto ptr = g_client_ptr.load();
    if (ptr) ptr->Disconnect();
}

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    unsigned short port = 7070;
    std::size_t send_size = 409600000;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<unsigned short>(std::atoi(argv[2]));
    if (argc > 3) send_size = static_cast<std::size_t>(std::stoul(argv[3]));

    std::signal(SIGINT, sigint_handler);

    auto packet_fn = [](const unsigned short&) -> std::shared_ptr<Network::Packet> { return nullptr; };
    auto handler = std::make_shared<Connection::Handler>(packet_fn);
    auto logger = std::make_shared<Logger>(std::cout, Logger::Level::LowLevel);

    Socket::Client client(Connection::Protocol::IPv4, handler, logger);
    g_client_ptr.store(&client);

    auto connect_res = client.Connect(host, port);
    if (!connect_res) {
        std::cerr << "Client Connect failed: " << connect_res.error()->what() << std::endl;
        return 1;
    }

    std::vector<std::byte> payload(send_size, std::byte('H'));

    auto send_res = client.Send(std::span<const std::byte>(payload.data(), payload.size()));
    if (!send_res) {
        std::cerr << "Client Send failed: " << send_res.error()->what() << std::endl;
        client.Disconnect();
        return 2;
    }

    std::cout << "Client: sent " << payload.size() << " bytes" << std::endl;

    auto recv_buf_res = client.Receive(send_size);
    if (!recv_buf_res) {
        std::cerr << "Client Receive failed: " << recv_buf_res.error()->what() << std::endl;
        client.Disconnect();
        return 3;
    }

    Buffer::FIFO buffer = std::move(recv_buf_res.value());
    auto expected_data = buffer.Extract();
    if (!expected_data) {
        std::cerr << "Client Extract failed" << std::endl;
        client.Disconnect();
        return 4;
    }

    auto vec = expected_data.value();
    std::cout << "Client: received " << vec.size() << " bytes" << std::endl;

    bool ok = (vec.size() == payload.size());
    if (ok) {
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] != payload[i]) { ok = false; break; }
        }
    }

    if (ok) std::cout << "Client: echo validation passed" << std::endl;
    else std::cerr << "Client: echo validation failed" << std::endl;

    client.Disconnect();
    return ok ? 0 : 5;
}
