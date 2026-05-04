#!/usr/bin/env bash
set -e

echo "=== HFT Risk Engine Build Script ==="

# Create logs dir
mkdir -p logs

# Create build dir
mkdir -p build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel $(nproc)

echo ""
echo "=== Build Complete ==="
echo ""
echo "Run engine:  ./build/hft_risk_engine"
echo "Run engine with custom config: ./build/hft_risk_engine config/risk.conf"
echo "Run tests:   ./build/run_tests"
