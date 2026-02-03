#include "src/core/thread_pool.hpp"  // Include thread pool header file (adjust path as needed)
#include <iostream>
#include <vector>
#include <mutex>   // Necessary for std::mutex (fixed missing header)
#include <cassert>
#include <chrono>
#include <string>
#include <future>
#include <thread>

// Test 1: Execute simple tasks with no return value (verify basic task scheduling)
void test_simple_task(ThreadPool& pool) {
    std::cout << "=== Test 1: Simple Task Without Return Value ===" << std::endl;
    int count = 0;
    std::mutex mtx;  // Protect shared variable count

    // Submit 10 simple tasks, each increments count by 1
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([&count, &mtx]() {
            std::lock_guard<std::mutex> lock(mtx);
            count++;
        });
    }

    // Wait for all tasks to complete (simple delay, use future in actual projects)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Assert: count should be 10 (all tasks executed)
    assert(count == 10 && "Test 1 Failed: Mismatched number of executed simple tasks");
    std::cout << "Test 1 Succeeded: count = " << count << std::endl << std::endl;
}

// Test 2: Execute tasks with return value (verify std::future result acquisition)
void test_return_task(ThreadPool& pool) {
    std::cout << "=== Test 2: Task With Return Value (Get Result via Future) ===" << std::endl;
    // Submit addition task, return sum of two numbers
    auto fut1 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);
    // Submit multiplication task, return product of two numbers
    auto fut2 = pool.enqueue([](double a, double b) { return a * b; }, 3.14, 2.0);
    // Submit string concatenation task
    auto fut3 = pool.enqueue([](const std::string& s1, const std::string& s2) { return s1 + s2; }, "Hello", " ThreadPool");

    // Get task results (future.get() blocks until task completion)
    int res1 = fut1.get();
    double res2 = fut2.get();
    std::string res3 = fut3.get();

    // Assert result correctness
    assert(res1 == 30 && "Test 2 Failed: Incorrect result of addition task");
    assert(res2 == 6.28 && "Test 2 Failed: Incorrect result of multiplication task");
    assert(res3 == "Hello ThreadPool" && "Test 2 Failed: Incorrect result of string concatenation task");

    std::cout << "Test 2 Succeeded: " << std::endl;
    std::cout << "  10+20 = " << res1 << std::endl;
    std::cout << "  3.14*2.0 = " << res2 << std::endl;
    std::cout << "  String Concatenation = " << res3 << std::endl << std::endl;
}

// Test 3: Mass concurrent tasks (verify thread pool concurrency and queue scheduling)
void test_concurrent_tasks(ThreadPool& pool) {
    std::cout << "=== Test 3: 1000 Concurrent Calculation Tasks ===" << std::endl;
    const int TASK_NUM = 1000;
    std::vector<std::future<int>> futures;
    futures.reserve(TASK_NUM);

    // Submit 1000 calculation tasks: each calculates i*i (i from 0 to 999)
    for (int i = 0; i < TASK_NUM; ++i) {
        futures.emplace_back(pool.enqueue([i]() {
            // Simulate slight calculation delay to detect scheduling logic
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            return i * i;
        }));
    }

    // Summarize all task results and verify correctness
    int sum = 0;
    int expected_sum = 0;
    for (int i = 0; i < TASK_NUM; ++i) {
        sum += futures[i].get();
        expected_sum += i * i;  // Calculate expected result
    }

    // Assert: actual sum should equal expected sum
    assert(sum == expected_sum && "Test 3 Failed: Incorrect summary of concurrent task results");
    std::cout << "Test 3 Succeeded: 1000 concurrent tasks executed, total sum = " << sum << std::endl << std::endl;
}

// Test 4: Forbid task submission after thread pool stops (verify exception throwing)
void test_stop_enqueue(ThreadPool& pool) {
    std::cout << "=== Test 4: Task Submission After Pool Stop (Verify Exception) ===" << std::endl;
    // Actively stop the thread pool
    pool.stop();
    // Wait for the thread pool to enter stop state
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Attempt to submit task, expect std::runtime_error to be thrown
    bool catch_exception = false;
    try {
        pool.enqueue([]() { std::cout << "This task should not be executed" << std::endl; });
    } catch (const std::runtime_error& e) {
        catch_exception = true;
        std::cout << "Expected exception caught: " << e.what() << std::endl;
    } catch (...) {
        catch_exception = true;
        std::cout << "Unknown exception caught" << std::endl;
    }

    // Assert: exception must be caught
    assert(catch_exception && "Test 4 Failed: No exception thrown when submitting task to stopped pool");
    std::cout << "Test 4 Succeeded" << std::endl << std::endl;
}

// Test 5: Verify unexecuted task handling on pool destruction (no assert, verify no crash)
void test_destructor_safety() {
    std::cout << "=== Test 5: Pool Destruction Safety (Mass Unexecuted Tasks) ===" << std::endl;
    // Create a temporary thread pool, submit a large number of tasks then destroy it
    {
        ThreadPool pool(4);  // 4 worker threads
        // Submit 500 tasks without getting future (simulate destruction with unexecuted tasks)
        for (int i = 0; i < 500; ++i) {
            pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                // Print a small number of tasks to verify partial execution before destruction
                if (i % 100 == 0) {
                    static std::mutex print_mtx; // Avoid cout disorder in multi-threading
                    std::lock_guard<std::mutex> lock(print_mtx);
                    std::cout << "  Task executed before destruction: i = " << i << std::endl;
                }
            });
        }
        // Scope ends, pool destructs: auto stop() + notify all threads + wait for exit
    }

    std::cout << "Test 5 Succeeded: Pool destroyed without crash, unexecuted tasks cleaned safely" << std::endl;
}

int main() {
    try {
        // Create thread pool: use hardware core count by default, or specify manually (e.g., ThreadPool pool(4))
        ThreadPool pool;
        std::cout << "Thread pool init finished, worker num: " << pool.thread_count() << std::endl << std::endl;

        // Execute all tests
        // test_simple_task(pool);
        test_return_task(pool);
        test_concurrent_tasks(pool);
        test_stop_enqueue(pool);

        // Test 5 executed independently (temporary thread pool)
        test_destructor_safety();

        std::cout << "=============================================" << std::endl;
        std::cout << "✅ All test cases executed successfully!" << std::endl;
        std::cout << "=============================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Uncaught exception during testing: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Uncaught unknown exception during testing" << std::endl;
        return 1;
    }

    return 0;
}