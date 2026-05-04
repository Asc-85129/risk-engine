#include "RiskEngine.hpp"
#include "OrderBook.hpp"
#include "MarketDataFeed.hpp"
#include "RiskMonitor.hpp"
#include "Logger.hpp"
#include "Config.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <sstream>
#include <csignal>

static std::atomic<bool> gRunning{true};

void sigHandler(int) { gRunning = false; }

int main(int argc, char* argv[]) {
    std::signal(SIGINT, sigHandler);

    // ----- Config -----
    hft::Config cfg;
    std::string cfgPath = (argc > 1) ? argv[1] : "config/risk.conf";
    if (!cfg.load(cfgPath)) {
        LOG_WARN("Config not found at " + cfgPath + ", using defaults.");
    }

    hft::RiskLimits limits = cfg.toRiskLimits();
    LOG_INFO("Risk limits loaded.");

    // ----- Logger -----
    hft::Logger::instance().setFile("logs/risk_engine.log");

    // ----- Symbols -----
    std::vector<hft::Symbol> symbols = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA"};

    // ----- Engine -----
    hft::RiskEngine engine(limits);
    engine.setRejectionCallback([](const hft::Order& o, hft::RejectionReason r) {
        std::ostringstream oss;
        oss << "REJECTED order=" << o.orderId << " sym=" << o.symbol
            << " reason=" << static_cast<int>(r);
        LOG_WARN(oss.str());
    });

    // ----- Order Books -----
    std::unordered_map<hft::Symbol, hft::OrderBook*> books;
    for (auto& s : symbols) books[s] = new hft::OrderBook(s);

    // ----- Market Data -----
    hft::MarketDataFeed feed(symbols, 5.0); // tick every 5ms
    feed.subscribe([&](const hft::Tick& tick) {
        engine.onMarketPrice(tick.symbol, (tick.bid + tick.ask) / 2.0);
    });
    feed.start();
    LOG_INFO("Market data feed started.");

    // ----- Order Simulator -----
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> symDist(0, symbols.size() - 1);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> qtyDist(1, 1200);  // some will exceed limit
    std::uniform_real_distribution<double> pxOffset(-2.0, 2.0);

    uint64_t orderId = 1;
    hft::RiskMonitor monitor(engine, symbols);

    LOG_INFO("Starting order simulation. Press Ctrl+C to stop.\n");

    auto printInterval = std::chrono::seconds(2);
    auto lastPrint     = std::chrono::steady_clock::now();

    while (gRunning) {
        // generate a batch of orders
        for (int i = 0; i < 20 && gRunning; ++i) {
            auto& sym = symbols[symDist(rng)];
            hft::Side side = sideDist(rng) ? hft::Side::BUY : hft::Side::SELL;
            hft::Quantity qty = qtyDist(rng);
            hft::Price px = feed.getLastPrice(sym) + pxOffset(rng);
            if (px <= 0) px = 1.0;

            hft::Order order{orderId++, sym, side, qty, px, 0};
            auto result = engine.checkOrder(order);

            if (result.result == hft::RiskResult::APPROVED) {
                books[sym]->addOrder(order.orderId, side, px, qty);
                // Simulate immediate partial fill 50% of time
                if (rng() % 2 == 0) {
                    hft::Quantity filled = qty / 2 + 1;
                    engine.onFill(sym, side, filled, px);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto now = std::chrono::steady_clock::now();
        if (now - lastPrint >= printInterval) {
            monitor.printDashboard();
            lastPrint = now;
        }
    }

    LOG_INFO("Shutting down...");
    feed.stop();

    // Final snapshot
    monitor.printDashboard();

    // Print order books
    for (auto& [sym, book] : books) {
        std::cout << book->toString();
        delete book;
    }

    LOG_INFO("Done.");
    return 0;
}
