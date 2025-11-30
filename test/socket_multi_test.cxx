// Combined multi-client server/client test
#include <StormByte/buffer/fifo.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/test_handlers.h>

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
auto logger = Logger::ThreadedLog(std::cout, Logger::Level::Error, "[%L] [T%i] %T: ");

int TestSocketMulti() {
    const std::string fn_name = "TestSocketMulti";

    auto packet_fn = [](const unsigned short&) -> std::shared_ptr<Network::Packet> { return nullptr; };
    auto handler = std::make_shared<Connection::Handler>(packet_fn);

    std::atomic<bool> stop{false};
    std::atomic<std::size_t> clients_finished{0};

    // Server thread
    std::thread server_thread([&]() -> int {
        Socket::Server server(Connection::Protocol::IPv4, handler, logger);
        auto listen_res = server.Listen(HOST, PORT);
        ASSERT_TRUE(fn_name, listen_res.has_value());

        std::vector<std::thread> client_handlers;

        while (!stop.load()) {
            auto expected_client = server.Accept();
            if (!expected_client) {
                // Accept may time out; check stop flag and continue
                continue;
            }

            Socket::Client client_socket = std::move(expected_client.value());

            // Handler thread per client
            client_handlers.emplace_back([client = std::move(client_socket), &clients_finished]() mutable -> int {
                const std::string fn_name_inner = "TestSocketMulti::Handler";
                auto expected_buffer = client.Receive();
                ASSERT_TRUE(fn_name_inner, expected_buffer.has_value());

                Buffer::FIFO buffer = std::move(expected_buffer.value());
                auto expected_data = buffer.Extract();
                ASSERT_TRUE(fn_name_inner, expected_data.has_value());

                auto vec = expected_data.value();
                ASSERT_FALSE(fn_name_inner, vec.empty());

                // Echo back
                auto send_res = client.Send(std::span<const std::byte>(vec.data(), vec.size()));
                ASSERT_TRUE(fn_name_inner, send_res.has_value());

                client.Disconnect();
                clients_finished.fetch_add(1);
                return 0;
            });
        }

        // Join handler threads
        for (auto &t : client_handlers) if (t.joinable()) t.join();

        if (Connection::IsConnected(server.Status())) {
            server.Disconnect();
        }
        return 0;
    });

    // Small delay to ensure server is listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client threads
    std::vector<std::thread> clients;
    for (std::size_t i = 0; i < NUM_CLIENTS; ++i) {
        clients.emplace_back([i, &clients_finished, handler]() -> int {
            const std::string fn_name_inner = "TestSocketMulti::Client";
            Socket::Client client(Connection::Protocol::IPv4, handler, logger);
            auto connect_res = client.Connect(HOST, PORT);
            ASSERT_TRUE(fn_name_inner, connect_res.has_value());

            // Prepare payload
            std::vector<std::byte> chunk(CHUNK_SIZE, std::byte('H'));
            std::size_t sent = 0;
            while (sent < TOTAL_BYTES_PER_CLIENT) {
                std::size_t to_send = std::min(CHUNK_SIZE, TOTAL_BYTES_PER_CLIENT - sent);
                auto res = client.Send(std::span<const std::byte>(chunk.data(), to_send));
                ASSERT_TRUE(fn_name_inner, res.has_value());
                sent += to_send;
            }

            // Receive echoed data
            auto recv_buf_res = client.Receive(TOTAL_BYTES_PER_CLIENT);
            ASSERT_TRUE(fn_name_inner, recv_buf_res.has_value());

            Buffer::FIFO buffer = std::move(recv_buf_res.value());
            auto expected_data = buffer.Extract();
            ASSERT_TRUE(fn_name_inner, expected_data.has_value());

            auto vec = expected_data.value();
            ASSERT_TRUE(fn_name_inner, vec.size() == TOTAL_BYTES_PER_CLIENT);

            for (size_t k = 0; k < vec.size(); ++k) {
                ASSERT_TRUE(fn_name_inner, vec[k] == std::byte('H'));
            }

            client.Disconnect();
            return 0;
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

    RETURN_TEST(fn_name, 0);
}

int main() {
    int result = 0;
    result += TestSocketMulti();
    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}
