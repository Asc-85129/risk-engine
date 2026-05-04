#pragma once
#include "RiskEngine.hpp"
#include <string>
#include <vector>

namespace hft {

struct RiskSnapshot {
    double   totalPnlUsd;
    uint64_t approvedOrders;
    uint64_t rejectedOrders;
    double   approvalRate;
    std::vector<std::pair<Symbol, Quantity>> positions;
};

class RiskMonitor {
public:
    explicit RiskMonitor(RiskEngine& engine,
                         std::vector<Symbol> symbols);

    RiskSnapshot snapshot() const;
    void         printDashboard() const;

private:
    RiskEngine&         engine_;
    std::vector<Symbol> symbols_;
};

} // namespace hft
