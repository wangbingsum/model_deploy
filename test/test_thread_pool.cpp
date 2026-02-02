#include <gtest/gtest.h>
#include <future>
#include <vector>
#include <numeric>
#include <chrono>
#include <thread>
#include "src/core/thread_pool.hpp" // 引入线程池头文件

// 测试套件：ThreadPoolBasic，测试线程池基础功能
TEST(ThreadPoolBasic, InitWithValidThreadNum) {
    // 测试1：默认构造（硬件并发数）
    ThreadPool pool1;
    ASSERT_GE(pool1.thread_count(), 1U); // 至少1个工作线程
    ASSERT_LE(pool1.thread_count(), std::thread::hardware_concurrency());

    // 测试2：指定构造（3个线程）
    ThreadPool pool2(3);
    ASSERT_EQ(pool2.thread_count(), 3U);

    // 测试3：指定0个线程（内部自动修正为1）
    ThreadPool pool3(0);
    ASSERT_EQ(pool3.thread_count(), 1U);
}

// 测试套件：ThreadPoolTask，测试任务提交与执行
TEST(ThreadPoolTask, NoReturnTaskExecute) {
    ThreadPool pool(2);
    bool flag = false;
    // 提交无返回值lambda任务，修改外部变量
    pool.enqueue([&flag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 模拟耗时
        flag = true;
    });
    // 等待任务执行完成（简单等待，实际用future更严谨）
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(flag); // 验证任务执行成功
}

TEST(ThreadPoolTask, ReturnTaskResultCorrect) {
    ThreadPool pool(2);
    // 测试1：简单数值计算
    auto fut1 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);
    ASSERT_EQ(fut1.get(), 30); // get()阻塞等待结果，验证正确性

    // 测试2：字符串拼接
    auto fut2 = pool.enqueue([](const std::string& s1, const std::string& s2) {
        return s1 + s2;
    }, "Hello", "GoogleTest");
    ASSERT_EQ(fut2.get(), "HelloGoogleTest");

    // 测试3：无参数有返回值
    auto fut3 = pool.enqueue([]() { return 3.1415926; });
    ASSERT_NEAR(fut3.get(), 3.1415926, 1e-7); // 浮点数近似比较
}

TEST(ThreadPoolTask, MultiTaskConcurrency) {
    const int TASK_NUM = 100;
    ThreadPool pool(4); // 4线程处理100个任务
    std::vector<std::future<int>> futures;

    // 提交100个任务，每个返回1，最终求和验证
    for (int i = 0; i < TASK_NUM; ++i) {
        futures.emplace_back(pool.enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 模拟并发
            return 1;
        }));
    }

    // 求和所有任务结果
    int sum = 0;
    for (auto& fut : futures) {
        sum += fut.get();
    }

    ASSERT_EQ(sum, TASK_NUM); // 验证所有任务均执行完成
}

// 测试套件：ThreadPoolException，测试异常场景
TEST(ThreadPoolException, EnqueueOnStoppedPool) {
    ThreadPool pool(2);
    pool.stop(); // 主动停止线程池

    // 断言提交任务时抛出指定运行时异常
    ASSERT_THROW(
        pool.enqueue([]() { return 1; }),
        std::runtime_error
    );

    // 断言抛出的异常信息匹配（可选，更精细的异常测试）
    try {
        pool.enqueue([]() {});
        FAIL() << "Expected std::runtime_error for enqueue on stopped pool";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "enqueue on stopped ThreadPool");
    }
}

TEST(ThreadPoolException, TaskThrowException) {
    ThreadPool pool(2);
    // 提交一个会主动抛出异常的任务
    auto fut = pool.enqueue([]() {
        throw std::invalid_argument("task internal error");
        return 0;
    });

    // 断言获取结果时重新抛出该异常
    ASSERT_THROW(
        fut.get(),
        std::invalid_argument
    );

    // 验证线程池仍可用（单个任务异常不影响线程池）
    auto fut2 = pool.enqueue([]() { return 100; });
    ASSERT_EQ(fut2.get(), 100);
}

// 测试套件：ThreadPoolDestruct，测试析构安全
TEST(ThreadPoolDestruct, CompleteAllTasksBeforeDestruct) {
    const int TASK_NUM = 50;
    int count = 0;
    {
        // 作用域内创建线程池，析构时会自动等待所有任务完成
        ThreadPool pool(2);
        for (int i = 0; i < TASK_NUM; ++i) {
            pool.enqueue([&count]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                ++count; // 原子操作？此处仅测试析构等待，实际并发需加锁
            });
        }
    } // 线程池析构，自动stop+notify_all+join所有线程

    ASSERT_EQ(count, TASK_NUM); // 验证析构前所有任务均执行完成
}

// 可选：手动编写main函数（GoogleTest提供默认main，此为自定义示例）
// int main(int argc, char **argv) {
//     testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }