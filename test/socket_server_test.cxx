// Server test using the project's Network::Socket API
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/buffer/fifo.hxx>
#include <StormByte/logger.hxx>
#include <StormByte/network/packet.hxx>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>

using namespace StormByte;
using namespace StormByte::Network;

static std::atomic<Socket::Server*> g_server_ptr{nullptr};

void sigint_handler(int) {
    auto ptr = g_server_ptr.load();
    if (ptr) {
        ptr->Disconnect();
    }
}

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    unsigned short port = 7070;
    std::size_t expected_size = 409600000;

    if (argc > 1) port = static_cast<unsigned short>(std::atoi(argv[1]));
    if (argc > 2) expected_size = static_cast<std::size_t>(std::stoul(argv[2]));

    std::signal(SIGINT, sigint_handler);

    // Dummy packet factory (not used in raw tests)
    auto packet_fn = [](const unsigned short&) -> std::shared_ptr<Network::Packet> { return nullptr; };
    auto handler = std::make_shared<Connection::Handler>(packet_fn);
    auto logger = std::make_shared<Logger>(std::cout, Logger::Level::LowLevel);

    Socket::Server server(Connection::Protocol::IPv4, handler, logger);
    g_server_ptr.store(&server);

    auto listen_res = server.Listen(host, port);
    if (!listen_res) {
        std::cerr << "Server Listen failed: " << listen_res.error()->what() << std::endl;
        return 1;
    }

    std::cout << "Server listening on " << host << ":" << port << " expecting " << expected_size << " bytes" << std::endl;

    // Accept loop: remain active until SIGINT triggers server.Disconnect()
    std::vector<std::thread> client_threads;
    while (Connection::IsConnected(server.Status())) {
        auto expected_client = server.Accept();
        if (!expected_client) {
            // Accept may time out periodically; if server still connected, continue waiting
            std::cerr << "Accept error (continuing): " << expected_client.error()->what() << std::endl;
            continue;
        }

        Socket::Client client_socket = std::move(expected_client.value());

        // Spawn a thread to handle this client concurrently
        client_threads.emplace_back([client = std::move(client_socket)]() mutable {
            // Read until the peer closes the connection (max_size == 0)
            auto expected_buffer = client.Receive();
            if (!expected_buffer) {
                std::cerr << "Client handler: Receive failed: " << expected_buffer.error()->what() << std::endl;
                client.Disconnect();
                return;
            }

            Buffer::FIFO buffer = std::move(expected_buffer.value());
            auto expected_data = buffer.Extract();
            if (!expected_data) {
                std::cerr << "Client handler: Extract failed" << std::endl;
                client.Disconnect();
                return;
            }

            auto vec = expected_data.value();
            if (vec.empty()) {
                std::cerr << "Client handler: No data received" << std::endl;
            }

            // Echo back
            if (!client.Send(std::span<const std::byte>(vec.data(), vec.size())).has_value()) {
                std::cerr << "Client handler: Echo send failed" << std::endl;
            } else {
                std::cout << "Server: echoed " << vec.size() << " bytes" << std::endl;
            }

            client.Disconnect();
        });
    }

    // Join handler threads before exiting
    for (auto &t : client_threads) {
        if (t.joinable()) t.join();
    }

    // If we exited the accept loop due to disconnect request, ensure cleanup
    if (Connection::IsConnected(server.Status())) {
        server.Disconnect();
    }

    std::cout << "Server exiting gracefully" << std::endl;
    return 0;
}
