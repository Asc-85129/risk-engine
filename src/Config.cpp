#include "Config.hpp"
#include <iostream>

namespace hft {

bool Config::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // trim
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        data_[key] = val;
    }
    return true;
}

double Config::getDouble(const std::string& key, double def) const {
    auto it = data_.find(key);
    return it != data_.end() ? std::stod(it->second) : def;
}

int64_t Config::getInt(const std::string& key, int64_t def) const {
    auto it = data_.find(key);
    return it != data_.end() ? std::stoll(it->second) : def;
}

std::string Config::getString(const std::string& key, const std::string& def) const {
    auto it = data_.find(key);
    return it != data_.end() ? it->second : def;
}

RiskLimits Config::toRiskLimits() const {
    RiskLimits l;
    l.maxPosition     = static_cast<Quantity>(getInt("max_position",     10000));
    l.maxOrderSize    = static_cast<Quantity>(getInt("max_order_size",   1000));
    l.maxNotionalUsd  = getDouble("max_notional_usd",  1'000'000.0);
    l.maxDailyLoss    = getDouble("max_daily_loss",    50'000.0);
    l.maxDrawdown     = getDouble("max_drawdown",      100'000.0);
    l.maxOrdersPerSec = static_cast<uint32_t>(getInt("max_orders_per_sec", 500));
    return l;
}

} // namespace hft
