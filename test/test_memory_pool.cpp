#include <iostream>
#include <string>
#include "src/core/memory_pool.hpp"

struct TestObj {
    int a;
    std::string b;
    TestObj(int a_, std::string b_) : a(a_), b(std::move(b_)) {
        std::cout << "TestObj constructed: a=" << a << ", b=" << b << std::endl;
    }

    ~TestObj() {
        std::cout << "TestObj destructed: a=" << a << std::endl;
    }
};

int main() {
    try {
        MemoryPool<TestObj, 64> pool;

        // 1. 测试构造/销毁
        TestObj* obj1 = pool.construct(10, "hello pool");
        TestObj* obj2 = pool.construct(20, std::string("c++17 memory pool"));

        // 2.测试直接分配释放
        TestObj* obj3 = pool.allocate();
        new(obj3) TestObj(30, "direct allocate");
        obj3->~TestObj();
        pool.deallocate(obj3);

        pool.destory(obj1);
        pool.destory(obj2);

        std::cout << "All operations completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
