#pragma once
#include "RiskEngine.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace hft {

// Simple INI-style config loader
class Config {
public:
    bool load(const std::string& path);
    RiskLimits toRiskLimits() const;

    double      getDouble(const std::string& key, double def = 0.0) const;
    int64_t     getInt   (const std::string& key, int64_t def = 0)  const;
    std::string getString(const std::string& key, const std::string& def = "") const;

private:
    std::unordered_map<std::string, std::string> data_;
};

} // namespace hft
