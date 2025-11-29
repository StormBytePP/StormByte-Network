// Combined multi-client server/client test
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

// Configuration
constexpr const char* HOST = "127.0.0.1";
constexpr unsigned short PORT = 7070;
constexpr std::size_t NUM_CLIENTS = 4;                    // number of concurrent clients
constexpr std::size_t CHUNK_SIZE = 65536;                 // chunk size for each send
constexpr std::size_t TOTAL_BYTES_PER_CLIENT = 104857600;   // total bytes each client will send (1 MiB)

int main() {
    auto packet_fn = [](const unsigned short&) -> std::shared_ptr<Network::Packet> { return nullptr; };
    auto handler = std::make_shared<Connection::Handler>(packet_fn);
    auto logger = std::make_shared<Logger>(std::cout, Logger::Level::Info);

    std::atomic<bool> stop{false};
    std::atomic<std::size_t> clients_finished{0};

    // Server thread
    std::thread server_thread([&]() {
        Socket::Server server(Connection::Protocol::IPv4, handler, logger);
        auto listen_res = server.Listen(HOST, PORT);
        if (!listen_res) {
            std::cerr << "Server Listen failed: " << listen_res.error()->what() << std::endl;
            return;
        }

        std::vector<std::thread> client_handlers;

        while (!stop.load()) {
            auto expected_client = server.Accept();
            if (!expected_client) {
                // Accept may time out; check stop flag and continue
                continue;
            }

            Socket::Client client_socket = std::move(expected_client.value());

            // Handler thread per client
            client_handlers.emplace_back([client = std::move(client_socket), &clients_finished]() mutable {
                auto expected_buffer = client.Receive();
                if (!expected_buffer) {
                    std::cerr << "Client handler: Receive failed: " << expected_buffer.error()->what() << std::endl;
                    client.Disconnect();
                    clients_finished.fetch_add(1);
                    return;
                }

                Buffer::FIFO buffer = std::move(expected_buffer.value());
                auto expected_data = buffer.Extract();
                if (!expected_data) {
                    std::cerr << "Client handler: Extract failed" << std::endl;
                    client.Disconnect();
                    clients_finished.fetch_add(1);
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
                clients_finished.fetch_add(1);
            });
        }

        // Join handler threads
        for (auto &t : client_handlers) if (t.joinable()) t.join();

        if (Connection::IsConnected(server.Status())) {
            server.Disconnect();
        }
    });

    // Small delay to ensure server is listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client threads
    std::vector<std::thread> clients;
    for (std::size_t i = 0; i < NUM_CLIENTS; ++i) {
        clients.emplace_back([i, &clients_finished, handler, logger]() {
            Socket::Client client(Connection::Protocol::IPv4, handler, logger);
            auto connect_res = client.Connect(HOST, PORT);
            if (!connect_res) {
                std::cerr << "Client " << i << " Connect failed: " << connect_res.error()->what() << std::endl;
                return;
            }

            // Prepare payload
            std::vector<std::byte> chunk(CHUNK_SIZE, std::byte('H'));
            std::size_t sent = 0;
            while (sent < TOTAL_BYTES_PER_CLIENT) {
                std::size_t to_send = std::min(CHUNK_SIZE, TOTAL_BYTES_PER_CLIENT - sent);
                auto res = client.Send(std::span<const std::byte>(chunk.data(), to_send));
                if (!res) {
                    std::cerr << "Client " << i << " Send failed: " << res.error()->what() << std::endl;
                    client.Disconnect();
                    return;
                }
                sent += to_send;
            }

            std::cout << "Client " << i << ": sent " << sent << " bytes" << std::endl;

            // Receive echoed data
            auto recv_buf_res = client.Receive(TOTAL_BYTES_PER_CLIENT);
            if (!recv_buf_res) {
                std::cerr << "Client " << i << " Receive failed: " << recv_buf_res.error()->what() << std::endl;
                client.Disconnect();
                return;
            }

            Buffer::FIFO buffer = std::move(recv_buf_res.value());
            auto expected_data = buffer.Extract();
            if (!expected_data) {
                std::cerr << "Client " << i << " Extract failed" << std::endl;
                client.Disconnect();
                return;
            }

            auto vec = expected_data.value();
            std::cout << "Client " << i << ": received " << vec.size() << " bytes" << std::endl;

            bool ok = (vec.size() == TOTAL_BYTES_PER_CLIENT);
            if (ok) {
                for (size_t k = 0; k < vec.size(); ++k) {
                    if (vec[k] != std::byte('H')) { ok = false; break; }
                }
            }

            if (ok) std::cout << "Client " << i << ": echo validation passed" << std::endl;
            else std::cerr << "Client " << i << ": echo validation failed" << std::endl;

            client.Disconnect();
        });
    }

    // Wait for all clients to finish
    for (auto &t : clients) if (t.joinable()) t.join();

    // Signal server to stop accepting (it will finish handling existing clients)
    stop.store(true);

    // Wait until server observed all clients finished
    while (clients_finished.load() < NUM_CLIENTS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (server_thread.joinable()) server_thread.join();

    std::cout << "All clients finished, server stopped." << std::endl;
    return 0;
}
