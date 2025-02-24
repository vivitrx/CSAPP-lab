#include "tsh_functionTest.h"
// 辅助函数
int count_argv(char *argv[]) {
    int count = 0;
    while (argv[count] != NULL) {
        count++;
    }
    return count;
}

void eval(char *cmdline) 
{
    if (builtin_cmd(&cmdline) != 0)
    {
        ; // 内建命令被集成在shell的源代码里，也就是说你可以直接从shell的内存映像里找到内建命令的代码并直接执行，此时也不需要waitpid什么的，你实际上是跳转shell进程的某个地方开始执行
    }else{
        pid_t cur_pid = fork();
        if (cur_pid == 0) // 是子进程
        {
            char *argv[MAXLINE]; char *envp[] = {NULL};
            parseline(cmdline, argv);
            setpgid(0, 0); // 设置子进程的pgid为pid
            execve(argv[0], argv, envp);
            exit(0);
        }else // 父进程行为
        {
            char *argv[MAXLINE];
            parseline(cmdline, argv);
            if (*argv[count_argv(argv) - 1] != '&') // cmdline在前台运行,也就是说输入中没有检测到'&'
            {
                int status;
                waitpid(cur_pid, &status, 0);
                return;
            }
        }
    }    
    return;
}

int main(){
  return 1;
}