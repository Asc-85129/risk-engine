#include "RiskMonitor.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace hft {

RiskMonitor::RiskMonitor(RiskEngine& engine, std::vector<Symbol> symbols)
    : engine_(engine), symbols_(std::move(symbols)) {}

RiskSnapshot RiskMonitor::snapshot() const {
    RiskSnapshot s;
    s.totalPnlUsd    = engine_.getTotalPnl();
    s.approvedOrders = engine_.getApprovedCount();
    s.rejectedOrders = engine_.getRejectedCount();
    uint64_t total   = s.approvedOrders + s.rejectedOrders;
    s.approvalRate   = total > 0 ? 100.0 * s.approvedOrders / total : 100.0;
    for (auto& sym : symbols_)
        s.positions.emplace_back(sym, engine_.getPosition(sym));
    return s;
}

void RiskMonitor::printDashboard() const {
    auto s = snapshot();
    std::cout << "\n╔══════════════════════════════════════╗\n";
    std::cout << "║      HFT Risk Engine Dashboard       ║\n";
    std::cout << "╠══════════════════════════════════════╣\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "║ Total PnL    : $" << std::setw(18) << s.totalPnlUsd << " ║\n";
    std::cout << "║ Approved     : " << std::setw(20) << s.approvedOrders << " ║\n";
    std::cout << "║ Rejected     : " << std::setw(20) << s.rejectedOrders << " ║\n";
    std::cout << "║ Approval Rate: " << std::setw(19) << s.approvalRate << "% ║\n";
    std::cout << "╠══════════════════════════════════════╣\n";
    std::cout << "║ Positions:                           ║\n";
    for (auto& [sym, pos] : s.positions) {
        std::string line = "║   " + sym + ": " + std::to_string(pos);
        line.resize(38, ' ');
        line += " ║";
        std::cout << line << "\n";
    }
    std::cout << "╚══════════════════════════════════════╝\n";
}

} // namespace hft
