#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "RiskEngine.hpp"
#include "OrderBook.hpp"

using namespace hft;

static Order makeOrder(uint64_t id, const Symbol& sym, Side side,
                       Quantity qty, Price px, Nanos ts = 0) {
    return {id, sym, side, qty, px, ts};
}

static RiskLimits defaultLimits() {
    RiskLimits l;
    l.maxPosition     = 1000;
    l.maxOrderSize    = 500;
    l.maxNotionalUsd  = 100'000.0;
    l.maxDailyLoss    = 10'000.0;
    l.maxDrawdown     = 20'000.0;
    l.maxOrdersPerSec = 1000;
    return l;
}

// -------- RiskEngine tests --------

TEST_CASE("Approve normal order", "[risk]") {
    RiskEngine eng(defaultLimits());
    auto r = eng.checkOrder(makeOrder(1, "AAPL", Side::BUY, 100, 185.0));
    REQUIRE(r.result == RiskResult::APPROVED);
}

TEST_CASE("Reject oversized order", "[risk]") {
    RiskEngine eng(defaultLimits());
    auto r = eng.checkOrder(makeOrder(1, "AAPL", Side::BUY, 600, 185.0));
    REQUIRE(r.result == RiskResult::REJECTED);
    REQUIRE(r.reason == RejectionReason::MAX_ORDER_SIZE_EXCEEDED);
}

TEST_CASE("Reject zero quantity", "[risk]") {
    RiskEngine eng(defaultLimits());
    auto r = eng.checkOrder(makeOrder(1, "AAPL", Side::BUY, 0, 185.0));
    REQUIRE(r.result == RiskResult::REJECTED);
}

TEST_CASE("Reject notional breach", "[risk]") {
    RiskEngine eng(defaultLimits());
    // 500 * 300 = 150,000 > 100,000
    auto r = eng.checkOrder(makeOrder(1, "AAPL", Side::BUY, 500, 300.0));
    REQUIRE(r.result == RiskResult::REJECTED);
    REQUIRE(r.reason == RejectionReason::MAX_NOTIONAL_EXCEEDED);
}

TEST_CASE("Reject position breach", "[risk]") {
    RiskEngine eng(defaultLimits());
    // Fill to near limit
    eng.onFill("AAPL", Side::BUY, 900, 185.0);
    // Now try to buy more that would exceed 1000
    auto r = eng.checkOrder(makeOrder(2, "AAPL", Side::BUY, 200, 185.0));
    REQUIRE(r.result == RiskResult::REJECTED);
    REQUIRE(r.reason == RejectionReason::MAX_POSITION_EXCEEDED);
}

TEST_CASE("Position tracks fills correctly", "[risk]") {
    RiskEngine eng(defaultLimits());
    eng.onFill("AAPL", Side::BUY,  300, 185.0);
    eng.onFill("AAPL", Side::SELL, 100, 186.0);
    REQUIRE(eng.getPosition("AAPL") == 200);
}

TEST_CASE("PnL tracks correctly", "[risk]") {
    RiskEngine eng(defaultLimits());
    eng.onFill("AAPL", Side::BUY,  100, 100.0);  // cost 10000
    eng.onFill("AAPL", Side::SELL, 100, 110.0);  // revenue 11000
    // realized pnl should be positive
    REQUIRE(eng.getTotalPnl() > 0);
}

TEST_CASE("Rate limit rejects excess orders", "[risk]") {
    RiskLimits lim = defaultLimits();
    lim.maxOrdersPerSec = 3;
    RiskEngine eng(lim);

    // Use same timestamp window (same nanosecond second)
    Nanos ts = 1'000'000'000LL; // 1 second mark
    int approved = 0;
    for (int i = 0; i < 10; ++i) {
        auto r = eng.checkOrder(makeOrder(i+1, "AAPL", Side::BUY, 10, 185.0, ts));
        if (r.result == RiskResult::APPROVED) ++approved;
    }
    REQUIRE(approved <= 3);
}

TEST_CASE("Approved/rejected counters", "[risk]") {
    RiskEngine eng(defaultLimits());
    eng.checkOrder(makeOrder(1, "AAPL", Side::BUY, 100, 185.0)); // approved
    eng.checkOrder(makeOrder(2, "AAPL", Side::BUY, 600, 185.0)); // rejected (size)
    REQUIRE(eng.getApprovedCount() == 1);
    REQUIRE(eng.getRejectedCount() == 1);
}

// -------- OrderBook tests --------

TEST_CASE("OrderBook best bid/ask", "[book]") {
    OrderBook book("AAPL");
    book.addOrder(1, Side::BUY,  184.0, 100);
    book.addOrder(2, Side::BUY,  183.0, 200);
    book.addOrder(3, Side::SELL, 185.0, 100);
    book.addOrder(4, Side::SELL, 186.0, 50);

    auto bid = book.bestBid();
    auto ask = book.bestAsk();
    REQUIRE(bid.has_value());
    REQUIRE(ask.has_value());
    REQUIRE(bid->price == Approx(184.0));
    REQUIRE(ask->price == Approx(185.0));
}

TEST_CASE("OrderBook mid price", "[book]") {
    OrderBook book("MSFT");
    book.addOrder(1, Side::BUY,  419.0, 50);
    book.addOrder(2, Side::SELL, 421.0, 50);
    REQUIRE(book.midPrice() == Approx(420.0));
}

TEST_CASE("OrderBook cancel", "[book]") {
    OrderBook book("TSLA");
    book.addOrder(1, Side::BUY, 250.0, 100);
    book.cancelOrder(1);
    REQUIRE_FALSE(book.bestBid().has_value());
}

TEST_CASE("OrderBook fill", "[book]") {
    OrderBook book("GOOGL");
    book.addOrder(1, Side::SELL, 175.0, 100);
    bool fullyFilled = book.fillOrder(1, 100);
    REQUIRE(fullyFilled);
    REQUIRE_FALSE(book.bestAsk().has_value());
}

TEST_CASE("OrderBook partial fill", "[book]") {
    OrderBook book("AMZN");
    book.addOrder(1, Side::BUY, 195.0, 100);
    bool fullyFilled = book.fillOrder(1, 60);
    REQUIRE_FALSE(fullyFilled);
    auto bid = book.bestBid();
    REQUIRE(bid.has_value());
    REQUIRE(bid->qty == 40);
}
