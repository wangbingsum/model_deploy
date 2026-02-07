#include <iostream>
#include <set>
#include <string>
#include "src/core/stop_watch.hpp"
#include "src/core/scoped_timer.hpp"
using namespace std;

// 测试移动语义
void test_01() {
    class Bagel {
    public:
        Bagel(std::string name, const std::set<std::string>& ts)
            // name是值传递,如果是传的左值,传参会执行一次拷贝构造,赋值给成员变量会执行一次移动构造, 传传右值,则会执行两次移动构造
            : _name(std::move(name))
            // 这里会做一个拷贝构造
            , _toppings(ts) {}

        void print() {
            cout << _name << ": ";
            for(auto& item : _toppings) {
                cout << item << " ";
            }
            cout << endl;
        }
    private:
        std::string _name;
        std::set<std::string> _toppings; 
    };

    auto t = std::set<std::string>{};
    t.insert("salt");
    cout << t.size() << endl;
    
    auto a = Bagel("a", t);
    a.print();

    t.insert("pepper");
    cout << t.size() << endl;
    auto b = Bagel("b", t);
    a.print();
    b.print();

    t.insert("Oregano");
    cout << t.size() << endl;
    a.print();
    b.print();

    cout << "finished" << endl;
}

// 测试构造函数
void test_02() {
    class Foo {
    public:
        Foo() {
            cout << "Foo()" << endl;
        };
        Foo(const Foo&) {
            cout << "Foo(const Foo& f)" << endl;
        }
        Foo& operator=(const Foo&) {
            cout << "Foo& operator=(const Foo&)" << endl;
            return *this;
        }
        Foo(Foo&&) {
            cout << "Foo(Foo&&)" <<endl;
        }
        Foo& operator=(Foo&&) {
            cout << "Foo& operator=(Foo&&)" << endl;
            return *this;
        }
    };
    // 这两个是等效的, 都只会调用Foo()这个基本的构造函数
    Foo x1{};
    // 编译器优化后等效于Foo x{}
    auto x2 = Foo();
    // Foo(const Foo& f)
    Foo x3(x2);
    // Foo(Foo&&)
    Foo x4(std::move(x3));
    // Foo()
    Foo x5;
    // Foo& operator=(const Foo&)
    x5 = x4;
    // Foo& operator=(Foo&&)
    x5 = std::move(x4);
    // Foo(Foo&&)
    Foo x6 = std::move(x5);
}

// 测试计时器
void test_03() {
    {
        Stopwatch watch;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    {
        ScopedTimer timer("sleep");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    // test_01()
    // test_02();
    test_03();
}
