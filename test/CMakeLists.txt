add_executable(test_sort test_sort.cpp)

install(
    TARGETS test_sort  # 对应add_executable的可执行文件名，必须一致！
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin  # 安装路径：项目根目录/bin
)