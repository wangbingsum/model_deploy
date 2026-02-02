add_executable(test_sort test_sort.cpp)

add_executable(test_thread_pool test_thread_pool.cpp)
# 链接库：GoogleTest+线程库
target_link_libraries(test_thread_pool
    GTest::GTest
    GTest::Main # GoogleTest默认main函数
    Threads::Threads
    -Wl,-rpath,/usr/lib32
    -Wl,-rpath,/usr/local/lib64
    -Wl,-rpath,/lib/x86_64-linux-gnu
)

install(
    TARGETS test_sort test_thread_pool
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin  # 安装路径：项目根目录/bin
)
