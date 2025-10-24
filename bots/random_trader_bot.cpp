#include "bot_base.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>

class RandomTraderBot : public TradingBot {
private:
    std::string symbol;
    double min_price;
    double max_price;
    int min_quantity;
    int max_quantity;
    
    std::mt19937 rng;
    std::uniform_real_distribution<double> price_dist;
    std::uniform_int_distribution<int> quantity_dist;
    std::uniform_int_distribution<int> side_dist;
    std::uniform_int_distribution<int> wait_dist;
    
    std::string randomSide() {
        return (side_dist(rng) == 0) ? "BUY" : "SELL";
    }
    
    double randomPrice() {
        double price = price_dist(rng);
        return std::round(price * 100.0) / 100.0;
    }
    
    int randomQuantity() {
        return quantity_dist(rng);
    }
    
    int randomWaitTime() {
        return wait_dist(rng) * 1000;
    }
    
    void placeRandomOrder() {
        std::string side = randomSide();
        double price = randomPrice();
        int quantity = randomQuantity();
        
        std::stringstream cmd;
        cmd << "ADD_ORDER " << side << " " << symbol << " " 
            << std::fixed << std::setprecision(2) << price 
            << " " << quantity;
        
        std::string response = sendCommand(cmd.str());
        
        logMessage(side + " " + std::to_string(quantity) + " @ $" + 
                   std::to_string(price));
        
        // Check if trade was executed
        if (response.find("TRADE EXECUTED") != std::string::npos) {
            logMessage("✓ Trade matched!");
        }
    }
    
protected:
    void executeStrategy() override {
        placeRandomOrder();
        
        // Random wait time between trades
        int wait_ms = randomWaitTime();
        sleep(wait_ms);
    }
    
public:
    RandomTraderBot(const std::string& ip, int port, const std::string& sym,
                    double min_p, double max_p, int min_q = 10, int max_q = 100)
        : TradingBot("RandomTrader", ip, port),
          symbol(sym), min_price(min_p), max_price(max_p),
          min_quantity(min_q), max_quantity(max_q),
          rng(std::random_device{}()),
          price_dist(min_price, max_price),
          quantity_dist(min_quantity, max_quantity),
          side_dist(0, 1),
          wait_dist(1, 5) {
    }
};

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port> <symbol> <min_price> <max_price>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8080 AAPL 145.00 155.00" << std::endl;
        return 1;
    }
    
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string symbol = argv[3];
    double min_price = std::stod(argv[4]);
    double max_price = std::stod(argv[5]);
    
    RandomTraderBot bot(ip, port, symbol, min_price, max_price);
    bot.run();
    
    return 0;
}