add_executable(test_sort test_sort.cpp)

add_executable(test_thread_pool test_thread_pool.cpp)
# 链接库：GoogleTest+线程库
target_link_libraries(test_thread_pool
    Threads::Threads
)

add_executable(test_memory_pool test_memory_pool.cpp)

install(
    TARGETS test_sort test_thread_pool test_memory_pool
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin  # 安装路径：项目根目录/bin
)
