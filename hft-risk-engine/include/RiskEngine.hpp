#pragma once
#include <atomic>
#include <unordered_map>
#include <string>
#include <chrono>
#include <functional>
#include <mutex>
#include <vector>

namespace hft {

using Price    = double;
using Quantity = int64_t;
using Symbol   = std::string;
using Nanos    = int64_t;

enum class Side { BUY, SELL };
enum class RiskResult { APPROVED, REJECTED };
enum class RejectionReason {
    NONE,
    MAX_POSITION_EXCEEDED,
    MAX_ORDER_SIZE_EXCEEDED,
    MAX_NOTIONAL_EXCEEDED,
    MAX_DAILY_LOSS_EXCEEDED,
    ORDER_RATE_EXCEEDED,
    MAX_DRAWDOWN_EXCEEDED
};

struct Order {
    uint64_t   orderId;
    Symbol     symbol;
    Side       side;
    Quantity   qty;
    Price      price;
    Nanos      timestamp;
};

struct RiskCheckResult {
    RiskResult      result;
    RejectionReason reason;
    std::string     message;
};

struct PositionState {
    std::atomic<Quantity> netPosition{0};
    std::atomic<int64_t>  realizedPnl{0};  // in cents
    std::atomic<int64_t>  unrealizedPnl{0};
    std::atomic<uint64_t> orderCount{0};
};

struct RiskLimits {
    Quantity maxPosition       = 10000;
    Quantity maxOrderSize      = 1000;
    double   maxNotionalUsd    = 1'000'000.0;
    double   maxDailyLoss      = 50'000.0;
    double   maxDrawdown       = 100'000.0;
    uint32_t maxOrdersPerSec   = 500;
};

class RiskEngine {
public:
    explicit RiskEngine(RiskLimits limits);

    RiskCheckResult checkOrder(const Order& order);
    void            onFill(const Symbol& sym, Side side, Quantity qty, Price px);
    void            onMarketPrice(const Symbol& sym, Price px);
    void            resetDaily();

    double          getTotalPnl() const;
    Quantity        getPosition(const Symbol& sym) const;
    uint64_t        getRejectedCount() const { return rejectedCount_.load(); }
    uint64_t        getApprovedCount() const { return approvedCount_.load(); }

    void setRejectionCallback(std::function<void(const Order&, RejectionReason)> cb) {
        rejectionCb_ = std::move(cb);
    }

private:
    RiskLimits limits_;
    mutable std::mutex mtx_;

    std::unordered_map<Symbol, PositionState*> positions_;
    std::unordered_map<Symbol, Price>          marketPrices_;

    // Rate limiting
    std::atomic<uint32_t> ordersThisSec_{0};
    std::atomic<Nanos>    rateLimitWindow_{0};

    // PnL tracking
    std::atomic<int64_t> totalRealizedPnl_{0};  // cents
    std::atomic<int64_t> peakPnl_{0};
    std::atomic<int64_t> dailyPnlStart_{0};

    std::atomic<uint64_t> rejectedCount_{0};
    std::atomic<uint64_t> approvedCount_{0};

    std::function<void(const Order&, RejectionReason)> rejectionCb_;

    PositionState& getOrCreatePosition(const Symbol& sym);
    bool           checkRateLimit(Nanos now);
    RiskCheckResult reject(const Order& o, RejectionReason r, const std::string& msg);
};

} // namespace hft
