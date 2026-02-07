#pragma once
#include <stdexcept>
#include <iostream>
#include <thread>

class Stopwatch {
public:
    // 不受系统时间调整影响
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    explicit constexpr Stopwatch(bool auto_start = true) noexcept
        : _is_running(false)
        , _is_paused(false)
        , _accumulated_duration(Duration::zero()) {

        if (auto_start) {
            start();
        }
    }

    ~Stopwatch() noexcept {
        if (is_running() && !is_paused()) {
            stop();
        }
        std::cout << "duration: " << get_elapsed_seconds() << " (s)" << std::endl;
    }

    // 防止状态被拷贝
    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;
    // 允许移动
    Stopwatch(Stopwatch&&) noexcept = default;
    Stopwatch& operator=(Stopwatch&&) noexcept = default;

    void start() {
        if (_is_running) {
            throw std::runtime_error("Stopwatch is already running!");
        }
        _start_time = Clock::now();
        _is_running = true;
        _is_paused = false;
    }

    void stop() {
        if (!_is_running || _is_paused) {
            throw std::runtime_error("Stopwatch is not running!");
        }
        _accumulated_duration += Clock::now() - _start_time;
        _is_running = false;
    }

    void pause() {
        if (!_is_running || _is_paused) {
            throw std::runtime_error("Cannot pause a non-running or paused stopwatch!");
        }
        _pause_time = Clock::now();
        _accumulated_duration += _pause_time - _start_time;
        _is_paused = true;
    }

    void resume() {
        if (!_is_paused) {
            throw std::runtime_error("Cannot resume a non-paused stopwatch!");
        }
        _start_time = Clock::now();
        _is_paused = false;
    }

    // 清空状态
    void reset() {
        _is_running = false;
        _is_paused = false;
        _accumulated_duration = Duration::zero();
        _start_time = TimePoint{};
        _pause_time = TimePoint{};
    }
    // 获取耗时(s)
    double get_elapsed_seconds() const {
        // 1. 先获取原始时长（纳秒为单位）
        Duration total_duration = get_elapsed_duration();
        // 2. 转换为浮点型的秒单位时长（std::chrono::duration<double> 表示“秒”，浮点型）
        std::chrono::duration<double> seconds_duration = total_duration;
        // 3. 返回秒数（保留高精度浮点）
        return seconds_duration.count();
    }
    // 获取耗时(ms)
    long long get_elapsed_milliseconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(get_elapsed_duration()).count();
    }
    // 获取耗时(us)
    long long get_elapsed_microseconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(get_elapsed_duration()).count();
    }   
    // 获取耗时(ns)
    long long get_elapsed_nanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(get_elapsed_duration()).count();
    }

    bool is_running() const noexcept {
        return _is_running && !_is_paused;
    }

    bool is_paused() const noexcept {
        return _is_paused;
    }

private:
    // 计算当前累计耗时
    Duration get_elapsed_duration() const {
        if (_is_running && !_is_paused) {
            return _accumulated_duration + (Clock::now() - _start_time);
        } else {
            return _accumulated_duration;
        }
    }

private:
    // 是否启动
    bool _is_running{false};
    // 是否暂停
    bool _is_paused{false};
    // 累计耗时
    Duration _accumulated_duration{Duration::zero()};
    // 启动时间节点
    TimePoint _start_time{};
    // 暂停时间节点
    TimePoint _pause_time{};
};
