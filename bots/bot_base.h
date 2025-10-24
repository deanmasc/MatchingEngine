#ifndef BOT_BASE_H
#define BOT_BASE_H

#include <string>
#include <atomic>
#include <thread>
#include <chrono>

class TradingBot {
protected:
    int socket_fd;
    std::string server_ip;
    int server_port;
    std::string bot_name;
    
    std::atomic<bool> running;
    
    bool connectToServer();
    void disconnectFromServer();
    
    std::string sendCommand(const std::string& command);
    bool sendCommandNoResponse(const std::string& command);
    
    virtual void executeStrategy() = 0;
    
    void sleep(int milliseconds);
    void logMessage(const std::string& message);
    
public:
    TradingBot(const std::string& name, const std::string& ip, int port);
    virtual ~TradingBot();
    
    void run();
    void stop();
};

#endif // BOT_BASE_H