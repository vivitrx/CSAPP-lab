cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(TshTest C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)

# 找到 Google Test
find_package(GTest REQUIRED)

# 包含 GTest 头文件
include_directories(${GTEST_INCLUDE_DIRS})

# 定义你的测试目标
add_executable(tsh_test unit_test.cpp tsh_functionTest.c)

# 链接 GTest 和 pthread（必要时）
target_link_libraries(tsh_test ${GTEST_LIBRARIES} pthread)
