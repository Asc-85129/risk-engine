#include "MarketDataFeed.hpp"
#include <chrono>
#include <thread>

namespace hft {

static const std::unordered_map<std::string, double> kInitPrices = {
    {"AAPL", 185.0}, {"MSFT", 420.0}, {"GOOGL", 175.0},
    {"AMZN", 195.0}, {"TSLA", 250.0}
};

MarketDataFeed::MarketDataFeed(std::vector<Symbol> symbols, double tickIntervalMs)
    : symbols_(std::move(symbols)), tickIntervalMs_(tickIntervalMs),
      rng_(std::random_device{}()) {
    for (auto& s : symbols_) {
        auto it = kInitPrices.find(s);
        prices_[s] = (it != kInitPrices.end()) ? it->second : 100.0;
    }
}

MarketDataFeed::~MarketDataFeed() { stop(); }

void MarketDataFeed::start() {
    running_ = true;
    thread_  = std::thread(&MarketDataFeed::run, this);
}

void MarketDataFeed::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void MarketDataFeed::subscribe(TickCallback cb) {
    std::lock_guard<std::mutex> lk(cbMtx_);
    callbacks_.push_back(std::move(cb));
}

Price MarketDataFeed::getLastPrice(const Symbol& sym) const {
    std::lock_guard<std::mutex> lk(cbMtx_);
    auto it = prices_.find(sym);
    return it != prices_.end() ? it->second : 0.0;
}

void MarketDataFeed::run() {
    auto interval = std::chrono::microseconds(
        static_cast<int64_t>(tickIntervalMs_ * 1000));

    while (running_) {
        auto wakeup = std::chrono::steady_clock::now() + interval;

        for (auto& sym : symbols_) {
            double move = noise_(rng_) * prices_[sym] * 0.001;
            prices_[sym] = std::max(1.0, prices_[sym] + move);

            double spread = prices_[sym] * 0.0001; // 1 bps spread
            Tick tick{sym,
                      prices_[sym] - spread / 2,
                      prices_[sym] + spread / 2,
                      std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::steady_clock::now().time_since_epoch()).count()};

            std::lock_guard<std::mutex> lk(cbMtx_);
            for (auto& cb : callbacks_) cb(tick);
        }

        std::this_thread::sleep_until(wakeup);
    }
}

} // namespace hft
