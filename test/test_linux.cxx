#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    const int port = 8080;  // Replace with your desired port
    const char* ip = "127.0.0.1";  // Replace with your desired IP address

    // Step 1: Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Step 2: Set socket options (optional but recommended)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket options\n";
        close(server_fd);
        return 1;
    }

    // Step 3: Bind the socket to an IP and port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        std::cerr << "Failed to bind socket\n";
        close(server_fd);
        return 1;
    }

    // Step 4: Start listening for connections
    if (listen(server_fd, 10) == -1) {  // 10 is the backlog size
        std::cerr << "Failed to listen on socket\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Listening on " << ip << ":" << port << "...\n";

    // Step 5: Accept connections (optional)
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd == -1) {
        std::cerr << "Failed to accept connection\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Connection accepted\n";

    // Cleanup
    close(client_fd);
    close(server_fd);

    return 0;
}
