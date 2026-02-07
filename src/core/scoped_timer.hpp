#pragma once
#include <chrono>
#include <iostream>

class ScopedTimer {
public:
    using ClockType = std::chrono::steady_clock;
    ScopedTimer(std::string func)
        : _function_name(std::move(func)), _start(ClockType::now()) {}    
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    auto operator=(const ScopedTimer&) -> ScopedTimer& = delete;
    auto operator=(ScopedTimer&&) -> ScopedTimer& = delete;
    ~ScopedTimer() {
        using namespace std::chrono;
        auto stop = ClockType::now();
        auto duration = (stop - _start);
        auto ms = duration_cast<milliseconds>(duration).count();
        std::cout << ms << " ms " << _function_name << "\n";
    }

private:
    const std::string _function_name{};
    const ClockType::time_point _start{};
};
