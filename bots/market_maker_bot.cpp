#include "bot_base.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

class MarketMakerBot : public TradingBot {
private:
    std::string symbol;
    double spread;
    int order_size;
    double base_price;
    
    void placeOrders() {
        double buy_price = base_price - spread;
        double sell_price = base_price + spread;
        
        buy_price = std::round(buy_price * 100.0) / 100.0;
        sell_price = std::round(sell_price * 100.0) / 100.0;
        
        // Place buy order
        std::stringstream buy_cmd;
        buy_cmd << "ADD_ORDER BUY " << symbol << " " 
                << std::fixed << std::setprecision(2) << buy_price 
                << " " << order_size;
        
        std::string buy_response = sendCommand(buy_cmd.str());
        
        // Place sell order
        std::stringstream sell_cmd;
        sell_cmd << "ADD_ORDER SELL " << symbol << " " 
                 << std::fixed << std::setprecision(2) << sell_price 
                 << " " << order_size;
        
        std::string sell_response = sendCommand(sell_cmd.str());
        
        logMessage("Placed orders: BUY @ $" + std::to_string(buy_price) + 
                   " | SELL @ $" + std::to_string(sell_price));
    }
    
protected:
    void executeStrategy() override {
        placeOrders();
        
        base_price += (std::rand() % 3 - 1) * 0.25;
        
        sleep(2000);
    }
    
public:
    MarketMakerBot(const std::string& ip, int port, const std::string& sym, 
                   double base, double sprd = 0.50, int size = 50)
        : TradingBot("MarketMaker", ip, port), 
          symbol(sym), spread(sprd), order_size(size), base_price(base) {
        std::srand(std::time(nullptr));
    }
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port> <symbol> <base_price>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8080 AAPL 150.00" << std::endl;
        return 1;
    }
    
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string symbol = argv[3];
    double base_price = std::stod(argv[4]);
    
    MarketMakerBot bot(ip, port, symbol, base_price);
    bot.run();
    
    return 0;
}