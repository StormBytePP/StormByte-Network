#ifdef WINDOWS
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // Link with the Winsock library

int main() {
    const int port = 8080;  // Replace with your desired port
    const char* ip = "127.0.0.1";  // Replace with your desired IP address

    // Step 1: Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Step 2: Create a socket
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket, error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Step 3: Set socket options (optional but recommended)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "Failed to set socket options, error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Step 4: Bind the socket to an IP and port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);  // Convert string IP to binary form
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket, error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Step 5: Start listening for connections
    if (listen(server_fd, 10) == SOCKET_ERROR) {  // 10 is the backlog size
        std::cerr << "Failed to listen on socket, error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Listening on " << ip << ":" << port << "...\n";

    // Step 6: Accept connections (optional)
    sockaddr_in client_addr{};
    int client_len = sizeof(client_addr);
    SOCKET client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "Failed to accept connection, error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Connection accepted\n";

    // Cleanup
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
#endif