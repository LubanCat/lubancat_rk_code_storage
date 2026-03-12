#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// 定义子进程任务类型枚举
typedef enum {
    TASK_LS,       // 执行ls -l命令
    TASK_PWD,      // 执行pwd命令
    TASK_CUSTOM    // 自定义计算任务
} TaskType;

// 子进程任务信息结构体，用于父进程管理
typedef struct {
    pid_t pid;             // 子进程PID
    TaskType type;         // 任务类型
    char desc[32];         // 任务描述
    int is_recycled;       // 是否已被回收，0：未回收，1：已回收
} ChildTask;

// 自定义任务：计算1~100的和并写入文件
void custom_child_task() {
    printf("子进程PID=%d 开始执行自定义任务：计算1~100的和并写入文件\n", getpid());
    
    // 计算1~100的和
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    
    // 打开/创建文件，写入计算结果
    int fd = open("custom_task_result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("自定义任务：文件打开失败");
        _exit(EXIT_FAILURE);
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "子进程PID=%d | 1~100的和 = %d\n", getpid(), sum);
    ssize_t write_len = write(fd, buf, strlen(buf));
    if (write_len < 0) {
        perror("自定义任务：文件写入失败");
        close(fd);
        _exit(EXIT_FAILURE);
    }
    
    close(fd);
    printf("子进程PID=%d 自定义任务执行完成，结果已写入custom_task_result.txt\n", getpid());
    _exit(EXIT_SUCCESS); // 自定义任务正常退出
}

int main() {
    // 定义3个子进程的任务列表
    ChildTask tasks[] = {
        {0, TASK_LS, "执行ls -l命令", 0},
        {0, TASK_PWD, "执行pwd命令", 0},
        {0, TASK_CUSTOM, "计算1~100的和", 0}
    };
    int task_count = sizeof(tasks) / sizeof(tasks[0]); // 子进程数量
    int status;
    int recycled_count = 0; // 已回收的子进程数

    // 父进程调整优先级，nice值+5，降低优先级
    if (nice(5) == -1) {
        perror("父进程优先级调整失败");
    }
    printf("父进程PID=%d 优先级调整完成，开始创建%d个子进程\n", getpid(), task_count);

    // 循环创建多个子进程
    for (int i = 0; i < task_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork创建子进程失败");
            // 某个子进程创建失败，继续创建剩余子进程
            continue;
        }

        if (pid == 0) { // 子进程逻辑
            printf("子进程PID=%d 开始执行任务：%s\n", getpid(), tasks[i].desc);
            
            // 根据任务类型执行不同逻辑
            switch (tasks[i].type) {
                case TASK_LS:
                    // 程序替换：执行ls -l，execlp自动搜索PATH
                    execlp("ls", "ls", "-l", NULL);
                    // execlp执行成功不会返回，以下仅失败时执行
                    perror("子进程ls命令执行失败");
                    _exit(EXIT_FAILURE);
                    
                case TASK_PWD:
                    // 程序替换：执行pwd
                    execlp("pwd", "pwd", NULL);
                    perror("子进程pwd命令执行失败");
                    _exit(EXIT_FAILURE);
                    
                case TASK_CUSTOM:
                    // 执行自定义任务
                    custom_child_task();
                    break;
                    
                default:
                    fprintf(stderr, "子进程PID=%d 无效任务类型\n", getpid());
                    _exit(EXIT_FAILURE);
            }
        } else { // 父进程记录子进程信息
            tasks[i].pid = pid;
            printf("父进程已创建子进程PID=%d，任务：%s\n", pid, tasks[i].desc);
        }
    }

    // 父进程非阻塞循环回收所有子进程
    printf("\n父进程开始非阻塞回收子进程...\n");
    while (recycled_count < task_count) {
        // 遍历所有子进程，逐个非阻塞检查是否退出
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].pid <= 0 || tasks[i].is_recycled) {
                continue; // 跳过创建失败或已回收的子进程
            }

            // WNOHANG：非阻塞模式，子进程未退出则立即返回0
            pid_t ret = waitpid(tasks[i].pid, &status, WNOHANG);
            if (ret < 0) { // waitpid调用失败
                fprintf(stderr, "waitpid回收子进程PID=%d失败：%s\n", 
                        tasks[i].pid, strerror(errno));
                tasks[i].is_recycled = 1;
                recycled_count++;
            } else if (ret > 0) { // 成功回收子进程
                tasks[i].is_recycled = 1;
                recycled_count++;
                
                // 解析子进程退出状态
                if (WIFEXITED(status)) {
                    printf("回收子进程PID=%d（任务：%s），正常退出，退出码：%d\n",
                           tasks[i].pid, tasks[i].desc, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("回收子进程PID=%d（任务：%s），被信号终止，信号编号：%d\n",
                           tasks[i].pid, tasks[i].desc, WTERMSIG(status));
                }
            }
            // ret == 0：子进程未退出，不处理
        }

        // 模拟父进程等待期间执行其他任务，每500ms轮询一次
        printf("父进程等待中，已回收%d/%d个子进程...\n",
               recycled_count, task_count);
        usleep(500000); // 休眠500毫秒
    }

    printf("\n父进程已回收所有子进程，程序执行完毕\n");
    return 0;
}