#pragma once
#include "RiskEngine.hpp"
#include <map>
#include <deque>
#include <mutex>
#include <optional>

namespace hft {

struct Level {
    Price    price;
    Quantity qty;
};

class OrderBook {
public:
    explicit OrderBook(Symbol sym) : symbol_(std::move(sym)) {}

    void addOrder(uint64_t id, Side side, Price px, Quantity qty);
    void cancelOrder(uint64_t id);
    bool fillOrder(uint64_t id, Quantity qty); // returns true if fully filled

    std::optional<Level> bestBid() const;
    std::optional<Level> bestAsk() const;
    double               midPrice() const;
    std::string          toString() const;

private:
    Symbol symbol_;
    mutable std::mutex mtx_;

    struct OrderEntry { Side side; Price px; Quantity qty; };
    std::unordered_map<uint64_t, OrderEntry> orders_;

    // bids: descending, asks: ascending
    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity>                      asks_;
};

} // namespace hft
