#include "bot_base.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

TradingBot::TradingBot(const std::string& name, const std::string& ip, int port)
    : socket_fd(-1), server_ip(ip), server_port(port), bot_name(name), running(false) {
}

TradingBot::~TradingBot() {
    stop();
}

bool TradingBot::connectToServer() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "[" << bot_name << "] Failed to create socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[" << bot_name << "] Failed to connect to server at " 
                  << server_ip << ":" << server_port << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    logMessage("Connected to trading server");
    return true;
}

void TradingBot::disconnectFromServer() {
    if (socket_fd >= 0) {
        sendCommandNoResponse("DISCONNECT");
        close(socket_fd);
        socket_fd = -1;
        logMessage("Disconnected from server");
    }
}

std::string TradingBot::sendCommand(const std::string& command) {
    if (socket_fd < 0) {
        return "ERROR: Not connected";
    }
    
    // Send command
    std::string cmd = command + "\n";
    ssize_t sent = send(socket_fd, cmd.c_str(), cmd.length(), 0);
    if (sent < 0) {
        return "ERROR: Send failed";
    }
    
    // Receive response
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (received <= 0) {
        return "ERROR: Receive failed";
    }
    
    return std::string(buffer);
}

bool TradingBot::sendCommandNoResponse(const std::string& command) {
    if (socket_fd < 0) {
        return false;
    }
    
    std::string cmd = command + "\n";
    ssize_t sent = send(socket_fd, cmd.c_str(), cmd.length(), 0);
    return sent > 0;
}

void TradingBot::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void TradingBot::logMessage(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", std::localtime(&time));
    
    std::cout << "[" << time_str << "." << ms.count() << "] "
              << "[" << bot_name << "] " << message << std::endl;
}

void TradingBot::run() {
    logMessage("Starting bot...");
    
    if (!connectToServer()) {
        logMessage("Failed to start - connection error");
        return;
    }
    
    running = true;
    logMessage("Bot running. Press Ctrl+C to stop.");
    
    while (running) {
        try {
            executeStrategy();
        } catch (const std::exception& e) {
            logMessage(std::string("Exception in strategy: ") + e.what());
            sleep(1000);
        }
    }
    
    disconnectFromServer();
    logMessage("Bot stopped");
}

void TradingBot::stop() {
    running = false;
}