#include "trading_engine.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// Order Implementation

Order::Order(const std::string& sym, OrderSide s, double p, int q, int id)
    : symbol(sym), side(s), price(p), quantity(q), order_id(id) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

// OrderBook Implementation

void OrderBook::sortOrders() {
    std::sort(buy_orders.begin(), buy_orders.end(),
        [](const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) {
            if (a->price != b->price) {
                return a->price > b->price;
            }
            return a->timestamp < b->timestamp;
        });
    
    std::sort(sell_orders.begin(), sell_orders.end(),
        [](const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) {
            if (a->price != b->price) {
                return a->price < b->price;
            }
            return a->timestamp < b->timestamp;
        });
}

std::string OrderBook::matchOrders() {
    std::stringstream result;
    
    sortOrders();
    
    while (!buy_orders.empty() && !sell_orders.empty()) {
        auto& best_buy = buy_orders[0];
        auto& best_sell = sell_orders[0];
        
        if (best_buy->price >= best_sell->price) {
            double execution_price;
            if (best_buy->timestamp < best_sell->timestamp) {
                execution_price = best_buy->price;
            } else {
                execution_price = best_sell->price;
            }
            
            int trade_quantity = std::min(best_buy->quantity, best_sell->quantity);
            
            result << "TRADE EXECUTED: " << trade_quantity << " " << symbol 
                   << " @ $" << std::fixed << std::setprecision(2) 
                   << execution_price << "\n";
            
            best_buy->quantity -= trade_quantity;
            best_sell->quantity -= trade_quantity;
            
            if (best_buy->quantity == 0) {
                buy_orders.erase(buy_orders.begin());
            }
            if (best_sell->quantity == 0) {
                sell_orders.erase(sell_orders.begin());
            }
        } else {
            break;
        }
    }
    
    return result.str();
}

std::string OrderBook::addOrder(std::shared_ptr<Order> order) {
    std::lock_guard<std::mutex> lock(book_mutex);
    
    std::stringstream msg;
    
    if (order->side == OrderSide::BUY) {
        buy_orders.push_back(order);
        msg << "Order added: BUY " << order->quantity << " " 
            << order->symbol << " @ $" << std::fixed << std::setprecision(2) 
            << order->price << " (Order ID: " << order->order_id << ")\n";
    } else {
        sell_orders.push_back(order);
        msg << "Order added: SELL " << order->quantity << " " 
            << order->symbol << " @ $" << std::fixed << std::setprecision(2) 
            << order->price << " (Order ID: " << order->order_id << ")\n";
    }
    
    std::string match_result = matchOrders();
    
    return msg.str() + match_result;
}

std::string OrderBook::displayOrders() const {
    std::lock_guard<std::mutex> lock(book_mutex);
    
    std::stringstream output;
    
    output << "\n=== " << symbol << " Order Book ===\n";
    
    output << "\nBUY ORDERS:\n";
    if (buy_orders.empty()) {
        output << "  No buy orders\n";
    } else {
        for (const auto& order : buy_orders) {
            output << "  Order #" << order->order_id << ": "
                   << order->quantity << " @ $" << std::fixed << std::setprecision(2)
                   << order->price << "\n";
        }
    }
    
    output << "\nSELL ORDERS:\n";
    if (sell_orders.empty()) {
        output << "  No sell orders\n";
    } else {
        for (const auto& order : sell_orders) {
            output << "  Order #" << order->order_id << ": "
                   << order->quantity << " @ $" << std::fixed << std::setprecision(2)
                   << order->price << "\n";
        }
    }
    
    output << "\n";
    
    return output.str();
}

// TradingEngine Implementation

OrderBook* TradingEngine::findOrderBook(const std::string& symbol) {
    auto it = order_books.find(symbol);
    if (it != order_books.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string TradingEngine::addOrder(const std::string& symbol, OrderSide side, double price, int quantity) {
    auto order = std::make_shared<Order>(symbol, side, price, quantity, next_order_id++);
    
    OrderBook* book = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(engine_mutex);
        
        // try_emplace constructs in-place if symbol doesn't exist
        auto [it, inserted] = order_books.try_emplace(symbol, symbol);
        book = &it->second;
    }
    
    return book->addOrder(order);
}

std::string TradingEngine::showOrders(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    OrderBook* book = findOrderBook(symbol);
    
    if (book != nullptr) {
        return book->displayOrders();
    } else {
        return "No orders found for symbol: " + symbol + "\n";
    }
}

void TradingEngine::start() {
    std::cout << "Trading Engine Started..." << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  add_order <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
    std::cout << "  show_orders <SYMBOL>" << std::endl;
    std::cout << "  exit" << std::endl;
    std::cout << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "exit" || command == "quit") {
            std::cout << "Trading Engine Stopped." << std::endl;
            break;
        }
        else if (command == "add_order") {
            std::string side_str, symbol;
            double price;
            int quantity;
            
            if (!(iss >> side_str >> symbol >> price >> quantity)) {
                std::cout << "Invalid command format. Use: add_order <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
                continue;
            }
            
            OrderSide side;
            if (side_str == "BUY" || side_str == "buy") {
                side = OrderSide::BUY;
            } else if (side_str == "SELL" || side_str == "sell") {
                side = OrderSide::SELL;
            } else {
                std::cout << "Invalid side. Use BUY or SELL." << std::endl;
                continue;
            }
            
            if (price <= 0 || quantity <= 0) {
                std::cout << "Price and quantity must be positive." << std::endl;
                continue;
            }
            
            std::string result = addOrder(symbol, side, price, quantity);
            std::cout << result;
        }
        else if (command == "show_orders") {
            std::string symbol;
            if (!(iss >> symbol)) {
                std::cout << "Invalid command format. Use: show_orders <SYMBOL>" << std::endl;
                continue;
            }
            
            std::string result = showOrders(symbol);
            std::cout << result;
        }
        else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Available commands: add_order, show_orders, exit" << std::endl;
        }
    }
}