#pragma once
// 1. 异步编程核心（必须有，修复当前错误）
#include <future>          // std::packaged_task / std::future / std::promise
// 2. 线程与并发基础
#include <thread>          // std::thread
#include <mutex>           // std::mutex / std::lock_guard / std::unique_lock
#include <condition_variable> // std::condition_variable
#include <atomic>          // std::atomic
// 3. 容器与函数封装
#include <vector>          // std::vector（存储工作线程）
#include <queue>           // std::queue（任务队列）
#include <functional>      // std::function（统一任务类型）
// 4. 其他基础依赖
#include <memory>          // std::shared_ptr / std::make_shared
#include <stdexcept>       // std::runtime_error（异常抛出）
#include <utility>         // std::forward / std::move（完美转发）
#include <iostream>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_num=std::thread::hardware_concurrency())
        : _is_running(true) {
        if (thread_num == 0) thread_num = 1;
        create_worker_threads(thread_num);
    }

    ~ThreadPool() {
        stop();
        _cv.notify_all();
        for (auto& worker : _workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // 禁用拷贝构造和拷贝赋值(线程池不可进行拷贝)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 启用移动构造和移动赋值
    ThreadPool(ThreadPool&&) noexcept = default;
    ThreadPool& operator=(ThreadPool&&) noexcept = default;

    // 提交任务
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        // 推导任务的返回类型
        using ReturnType = std::invoke_result_t<F, Args...>;
        // 封装任务为std::packaged_task(绑定结果到std::future), 使用智能指针管理
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // 获取任务的future, 供外部获取结果
        std::future<ReturnType> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_is_running) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            // 将任务封装为无参可调用对象,加入任务队列
            _tasks.emplace([task](){ (*task)(); });
        }

        // 唤醒一个休眠的工作线程,执行新任务
        _cv.notify_one();
        return res;
    }

    // 主动停止线程池(可外部调用,析构时会自动调用)
    void stop() {
        _is_running.store(false, std::memory_order_seq_cst);
    }

    // 获取当前工作线程的数量
    size_t thread_count() const {
        return _workers.size();
    }

private:
    // 创建指定数量的工作线程
    void create_worker_threads(size_t thread_num) {
        _workers.reserve(thread_num);
        for (size_t i = 0; i < thread_num; ++i) {
            _workers.emplace_back(&ThreadPool::worker_loop, this);
        }
    }

    // 工作线程的核心循环, 持续获取任务执行, 知道线程池停止
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                // 加锁等待: 无任务且线程池运行时,释放锁并休眠
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [this]() {
                    // 唤醒条件: 线程池停止, 或者任务队列非空
                    return !_is_running || !_tasks.empty();
                });
                
                // 循环退出条件
                if (!_is_running && _tasks.empty()) {
                    return;
                }

                // 从任务队列的头部取出任务,并从任务队列中删除
                task = std::move(_tasks.front());
                _tasks.pop();
            }

            try {
                if (task) {
                    task();
                }
            } catch (const std::exception& e) {
                std::cerr << "Worker thread execute task error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Worker thread execute task unknown error" << std::endl;
            }
        }
    }

private:
    // 工作线程
    std::vector<std::thread> _workers;
    // 线程安全的任务队列
    std::queue<std::function<void()>> _tasks;
    // 保护任务队列和共享状态的互斥锁, mutable修饰表示可以在const成员函数中修改锁的状态
    mutable std::mutex _mtx;
    // 任务通知的条件变量(休眠-唤醒)
    std::condition_variable _cv;
    // 线程池的运行状态(原子量, 无锁同步)
    std::atomic<bool> _is_running;
};
