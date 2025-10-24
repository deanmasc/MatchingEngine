#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8080" << std::endl;
        return 1;
    }
    
    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server at " << server_ip << ":" << port << std::endl;
        std::cerr << "Make sure the server is running!" << std::endl;
        close(sock);
        return 1;
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "Connected to trading server!" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  ADD_ORDER <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
    std::cout << "  SHOW_ORDERS <SYMBOL>" << std::endl;
    std::cout << "  DISCONNECT" << std::endl;
    std::cout << std::endl;
    
    // Command loop
    std::string line;
    char buffer[4096];
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        // Send command
        line += "\n";
        send(sock, line.c_str(), line.length(), 0);
        
        // Receive response
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            std::cout << "Server disconnected" << std::endl;
            break;
        }
        
        std::cout << buffer;
        
        if (line.find("DISCONNECT") == 0) {
            break;
        }
    }
    
    close(sock);
    std::cout << "\nDisconnected from server." << std::endl;
    return 0;
}