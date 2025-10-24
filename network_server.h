#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "trading_engine.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class NetworkServer {
private:
    TradingEngine* engine;
    int server_socket;
    int port;
    std::atomic<bool> running;
    
    std::vector<int> client_sockets;
    std::mutex clients_mutex;
    
    // Thread functions
    void acceptClients();
    void handleClient(int client_socket);
    
    // Protocol functions
    std::string processCommand(const std::string& command);
    
public:
    NetworkServer(TradingEngine* eng, int p);
    ~NetworkServer();
    
    void start();
    void stop();
    void broadcastMessage(const std::string& message);
};

#endif // NETWORK_SERVER_H