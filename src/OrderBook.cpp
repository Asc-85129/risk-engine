#include "OrderBook.hpp"
#include <sstream>
#include <stdexcept>

namespace hft {

void OrderBook::addOrder(uint64_t id, Side side, Price px, Quantity qty) {
    std::lock_guard<std::mutex> lk(mtx_);
    orders_[id] = {side, px, qty};
    if (side == Side::BUY)  bids_[px] += qty;
    else                    asks_[px] += qty;
}

void OrderBook::cancelOrder(uint64_t id) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = orders_.find(id);
    if (it == orders_.end()) return;
    auto& e = it->second;
    if (e.side == Side::BUY) {
        bids_[e.px] -= e.qty;
        if (bids_[e.px] <= 0) bids_.erase(e.px);
    } else {
        asks_[e.px] -= e.qty;
        if (asks_[e.px] <= 0) asks_.erase(e.px);
    }
    orders_.erase(it);
}

bool OrderBook::fillOrder(uint64_t id, Quantity qty) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = orders_.find(id);
    if (it == orders_.end()) return false;
    auto& e = it->second;
    e.qty -= qty;
    if (e.side == Side::BUY) {
        bids_[e.px] -= qty;
        if (bids_[e.px] <= 0) bids_.erase(e.px);
    } else {
        asks_[e.px] -= qty;
        if (asks_[e.px] <= 0) asks_.erase(e.px);
    }
    if (e.qty <= 0) { orders_.erase(it); return true; }
    return false;
}

std::optional<Level> OrderBook::bestBid() const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (bids_.empty()) return std::nullopt;
    auto it = bids_.begin();
    return Level{it->first, it->second};
}

std::optional<Level> OrderBook::bestAsk() const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (asks_.empty()) return std::nullopt;
    auto it = asks_.begin();
    return Level{it->first, it->second};
}

double OrderBook::midPrice() const {
    auto bid = bestBid();
    auto ask = bestAsk();
    if (!bid || !ask) return 0.0;
    return (bid->price + ask->price) / 2.0;
}

std::string OrderBook::toString() const {
    std::ostringstream oss;
    oss << "=== " << symbol_ << " ===\n";
    {
        std::lock_guard<std::mutex> lk(mtx_);
        int i = 0;
        for (auto& [px, qty] : asks_) {
            oss << "  ASK " << px << " x " << qty << "\n";
            if (++i >= 5) break;
        }
        oss << "  ---\n";
        i = 0;
        for (auto& [px, qty] : bids_) {
            oss << "  BID " << px << " x " << qty << "\n";
            if (++i >= 5) break;
        }
    }
    return oss.str();
}

} // namespace hft
