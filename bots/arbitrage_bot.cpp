#include "bot_base.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>

class ArbitrageBot : public TradingBot {
private:
    std::string symbol;
    double target_buy_price;
    double target_sell_price;
    int position;
    int trade_size;
    double total_profit;
    
    struct OrderBookSnapshot {
        double best_bid;
        double best_ask;
        bool valid;
    };
    
    OrderBookSnapshot getOrderBook() {
        std::string response = sendCommand("SHOW_ORDERS " + symbol);
        
        OrderBookSnapshot snapshot = {0.0, 0.0, false};
        
        std::regex price_pattern(R"((\d+)\s+@\s+\$(\d+\.\d+))");
        std::smatch matches;
        
        bool in_buy_section = false;
        bool in_sell_section = false;
        
        std::istringstream iss(response);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.find("BUY ORDERS") != std::string::npos) {
                in_buy_section = true;
                in_sell_section = false;
                continue;
            }
            if (line.find("SELL ORDERS") != std::string::npos) {
                in_sell_section = true;
                in_buy_section = false;
                continue;
            }
            
            if (std::regex_search(line, matches, price_pattern)) {
                double price = std::stod(matches[2].str());
                
                if (in_buy_section && snapshot.best_bid == 0.0) {
                    snapshot.best_bid = price;
                }
                if (in_sell_section && snapshot.best_ask == 0.0) {
                    snapshot.best_ask = price;
                }
            }
        }
        
        snapshot.valid = (snapshot.best_bid > 0.0 || snapshot.best_ask > 0.0);
        return snapshot;
    }
    
    void executeTrade(const std::string& side, double price) {
        std::stringstream cmd;
        cmd << "ADD_ORDER " << side << " " << symbol << " " 
            << std::fixed << std::setprecision(2) << price 
            << " " << trade_size;
        
        std::string response = sendCommand(cmd.str());
        
        if (side == "BUY") {
            position += trade_size;
            logMessage("✓ BOUGHT " + std::to_string(trade_size) + " @ $" + 
                      std::to_string(price) + " (Position: " + std::to_string(position) + ")");
        } else {
            position -= trade_size;
            logMessage("✓ SOLD " + std::to_string(trade_size) + " @ $" + 
                      std::to_string(price) + " (Position: " + std::to_string(position) + ")");
        }
    }
    
protected:
    void executeStrategy() override {
        OrderBookSnapshot book = getOrderBook();
        
        if (!book.valid) {
            logMessage("Waiting for orders to appear in book...");
            sleep(2000);
            return;
        }
        
        if (book.best_bid > 0.0 && book.best_ask > 0.0) {
            double spread = book.best_ask - book.best_bid;
            logMessage("Market: BID $" + std::to_string(book.best_bid) + 
                      " | ASK $" + std::to_string(book.best_ask) + 
                      " | Spread $" + std::to_string(spread));
        }
                
        // Buy opportunity: Ask price is below target
        if (book.best_ask > 0.0 && book.best_ask < target_buy_price && position <= 0) {
            logMessage("BUY OPPORTUNITY: Price $" + std::to_string(book.best_ask) + 
                      " < Target $" + std::to_string(target_buy_price));
            executeTrade("BUY", book.best_ask);
        }
        
        // Sell opportunity: Bid price is above target
        if (book.best_bid > 0.0 && book.best_bid > target_sell_price && position > 0) {
            logMessage("SELL OPPORTUNITY: Price $" + std::to_string(book.best_bid) + 
                      " > Target $" + std::to_string(target_sell_price));
            
            double profit = (book.best_bid - target_buy_price) * trade_size;
            total_profit += profit;
            
            executeTrade("SELL", book.best_bid);
            logMessage("Profit on this trade: $" + std::to_string(profit) + 
                      " | Total profit: $" + std::to_string(total_profit));
        }
        
        sleep(500);
    }
    
public:
    ArbitrageBot(const std::string& ip, int port, const std::string& sym,
                 double buy_target, double sl_target, int size = 50)
        : TradingBot("Arbitrage", ip, port),
          symbol(sym), target_buy_price(buy_target), target_sell_price(sell_target),
          position(0), trade_size(size), total_profit(0.0) {
    }
};

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port> <symbol> <buy_target> <sell_target>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8080 AAPL 149.00 151.00" << std::endl;
        std::cout << "  Buy when price < $149.00, sell when price > $151.00" << std::endl;
        return 1;
    }
    
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string symbol = argv[3];
    double buy_target = std::stod(argv[4]);
    double sell_target = std::stod(argv[5]);
    
    ArbitrageBot bot(ip, port, symbol, buy_target, sell_target);
    bot.run();
    
    return 0;
}