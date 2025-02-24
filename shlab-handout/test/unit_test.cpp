#include <gtest/gtest.h>
extern "C" {
    #include "tsh.c"  // 引入 tsh.c 中的头文件
}

// 测试 eval 函数
TEST(EvalTest, BasicCommand) {
    char cmdline[] = "ls";
    eval(cmdline);  // 测试 ls 命令是否能够正常执行
}

// 测试空命令的行为
TEST(EvalTest, EmptyCommand) {
    char cmdline[] = "ls &";
    eval(cmdline);  // 测试空命令是否被正确处理（例如：无操作）
}
