# HFT Risk Management Engine

A production-inspired, low-latency **Risk Management Engine** written in modern C++17, designed for High-Frequency Trading (HFT) environments.

## Features

| Feature | Details |
|---|---|
| **Pre-trade Risk Checks** | Order size, notional, position limits, daily loss, drawdown |
| **Rate Limiting** | Configurable orders/second with lock-free counters |
| **Order Book** | Price-level aggregated bid/ask with O(log n) ops |
| **Market Data Feed** | Simulated tick feed with Gaussian price moves |
| **PnL Tracking** | Realized PnL per fill, peak tracking, drawdown |
| **Risk Monitor** | Live dashboard with positions and approval rate |
| **Configurable Limits** | INI-style config file |
| **Logging** | Thread-safe timestamped logger to stdout + file |
| **Unit Tests** | Catch2 test suite covering all risk scenarios |

## Architecture

```
┌─────────────────────────────────────────────┐
│                  main.cpp                    │
│   (Order Simulator + Feed Integration)       │
└────────┬──────────────┬───────────┬──────────┘
         │              │           │
   ┌─────▼──────┐ ┌────▼────┐ ┌───▼────────┐
   │ RiskEngine │ │OrderBook│ │MarketData  │
   │            │ │         │ │Feed        │
   │ - Checks   │ │ - Bids  │ │            │
   │ - PnL      │ │ - Asks  │ │ - Ticks    │
   │ - Limits   │ │ - Fills │ │ - Prices   │
   └─────┬──────┘ └─────────┘ └────────────┘
         │
   ┌─────▼──────┐    ┌──────────┐   ┌────────┐
   │RiskMonitor │    │  Logger  │   │ Config │
   │ - Snapshot │    │ - File   │   │ - INI  │
   │ - Dashboard│    │ - Stdout │   │ - Loads│
   └────────────┘    └──────────┘   └────────┘
```

## Risk Checks (in order)

1. **Order Size** — qty must be `> 0` and `<= max_order_size`
2. **Notional** — `qty * price <= max_notional_usd`
3. **Rate Limit** — orders per second rolling window
4. **Position Limit** — projected net position stays within `±max_position`
5. **Daily Loss** — realized PnL doesn't exceed `max_daily_loss`
6. **Drawdown** — `peak_pnl - current_pnl <= max_drawdown`

## Real-world HFT Criticalities Addressed

- **Lock-free atomics** for counters (approved/rejected/position) — no contention on hot path
- **Mutex only where necessary** (position map writes, market price updates)
- **Nanosecond timestamps** for rate limiting windows
- **Signal handling** for graceful shutdown (SIGINT)
- **Separation of concerns** — risk engine has no knowledge of market data or order generation
- **Callback pattern** for rejection events — pluggable alerting

## Prerequisites

- **CMake** >= 3.16
- **GCC** >= 9 or **Clang** >= 10 (C++17 required)
- **Git** (for Catch2 via FetchContent)
- **Internet access** (first build fetches Catch2)

### Install on Ubuntu/Debian
```bash
sudo apt update && sudo apt install -y build-essential cmake git
```

### Install on macOS
```bash
brew install cmake
xcode-select --install
```

## Build & Run

```bash
# Clone
git clone https://github.com/YOUR_USERNAME/hft-risk-engine.git
cd hft-risk-engine

# Build
chmod +x scripts/build.sh
./scripts/build.sh

# Run (Ctrl+C to stop)
./build/hft_risk_engine

# Run with custom config
./build/hft_risk_engine config/risk.conf

# Run tests
./build/run_tests
```

## Configuration (`config/risk.conf`)

```ini
max_position       = 10000     # Max net shares per symbol
max_order_size     = 1000      # Max qty per single order
max_notional_usd   = 1000000   # Max order notional ($)
max_daily_loss     = 50000     # Max daily loss ($) before halt
max_drawdown       = 100000    # Max drawdown from peak PnL ($)
max_orders_per_sec = 500       # Rate limiter
```

## Sample Output

```
╔══════════════════════════════════════╗
║      HFT Risk Engine Dashboard       ║
╠══════════════════════════════════════╣
║ Total PnL    :          -12345.67 ║
║ Approved     :                 842 ║
║ Rejected     :                 158 ║
║ Approval Rate:               84.2% ║
╠══════════════════════════════════════╣
║ Positions:                           ║
║   AAPL: 320                          ║
║   MSFT: -150                         ║
║   GOOGL: 0                           ║
║   AMZN: 480                          ║
║   TSLA: -90                          ║
╚══════════════════════════════════════╝
```

## GitHub Setup Commands

```bash
git init
git add .
git commit -m "feat: initial HFT risk management engine"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/hft-risk-engine.git
git push -u origin main
```

## Project Structure

```
hft-risk-engine/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── config/
│   └── risk.conf
├── include/
│   ├── RiskEngine.hpp
│   ├── OrderBook.hpp
│   ├── MarketDataFeed.hpp
│   ├── RiskMonitor.hpp
│   ├── Logger.hpp
│   └── Config.hpp
├── src/
│   ├── main.cpp
│   ├── RiskEngine.cpp
│   ├── OrderBook.cpp
│   ├── MarketDataFeed.cpp
│   ├── RiskMonitor.cpp
│   ├── Logger.cpp
│   └── Config.cpp
├── tests/
│   └── test_risk.cpp
└── scripts/
    └── build.sh
```

## License
MIT
