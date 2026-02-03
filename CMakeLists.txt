cmake_minimum_required(VERSION 3.10)

project(model_deploy C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找GoogleTest库（已安装到系统）
find_package(Threads REQUIRED)

include_directories(.)

add_subdirectory(test)