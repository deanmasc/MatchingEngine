#include "trading_engine.h"
#include "network_server.h"
#include <iostream>

int main() {
    TradingEngine engine;
    NetworkServer server(&engine, 8080);
    
    std::cout << "Starting networked trading server...\n" << std::endl;
    server.start();
    
    return 0;
}