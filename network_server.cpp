#include "network_server.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

NetworkServer::NetworkServer(TradingEngine* eng, int p) 
    : engine(eng), server_socket(-1), port(p), running(false) {
}

NetworkServer::~NetworkServer() {
    stop();
}

void NetworkServer::start() {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Allow reuse of address
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        close(server_socket);
        return;
    }
    
    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return;
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "Trading Server started on port " << port << std::endl;
    std::cout << "Waiting for clients to connect..." << std::endl;
    std::cout << "==================================" << std::endl;
    
    running = true;
    acceptClients();
}

void NetworkServer::acceptClients() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, 
                                   (struct sockaddr*)&client_addr, 
                                   &client_len);
        
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Failed to accept client" << std::endl;
            }
            continue;
        }
        
        std::cout << "\n[SERVER] Client connected from " 
                  << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client_sockets.push_back(client_socket);
        }
        
        // Spawn thread to handle this client
        std::thread client_thread(&NetworkServer::handleClient, this, client_socket);
        client_thread.detach();  // Let it run independently
    }
}

void NetworkServer::handleClient(int client_socket) {
    char buffer[1024];
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            std::cout << "[SERVER] Client disconnected" << std::endl;
            break;
        }
        
        std::string command(buffer);
        std::cout << "[SERVER] Received: " << command;
        
        std::string response = processCommand(command);
        
        // Send response
        send(client_socket, response.c_str(), response.length(), 0);
        
        // Check for disconnect command
        if (command.find("DISCONNECT") == 0) {
            break;
        }
    }
    
    // Clean up
    close(client_socket);
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_sockets.erase(
            std::remove(client_sockets.begin(), client_sockets.end(), client_socket),
            client_sockets.end()
        );
    }
}

std::string NetworkServer::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "ADD_ORDER") {
        std::string side_str, symbol;
        double price;
        int quantity;
        
        if (!(iss >> side_str >> symbol >> price >> quantity)) {
            return "ERROR: Invalid command format\nUsage: ADD_ORDER <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>\n";
        }
        
        OrderSide side;
        if (side_str == "BUY") {
            side = OrderSide::BUY;
        } else if (side_str == "SELL") {
            side = OrderSide::SELL;
        } else {
            return "ERROR: Invalid side. Use BUY or SELL\n";
        }
        
        if (price <= 0 || quantity <= 0) {
            return "ERROR: Price and quantity must be positive\n";
        }
        
        std::string result = engine->addOrder(symbol, side, price, quantity);
        return result;
    }
    else if (cmd == "SHOW_ORDERS") {
        std::string symbol;
        if (!(iss >> symbol)) {
            return "ERROR: Invalid command format\nUsage: SHOW_ORDERS <SYMBOL>\n";
        }
        
        return engine->showOrders(symbol);
    }
    else if (cmd == "DISCONNECT") {
        return "OK: Goodbye!\n";
    }
    else {
        return "ERROR: Unknown command\nAvailable commands: ADD_ORDER, SHOW_ORDERS, DISCONNECT\n";
    }
}

void NetworkServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    for (int socket : client_sockets) {
        send(socket, message.c_str(), message.length(), 0);
    }
}

void NetworkServer::stop() {
    running = false;
    
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    // Close all client connections
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int socket : client_sockets) {
        close(socket);
    }
    client_sockets.clear();
}