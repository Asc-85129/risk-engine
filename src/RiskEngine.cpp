#include "RiskEngine.hpp"
#include <sstream>
#include <cmath>
#include <stdexcept>

namespace hft {

static Nanos nowNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

RiskEngine::RiskEngine(RiskLimits limits) : limits_(limits) {}

PositionState& RiskEngine::getOrCreatePosition(const Symbol& sym) {
    auto it = positions_.find(sym);
    if (it == positions_.end()) {
        auto* ps = new PositionState();
        positions_[sym] = ps;
        return *ps;
    }
    return *it->second;
}

bool RiskEngine::checkRateLimit(Nanos now) {
    Nanos window = rateLimitWindow_.load(std::memory_order_relaxed);
    Nanos windowStart = window - (window % 1'000'000'000LL); // floor to second
    Nanos nowWindow   = now  - (now  % 1'000'000'000LL);

    if (nowWindow != windowStart) {
        ordersThisSec_.store(0, std::memory_order_relaxed);
        rateLimitWindow_.store(nowWindow, std::memory_order_relaxed);
    }
    uint32_t cnt = ordersThisSec_.fetch_add(1, std::memory_order_relaxed);
    return cnt < limits_.maxOrdersPerSec;
}

RiskCheckResult RiskEngine::reject(const Order& o, RejectionReason r, const std::string& msg) {
    rejectedCount_.fetch_add(1, std::memory_order_relaxed);
    if (rejectionCb_) rejectionCb_(o, r);
    return {RiskResult::REJECTED, r, msg};
}

RiskCheckResult RiskEngine::checkOrder(const Order& order) {
    Nanos now = order.timestamp > 0 ? order.timestamp : nowNanos();

    // 1. Order size
    if (order.qty <= 0 || order.qty > limits_.maxOrderSize) {
        return reject(order, RejectionReason::MAX_ORDER_SIZE_EXCEEDED,
            "Order size " + std::to_string(order.qty) + " exceeds max " +
            std::to_string(limits_.maxOrderSize));
    }

    // 2. Notional
    double notional = order.qty * order.price;
    if (notional > limits_.maxNotionalUsd) {
        return reject(order, RejectionReason::MAX_NOTIONAL_EXCEEDED,
            "Notional " + std::to_string(notional) + " exceeds max " +
            std::to_string(limits_.maxNotionalUsd));
    }

    // 3. Rate limit
    if (!checkRateLimit(now)) {
        return reject(order, RejectionReason::ORDER_RATE_EXCEEDED,
            "Order rate exceeds " + std::to_string(limits_.maxOrdersPerSec) + "/s");
    }

    // 4. Position limit
    {
        std::lock_guard<std::mutex> lk(mtx_);
        PositionState& ps = getOrCreatePosition(order.symbol);
        Quantity cur = ps.netPosition.load(std::memory_order_relaxed);
        Quantity delta = (order.side == Side::BUY) ? order.qty : -order.qty;
        Quantity projected = cur + delta;
        if (std::abs(projected) > limits_.maxPosition) {
            return reject(order, RejectionReason::MAX_POSITION_EXCEEDED,
                "Projected position " + std::to_string(projected) +
                " exceeds max " + std::to_string(limits_.maxPosition));
        }
    }

    // 5. Daily loss
    int64_t pnlCents = totalRealizedPnl_.load(std::memory_order_relaxed);
    double  pnlUsd   = pnlCents / 100.0;
    if (pnlUsd < -limits_.maxDailyLoss) {
        return reject(order, RejectionReason::MAX_DAILY_LOSS_EXCEEDED,
            "Daily loss " + std::to_string(-pnlUsd) + " exceeds max " +
            std::to_string(limits_.maxDailyLoss));
    }

    // 6. Drawdown
    int64_t peak = peakPnl_.load(std::memory_order_relaxed);
    double  dd   = (peak - pnlCents) / 100.0;
    if (dd > limits_.maxDrawdown) {
        return reject(order, RejectionReason::MAX_DRAWDOWN_EXCEEDED,
            "Drawdown " + std::to_string(dd) + " exceeds max " +
            std::to_string(limits_.maxDrawdown));
    }

    approvedCount_.fetch_add(1, std::memory_order_relaxed);
    return {RiskResult::APPROVED, RejectionReason::NONE, "OK"};
}

void RiskEngine::onFill(const Symbol& sym, Side side, Quantity qty, Price px) {
    std::lock_guard<std::mutex> lk(mtx_);
    PositionState& ps = getOrCreatePosition(sym);

    Quantity delta = (side == Side::BUY) ? qty : -qty;
    Quantity prev  = ps.netPosition.load(std::memory_order_relaxed);
    ps.netPosition.store(prev + delta, std::memory_order_relaxed);

    // Simple realized PnL: SELL reduces long position at profit/loss
    int64_t fillCents = static_cast<int64_t>(qty * px * 100.0);
    if (side == Side::SELL) {
        totalRealizedPnl_.fetch_add(fillCents, std::memory_order_relaxed);
    } else {
        totalRealizedPnl_.fetch_sub(fillCents, std::memory_order_relaxed);
    }

    int64_t cur = totalRealizedPnl_.load(std::memory_order_relaxed);
    int64_t pk  = peakPnl_.load(std::memory_order_relaxed);
    if (cur > pk) peakPnl_.store(cur, std::memory_order_relaxed);
}

void RiskEngine::onMarketPrice(const Symbol& sym, Price px) {
    std::lock_guard<std::mutex> lk(mtx_);
    marketPrices_[sym] = px;
    // update unrealized
    auto it = positions_.find(sym);
    if (it != positions_.end()) {
        Quantity pos = it->second->netPosition.load(std::memory_order_relaxed);
        int64_t upnl = static_cast<int64_t>(pos * px * 100.0);
        it->second->unrealizedPnl.store(upnl, std::memory_order_relaxed);
    }
}

void RiskEngine::resetDaily() {
    std::lock_guard<std::mutex> lk(mtx_);
    dailyPnlStart_.store(totalRealizedPnl_.load(), std::memory_order_relaxed);
}

double RiskEngine::getTotalPnl() const {
    return totalRealizedPnl_.load(std::memory_order_relaxed) / 100.0;
}

Quantity RiskEngine::getPosition(const Symbol& sym) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = positions_.find(sym);
    if (it == positions_.end()) return 0;
    return it->second->netPosition.load(std::memory_order_relaxed);
}

} // namespace hft
