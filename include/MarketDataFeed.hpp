#pragma once
#include "RiskEngine.hpp"
#include "OrderBook.hpp"
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <random>

namespace hft {

struct Tick {
    Symbol symbol;
    Price  bid;
    Price  ask;
    Nanos  timestamp;
};

using TickCallback = std::function<void(const Tick&)>;

// Simulates a market data feed with Gaussian price moves
class MarketDataFeed {
public:
    MarketDataFeed(std::vector<Symbol> symbols, double tickIntervalMs = 1.0);
    ~MarketDataFeed();

    void start();
    void stop();
    void subscribe(TickCallback cb);

    Price getLastPrice(const Symbol& sym) const;

private:
    void run();

    std::vector<Symbol>       symbols_;
    double                    tickIntervalMs_;
    std::atomic<bool>         running_{false};
    std::thread               thread_;
    std::vector<TickCallback> callbacks_;
    mutable std::mutex        cbMtx_;

    std::unordered_map<Symbol, Price> prices_;
    std::mt19937                      rng_;
    std::normal_distribution<double>  noise_{0.0, 0.05};
};

} // namespace hft
