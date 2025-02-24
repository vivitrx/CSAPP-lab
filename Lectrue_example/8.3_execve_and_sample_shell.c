#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

// 定义子进程的数量
#define N 2

int main() {
    int status, i;
    pid_t pid[N], retpid;

    // 父进程创建 N 个子进程
    for (i = 0; i < N; i++) {
        if ((pid[i] = fork()) == 0) {
            // 子进程执行的代码
            exit(100 + i);  // 每个子进程有不同的退出状态
        }
    }

    // 父进程依次回收 N 个子进程
    i = 0;
    while ((retpid = waitpid(pid[i++], &status, 0)) > 0) {
        if (WIFEXITED(status)) {
            // 子进程正常退出
            printf("child %d terminated normally with exit status=%d\n", retpid, WEXITSTATUS(status));
        } else {
            // 子进程异常退出
            printf("child %d terminated abnormally\n", retpid);
        }

    for (i = 0; i < N+1; i++) {
      printf("pid[%d] = %d\n", i, pid[i]);
    }

        
        // 如果没有更多的子进程，说明正常终止
        if (errno != ECHILD) {
            perror("waitpid error");
            exit(1);
        }
    }
    return 0;
}
