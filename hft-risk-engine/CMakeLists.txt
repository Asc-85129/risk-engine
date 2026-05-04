cmake_minimum_required(VERSION 3.16)
project(hft_risk_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile flags for performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Library ----
set(LIB_SOURCES
    src/RiskEngine.cpp
    src/OrderBook.cpp
    src/MarketDataFeed.cpp
    src/RiskMonitor.cpp
    src/Logger.cpp
    src/Config.cpp
)

add_library(hft_lib STATIC ${LIB_SOURCES})
target_include_directories(hft_lib PUBLIC include)
target_link_libraries(hft_lib pthread)

# ---- Main binary ----
add_executable(hft_risk_engine src/main.cpp)
target_link_libraries(hft_risk_engine hft_lib)

# ---- Tests (Catch2 via FetchContent) ----
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.10
)
FetchContent_MakeAvailable(Catch2)

add_executable(run_tests tests/test_risk.cpp)
target_link_libraries(run_tests hft_lib Catch2::Catch2)

enable_testing()
add_test(NAME RiskTests COMMAND run_tests)

# Install
install(TARGETS hft_risk_engine DESTINATION bin)
