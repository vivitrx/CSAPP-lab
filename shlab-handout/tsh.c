/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* 杂项常量定义 */
#define MAXLINE    1024   /* 最大行长度 */
#define MAXARGS     128   /* 每行命令的最大参数个数 */
#define MAXJOBS      16   /* 同时最多运行的作业数 */
#define MAXJID    1<<16   /* 最大作业 ID */

/* 作业状态 */
#define UNDEF 0 /* 未定义 */
#define FG 1    /* 在前台运行 */
#define BG 2    /* 在后台运行 */
#define ST 3    /* 已停止 */

/* 
 * 作业状态：FG（前台）、BG（后台）、ST（已停止）
 * 作业状态转移及触发条件：
 *     FG -> ST  : ctrl-z（停止）
 *     ST -> FG  : fg 命令（前台运行）
 *     ST -> BG  : bg 命令（后台运行）
 *     BG -> FG  : fg 命令（前台运行）
 * 最多只能有一个作业处于前台（FG）状态。
 */

/* 全局变量 */
extern char **environ;      /* 在 libc 中定义 */
char prompt[] = "tsh> ";    /* 命令行提示符（请勿更改） */
int verbose = 0;            /* 如果为真，则打印额外的输出 */
int nextjid = 1;            /* 下一个分配的作业 ID */
char sbuf[MAXLINE];         /* 用于构造 sprintf 消息 */

struct job_t {              /* 作业结构体 */
    pid_t pid;              /* 作业的 PID */
    int jid;                /* 作业 ID [1, 2, ...] */
    int state;              /* 作业状态：UNDEF, BG, FG, 或 ST */
    char cmdline[MAXLINE];  /* 命令行 */
};
struct job_t jobs[MAXJOBS]; /* 作业列表 */
/* 结束全局变量 */
void unix_error(char *msg); /* Unix-style error */
/* Error wrapper function */
pid_t Fork(void);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Execve(const char *filename, char *const argv[], char *const envp[]);
void Setpgid(pid_t pid, pid_t pgid);
void Kill(pid_t pid, int sig);
pid_t Fork(void){
    pid_t pid;
    if((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset){
    if(sigprocmask(how, set, oldset) < 0)
        unix_error("Sigprocmask error");
}
void Sigemptyset(sigset_t *set){
    if(sigemptyset(set) < 0)
        unix_error("Sigprocmask error");
}
void Sigfillset(sigset_t *set){
    if(sigfillset(set) < 0)
        unix_error("Sigfillset error");
}
void Sigaddset(sigset_t *set, int signum){
    if(sigaddset(set, signum) < 0)
        unix_error("Sigaddset error");
}
void Execve(const char *filename, char *const argv[], char *const envp[]){
    if(execve(filename, argv, envp) < 0){
        printf("%s: Command not found.\n", argv[0]);
    }
}
void Setpgid(pid_t pid, pid_t pgid){
    if(setpgid(pid, pgid) < 0){
        unix_error("Setpid error");
    }
}
void Kill(pid_t pid, int sig){
    if(kill(pid, sig) < 0){
        unix_error("Kill error");
    }
}

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline); // √
int builtin_cmd(char **argv); // √
void do_bgfg(char **argv); // √
void waitfg(pid_t pid); // √

void sigchld_handler(int sig);
void sigtstp_handler(int sig); // √
void sigint_handler(int sig); // √

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
* eval - 评估用户刚输入的命令行
* 
* 如果用户请求了一个内建命令（如 quit、jobs、bg 或 fg）
* 则立即执行它。否则，创建一个子进程，并在子进程中运行该任务。
* 如果任务在前台运行，则等待它结束后再返回。
*
* 注意：每个子进程必须有一个唯一的进程组 ID，以便我们后台的子进程
* 在我们按下 ctrl-c（或 ctrl-z）时不会接收到来自内核的 SIGINT（中断信号）
* 或 SIGTSTP（暂停信号）。 
*/
void eval(char *cmdline) 
{  
    char* argv[MAXARGS];
    int job_type = parseline(cmdline, argv) ? BG : FG;
    pid_t pid; 
  
    sigset_t mask_one, empty_set;
    Sigemptyset(&empty_set);
    Sigemptyset(&mask_one); Sigaddset(&mask_one, SIGCHLD);
    if (!builtin_cmd(argv))
    {
        Sigprocmask(SIG_BLOCK, &mask_one, NULL); // 屏蔽SIGCHLD信号，至于为什么只屏蔽SIGCHLD而不屏蔽其他信号？因为你希望在创建子进程的时候还能响应其他信号，比如说，我们当前在写的作业--the tiny shell，tsh，是你的父进程shell创建的（无论你用的是bash还是dash），也就是说bash对你的tsh是有完全的控制权的，bash想杀tsh就杀tsh（只需要简单调用一个kill()就能发送SIGTEM信号），但是如果你在此处屏蔽了所有的信号，那么bash就不能想杀tsh就杀tsh了，这显然是不对的
        if((pid = fork()) == 0){
            Sigprocmask(SIG_SETMASK, &empty_set, NULL); // 对子进程解除信号屏蔽，子进程会继承父进程的信号屏蔽状态，若子进程还要管理孙进程，那么你最好不要让子进程继承父进程的信号屏蔽状态（这会影响子进程对孙进程的管理）
            setpgid(0,0);
            Execve(argv[0], argv, environ);
            exit(0);
        }
    }
    if (job_type == FG )
    {
        if (pid != 0)
        {
            addjob(jobs, pid, job_type, cmdline);
            waitfg(pid); // 在调用waitfg()之前，我们确保已经屏蔽了SIGCHLD信号（if((pid = fork()) == 0)的上一行），若没有屏蔽会有产生竞态的可能
        }
    }else{
        addjob(jobs, pid, job_type, cmdline);
        printf("[%i] (%i) %s", pid2jid(pid), pid, cmdline);
    }
    Sigprocmask(SIG_SETMASK, &empty_set, NULL); // 解除父进程的信号屏蔽
    return;
}

/* 注:argv[]：传递给新程序的参数数组。
   这个数组的第一个元素通常是程序的名称（例如：/bin/ls 或 ls），其余元素是传递给程序的参数。
   argv数组存储的是用户输入的命令，比如说用户输入/bin/echo 'Hello World'
   那么argv存储的就是:
   argv[0] = "/bin/echo";
   argv[1] = "Hello World";
   argv[2] = NULL;
 * parseline - 解析命令行并构建 argv 数组。
 * 
 * 被单引号括起来的字符会被当作一个整体参数处理。
 * 如果用户请求的是后台任务，返回 true；如果用户请求的是前台任务，返回 false。
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
  
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (strcmp(argv[0], "quit") == 0)
    {
        exit(0);
    }
    if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0)
    {
       do_bgfg(argv);
       return 1;
    }
    if (strcmp(argv[0], "jobs") == 0)
    {
        listjobs(jobs);
        return 1;
    }
    if (strcmp(argv[0], "&") == 0)
    {
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
    输入时后面的参数有%则代表jid，没有则代表pid
    bg <job>：向指定作业发送 SIGCONT 信号，并让它在后台运行。<job> 可以是 PID 或 JID。
    fg <job>：向指定作业发送 SIGCONT 信号，并让它在前台运行。<job> 可以是 PID 或 JID。
    作业指的是进程组
 */
void do_bgfg(char **argv) 
{
    // 解析参数（输入是fg还是bg？查询的是job还是process？
    // 查询的哪个job？查询的是哪个进程？进程存在吗？job存在吗？）
    int jid;
    struct job_t* target_job = NULL; 
    pid_t target_pid;

    // 若用户输入了命令但是没输入job id或process id
    if (argv[1] == NULL)
    {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    if (!isdigit(argv[1][0]) && argv[1][0] != '%') // 若用户输入非法，比如说fg a
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    // 判断用户是传入的 %jobid 还是 pid
    if (argv[1][0] == '%')
    {
        // 说明输入的是job id，你只能获取到jid
        if (sscanf(&argv[1][1], "%d", &jid) > 0)
        {
            target_job = getjobjid(jobs, jid);
            // 若所请求的job不存在
            if (target_job == NULL)
            {
                printf("%%%i: No such job\n", jid);
                return;
            }
        }
    }
    else
    {
        // 说明输入的是process id
        target_pid = atoi(argv[1]); 
        target_job = getjobpid(jobs, target_pid);
        // 若所请求的进程不存在
        if (target_job == NULL)
        {
            printf("(%i): noooo such process\n", target_pid);
            return;
        }
    }

    // 正常的情况，fg和bg的区别仅在于fg会调用waitfg()强制终端等待任务的完成
    Kill(-target_job->pid, SIGCONT);
    if (strcmp(argv[0], "fg") == 0)
    {
        target_job->state = FG;
        waitfg(target_pid);
    }
    else
    {
        target_job->state = BG;
        // 打印目标job信息;
        printf("[%d] (%d) %s", target_job->jid, target_job->pid, target_job->cmdline);
    }
    return;
}
/* 
 * waitfg - Block until process pid is no longer the foreground process
    阻塞父进程，直到进程号为pid的进程不再为前台进程。
    注意，在调用该函数之前，请确保调用的进程已经屏蔽了SIGCHLD信号
 */
void waitfg(pid_t pid)
{
    sigset_t emptyset; Sigemptyset(&emptyset);
    while (fgpid(jobs) != 0)
    {
        sigsuspend(&emptyset);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - 
 * 当一个子进程终止（变成僵尸进程）或者因为接收到 SIGSTOP 或 SIGTSTP 信号而停止时，
 * 内核会向shell发送 SIGCHLD 信号。
 * (意思是正常执行完流程的进程和接收到 SIGSTOP和SIGTSTP 的流程都会发出SIGCHLD信号，这三种情况你都要handle到，不过显然不能一棍子全部回收)
 * 该处理程序会清理所有已终止的僵尸子进程，但不会等待任何其他当前正在运行的子进程终止。
 * (意思是因为收到 SIGSTOP 或 SIGTSTP 信号而停止的进程，该进程不应该被该函数回收，该函数只应该回收终止的进程)
 */
void sigchld_handler(int sig) 
{
    // 找出所有的正常终止的进程(僵尸进程)并清理（使用deletejob()从job list上去除）
    // 对于接收到SIGTSTP的进程b，操作系统内核的意图是将其暂停，操作系统使用PCB数据结构管理进程，因此你应该将进程a的PCB中的进程状态变量置为ST而非直接deletejob()
    // 对于接收到SIGSTOP的进程a，操作系统内核的意图是将其终止，因此你应该将进程b直接回收（直接使用deletejob()）
    int status;
    int cur_pid;
    int old_errno = errno; // 如果waitpid()失败，那么waitpid()会返回-1，并将errno置为ECHILD或EINTR，方便你排查
    while ((cur_pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) // WNOHANG是指那些运行完正常终止的进程
    {
        if (WIFEXITED(status)) // 这里的status是cur_pid的status，可以通过他来判断cur_pid的状态
        {
            deletejob(jobs, cur_pid);
        }else if (WIFSTOPPED(status)) // 该进程因为接收到了SIGTSTP信号而暂停
        {
            getjobpid(jobs, cur_pid)->state = ST;
            printf ("Job [%d] (%d) terminated by signal %d\n", pid2jid(cur_pid), cur_pid, WTERMSIG(status));
        }else if (WIFSIGNALED(status)) // 该进程因为接受到了SIGSTOP信号而终止
        {
            deletejob(jobs, cur_pid);
            printf ("Job [%d] (%d) terminated by signal %d\n", pid2jid(cur_pid), cur_pid, WTERMSIG(status));
        }
    }
    errno = old_errno;
    return;
}

/* 
 * sigint_handler - 当用户在键盘上按下 ctrl-c 时，内核会向 shell 发送一个 SIGINT 信号。
 *    捕获该信号并将其传递给前台作业。
 */
void sigint_handler(int sig) 
{
    // 找到前台作业
    pid_t fg = fgpid(jobs);
    if (fg == 0)
    {
        return;
    }else{
        // 对前台作业所属的进程组发送SIGINT
        kill(-fg, sig);
    }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    pid_t fg = fgpid(jobs); // 找到前台作业
    if (fg == 0)
    {
        return;
    }else{
        kill(-fg, sig); // 对前台作业所属的进程组发送SIGTSTP
    }
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
/* 参数解释
   jobs：你要往哪个任务列表添加任务？
   pid：你要添加的任务的pid是多少？
   state：这个任务是一个前台任务还是一个后台任务？
   cmdline：这个任务的名字叫什么？
*/ 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
  
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
  
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}