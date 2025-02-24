cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(TinyShellTest C)

# 启用 GoogleTest
enable_testing()

# 查找 GoogleTest
find_package(GTest REQUIRED)

# 指定测试源文件
set(TEST_SOURCES
    tsh_functionTest.c
    unit_test.cpp
)

# 生成可执行测试文件
add_executable(tsh_test ${TEST_SOURCES})

# 链接 GoogleTest 和 pthread
target_link_libraries(tsh_test PRIVATE GTest::GTest GTest::Main pthread)

# 添加测试
add_test(NAME TinyShellTest COMMAND tsh_test)
