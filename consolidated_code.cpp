// main.cpp
#include "trading_engine.h"

int main()
{
    TradingEngine engine;
    engine.start();
    return 0;
}

// trading_engine.cpp

#include "trading_engine.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// Order Implementation

Order::Order(const std::string &sym, OrderSide s, double p, int q, int id)
    : symbol(sym), side(s), price(p), quantity(q), order_id(id)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

// OrderBook Implementation

void OrderBook::sortOrders()
{
    std::sort(buy_orders.begin(), buy_orders.end(),
              [](const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b)
              {
                  if (a->price != b->price)
                  {
                      return a->price > b->price;
                  }
                  return a->timestamp < b->timestamp;
              });

    std::sort(sell_orders.begin(), sell_orders.end(),
              [](const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b)
              {
                  if (a->price != b->price)
                  {
                      return a->price < b->price;
                  }
                  return a->timestamp < b->timestamp;
              });
}

std::string OrderBook::matchOrders()
{
    std::stringstream result;

    sortOrders();

    while (!buy_orders.empty() && !sell_orders.empty())
    {
        auto &best_buy = buy_orders[0];
        auto &best_sell = sell_orders[0];

        if (best_buy->price >= best_sell->price)
        {
            double execution_price;
            if (best_buy->timestamp < best_sell->timestamp)
            {
                execution_price = best_buy->price;
            }
            else
            {
                execution_price = best_sell->price;
            }

            int trade_quantity = std::min(best_buy->quantity, best_sell->quantity);

            result << "TRADE EXECUTED: " << trade_quantity << " " << symbol
                   << " @ $" << std::fixed << std::setprecision(2)
                   << execution_price << "\n";

            best_buy->quantity -= trade_quantity;
            best_sell->quantity -= trade_quantity;

            if (best_buy->quantity == 0)
            {
                buy_orders.erase(buy_orders.begin());
            }
            if (best_sell->quantity == 0)
            {
                sell_orders.erase(sell_orders.begin());
            }
        }
        else
        {
            break;
        }
    }

    return result.str();
}

std::string OrderBook::addOrder(std::shared_ptr<Order> order)
{
    std::lock_guard<std::mutex> lock(book_mutex);

    std::stringstream msg;

    if (order->side == OrderSide::BUY)
    {
        buy_orders.push_back(order);
        msg << "Order added: BUY " << order->quantity << " "
            << order->symbol << " @ $" << std::fixed << std::setprecision(2)
            << order->price << " (Order ID: " << order->order_id << ")\n";
    }
    else
    {
        sell_orders.push_back(order);
        msg << "Order added: SELL " << order->quantity << " "
            << order->symbol << " @ $" << std::fixed << std::setprecision(2)
            << order->price << " (Order ID: " << order->order_id << ")\n";
    }

    std::string match_result = matchOrders();

    return msg.str() + match_result;
}

std::string OrderBook::displayOrders() const
{
    std::lock_guard<std::mutex> lock(book_mutex);

    std::stringstream output;

    output << "\n=== " << symbol << " Order Book ===\n";

    output << "\nBUY ORDERS:\n";
    if (buy_orders.empty())
    {
        output << "  No buy orders\n";
    }
    else
    {
        for (const auto &order : buy_orders)
        {
            output << "  Order #" << order->order_id << ": "
                   << order->quantity << " @ $" << std::fixed << std::setprecision(2)
                   << order->price << "\n";
        }
    }

    output << "\nSELL ORDERS:\n";
    if (sell_orders.empty())
    {
        output << "  No sell orders\n";
    }
    else
    {
        for (const auto &order : sell_orders)
        {
            output << "  Order #" << order->order_id << ": "
                   << order->quantity << " @ $" << std::fixed << std::setprecision(2)
                   << order->price << "\n";
        }
    }

    output << "\n";

    return output.str();
}

// TradingEngine Implementation

OrderBook *TradingEngine::findOrderBook(const std::string &symbol)
{
    auto it = order_books.find(symbol);
    if (it != order_books.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::string TradingEngine::addOrder(const std::string &symbol, OrderSide side, double price, int quantity)
{
    auto order = std::make_shared<Order>(symbol, side, price, quantity, next_order_id++);

    OrderBook *book = nullptr;

    {
        std::lock_guard<std::mutex> lock(engine_mutex);

        // try_emplace constructs in-place if symbol doesn't exist
        auto [it, inserted] = order_books.try_emplace(symbol, symbol);
        book = &it->second;
    }

    return book->addOrder(order);
}

std::string TradingEngine::showOrders(const std::string &symbol)
{
    std::lock_guard<std::mutex> lock(engine_mutex);

    OrderBook *book = findOrderBook(symbol);

    if (book != nullptr)
    {
        return book->displayOrders();
    }
    else
    {
        return "No orders found for symbol: " + symbol + "\n";
    }
}

void TradingEngine::start()
{
    std::cout << "Trading Engine Started..." << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  add_order <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
    std::cout << "  show_orders <SYMBOL>" << std::endl;
    std::cout << "  exit" << std::endl;
    std::cout << std::endl;

    std::string line;
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "exit" || command == "quit")
        {
            std::cout << "Trading Engine Stopped." << std::endl;
            break;
        }
        else if (command == "add_order")
        {
            std::string side_str, symbol;
            double price;
            int quantity;

            if (!(iss >> side_str >> symbol >> price >> quantity))
            {
                std::cout << "Invalid command format. Use: add_order <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
                continue;
            }

            OrderSide side;
            if (side_str == "BUY" || side_str == "buy")
            {
                side = OrderSide::BUY;
            }
            else if (side_str == "SELL" || side_str == "sell")
            {
                side = OrderSide::SELL;
            }
            else
            {
                std::cout << "Invalid side. Use BUY or SELL." << std::endl;
                continue;
            }

            if (price <= 0 || quantity <= 0)
            {
                std::cout << "Price and quantity must be positive." << std::endl;
                continue;
            }

            std::string result = addOrder(symbol, side, price, quantity);
            std::cout << result;
        }
        else if (command == "show_orders")
        {
            std::string symbol;
            if (!(iss >> symbol))
            {
                std::cout << "Invalid command format. Use: show_orders <SYMBOL>" << std::endl;
                continue;
            }

            std::string result = showOrders(symbol);
            std::cout << result;
        }
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Available commands: add_order, show_orders, exit" << std::endl;
        }
    }
}

// trading_engine.h

#ifndef TRADING_ENGINE_H
#define TRADING_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <map>

enum class OrderSide
{
    BUY,
    SELL
};

struct Order
{
    std::string symbol;
    OrderSide side;
    double price;
    int quantity;
    int order_id;
    long long timestamp;

    Order(const std::string &sym, OrderSide s, double p, int q, int id);
};

class OrderBook
{
private:
    std::string symbol;
    std::vector<std::shared_ptr<Order>> buy_orders;  // All buy orders
    std::vector<std::shared_ptr<Order>> sell_orders; // All sell orders

    mutable std::mutex book_mutex; // Thread-safe access to this order book

    std::string matchOrders();

    void sortOrders();

public:
    OrderBook(const std::string &sym) : symbol(sym) {}

    std::string addOrder(std::shared_ptr<Order> order);

    std::string displayOrders() const;
};

class TradingEngine
{
private:
    std::map<std::string, OrderBook> order_books;
    int next_order_id;
    std::mutex engine_mutex; // Protects order_books vector

    OrderBook *findOrderBook(const std::string &symbol);

public:
    TradingEngine() : next_order_id(1) {}

    std::string addOrder(const std::string &symbol, OrderSide side, double price, int quantity);

    std::string showOrders(const std::string &symbol);

    void start();
};

#endif // TRADING_ENGINE_H

// client.cpp

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8080" << std::endl;
        return 1;
    }

    const char *server_ip = argv[1];
    int port = std::stoi(argv[2]);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Failed to connect to server at " << server_ip << ":" << port << std::endl;
        std::cerr << "Make sure the server is running!" << std::endl;
        close(sock);
        return 1;
    }

    std::cout << "==================================" << std::endl;
    std::cout << "Connected to trading server!" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  ADD_ORDER <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>" << std::endl;
    std::cout << "  SHOW_ORDERS <SYMBOL>" << std::endl;
    std::cout << "  DISCONNECT" << std::endl;
    std::cout << std::endl;

    // Command loop
    std::string line;
    char buffer[4096];

    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty())
            continue;

        // Send command
        line += "\n";
        send(sock, line.c_str(), line.length(), 0);

        // Receive response
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0)
        {
            std::cout << "Server disconnected" << std::endl;
            break;
        }

        std::cout << buffer;

        if (line.find("DISCONNECT") == 0)
        {
            break;
        }
    }

    close(sock);
    std::cout << "\nDisconnected from server." << std::endl;
    return 0;
}

// network_server.cpp

#include "network_server.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

NetworkServer::NetworkServer(TradingEngine *eng, int p)
    : engine(eng), server_socket(-1), port(p), running(false)
{
}

NetworkServer::~NetworkServer()
{
    stop();
}

void NetworkServer::start()
{
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
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

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Failed to bind to port " << port << std::endl;
        close(server_socket);
        return;
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0)
    {
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

void NetworkServer::acceptClients()
{
    while (running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket,
                                   (struct sockaddr *)&client_addr,
                                   &client_len);

        if (client_socket < 0)
        {
            if (running)
            {
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
        client_thread.detach(); // Let it run independently
    }
}

void NetworkServer::handleClient(int client_socket)
{
    char buffer[1024];

    while (running)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0)
        {
            std::cout << "[SERVER] Client disconnected" << std::endl;
            break;
        }

        std::string command(buffer);
        std::cout << "[SERVER] Received: " << command;

        std::string response = processCommand(command);

        // Send response
        send(client_socket, response.c_str(), response.length(), 0);

        // Check for disconnect command
        if (command.find("DISCONNECT") == 0)
        {
            break;
        }
    }

    // Clean up
    close(client_socket);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_sockets.erase(
            std::remove(client_sockets.begin(), client_sockets.end(), client_socket),
            client_sockets.end());
    }
}

std::string NetworkServer::processCommand(const std::string &command)
{
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "ADD_ORDER")
    {
        std::string side_str, symbol;
        double price;
        int quantity;

        if (!(iss >> side_str >> symbol >> price >> quantity))
        {
            return "ERROR: Invalid command format\nUsage: ADD_ORDER <BUY|SELL> <SYMBOL> <PRICE> <QUANTITY>\n";
        }

        OrderSide side;
        if (side_str == "BUY")
        {
            side = OrderSide::BUY;
        }
        else if (side_str == "SELL")
        {
            side = OrderSide::SELL;
        }
        else
        {
            return "ERROR: Invalid side. Use BUY or SELL\n";
        }

        if (price <= 0 || quantity <= 0)
        {
            return "ERROR: Price and quantity must be positive\n";
        }

        std::string result = engine->addOrder(symbol, side, price, quantity);
        return result;
    }
    else if (cmd == "SHOW_ORDERS")
    {
        std::string symbol;
        if (!(iss >> symbol))
        {
            return "ERROR: Invalid command format\nUsage: SHOW_ORDERS <SYMBOL>\n";
        }

        return engine->showOrders(symbol);
    }
    else if (cmd == "DISCONNECT")
    {
        return "OK: Goodbye!\n";
    }
    else
    {
        return "ERROR: Unknown command\nAvailable commands: ADD_ORDER, SHOW_ORDERS, DISCONNECT\n";
    }
}

void NetworkServer::broadcastMessage(const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (int socket : client_sockets)
    {
        send(socket, message.c_str(), message.length(), 0);
    }
}

void NetworkServer::stop()
{
    running = false;

    if (server_socket >= 0)
    {
        close(server_socket);
    }

    // Close all client connections
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int socket : client_sockets)
    {
        close(socket);
    }
    client_sockets.clear();
}

// network_server.h

#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "trading_engine.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class NetworkServer
{
private:
    TradingEngine *engine;
    int server_socket;
    int port;
    std::atomic<bool> running;

    std::vector<int> client_sockets;
    std::mutex clients_mutex;

    // Thread functions
    void acceptClients();
    void handleClient(int client_socket);

    // Protocol functions
    std::string processCommand(const std::string &command);

public:
    NetworkServer(TradingEngine *eng, int p);
    ~NetworkServer();

    void start();
    void stop();
    void broadcastMessage(const std::string &message);
};

#endif // NETWORK_SERVER_H

// server_main.cpp

#include "trading_engine.h"
#include "network_server.h"
#include <iostream>

int main()
{
    TradingEngine engine;
    NetworkServer server(&engine, 8080);

    std::cout << "Starting networked trading server...\n"
              << std::endl;
    server.start();

    return 0;
}
