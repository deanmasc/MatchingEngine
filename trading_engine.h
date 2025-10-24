#ifndef TRADING_ENGINE_H
#define TRADING_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <map>

enum class OrderSide {
    BUY,
    SELL
};

struct Order {
    std::string symbol;           
    OrderSide side;              
    double price;             
    int quantity;           
    int order_id;                
    long long timestamp;     
    
    Order(const std::string& sym, OrderSide s, double p, int q, int id);
};

class OrderBook {
private:
    std::string symbol;
    std::vector<std::shared_ptr<Order>> buy_orders;   // All buy orders
    std::vector<std::shared_ptr<Order>> sell_orders;  // All sell orders
    
    mutable std::mutex book_mutex;  // Thread-safe access to this order book
    
    std::string matchOrders();
    
    void sortOrders();
    
public:
    OrderBook(const std::string& sym) : symbol(sym) {}
    
    std::string addOrder(std::shared_ptr<Order> order);
    
    std::string displayOrders() const;
};

class TradingEngine {
private:
    std::map<std::string, OrderBook> order_books;
    int next_order_id;
    std::mutex engine_mutex;  // Protects order_books vector
    
    OrderBook* findOrderBook(const std::string& symbol);
    
public:
    TradingEngine() : next_order_id(1) {}
    
    std::string addOrder(const std::string& symbol, OrderSide side, double price, int quantity);
    
    std::string showOrders(const std::string& symbol);
    
    void start();
};

#endif // TRADING_ENGINE_H