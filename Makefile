CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

# Existing targets
trading_engine: main.cpp trading_engine.cpp trading_engine.h
	$(CXX) $(CXXFLAGS) main.cpp trading_engine.cpp -o trading_engine

server: server_main.cpp trading_engine.cpp network_server.cpp trading_engine.h network_server.h
	$(CXX) $(CXXFLAGS) server_main.cpp trading_engine.cpp network_server.cpp -o trading_server

client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o client

# Bot targets
bots/bot_base.o: bots/bot_base.cpp bots/bot_base.h
	$(CXX) $(CXXFLAGS) -c bots/bot_base.cpp -o bots/bot_base.o

market_maker_bot: bots/market_maker_bot.cpp bots/bot_base.o
	$(CXX) $(CXXFLAGS) bots/market_maker_bot.cpp bots/bot_base.o -o market_maker_bot

random_trader_bot: bots/random_trader_bot.cpp bots/bot_base.o
	$(CXX) $(CXXFLAGS) bots/random_trader_bot.cpp bots/bot_base.o -o random_trader_bot

arbitrage_bot: bots/arbitrage_bot.cpp bots/bot_base.o
	$(CXX) $(CXXFLAGS) bots/arbitrage_bot.cpp bots/bot_base.o -o arbitrage_bot

# Build all bots
bots: market_maker_bot random_trader_bot arbitrage_bot

# Build everything
all: trading_engine server client bots

# Clean
clean:
	rm -f trading_engine trading_server client
	rm -f market_maker_bot random_trader_bot arbitrage_bot
	rm -f bots/*.o

.PHONY: all bots clean