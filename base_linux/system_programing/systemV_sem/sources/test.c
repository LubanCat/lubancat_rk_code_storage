#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "sem.h"

#define DELAY_TIME 3        // 为了突出演示效果，等待几秒钟
#define FTOK_PATH "/opt"    // 目录确保存在，避免ftok失败
#define FTOK_PROJ_ID 66     // 自定义项目ID

int main(void)
{
    pid_t result;
    int sem_id;
    key_t sem_key;  // 存储ftok生成的信号量键值

    /* 1. 用ftok生成唯一的IPC键值 */
    if ((sem_key = ftok(FTOK_PATH, FTOK_PROJ_ID)) == -1)
    {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }
    printf("Generate sem key via ftok: 0x%x\n", sem_key);  // 打印生成的键值

    /* 2. 创建/获取信号量 */
    sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (sem_id == -1)
    {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    init_sem(sem_id, 0);

    // 调用fork()函数
    result = fork();
    if(result == -1)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (result == 0) //返回值为0代表子进程
    {
        printf("Child process will wait for some seconds...\n");
        sleep(DELAY_TIME);
        printf("The returned value is %d in the child process(PID = %d)\n", result, getpid());

        sem_v(sem_id);  // V操作：释放信号量
    }
    else // 返回值大于0代表父进程
    {
        sem_p(sem_id);  // P操作：获取信号量，子进程未释放前会阻塞
        printf("The returned value is %d in the father process(PID = %d)\n", result, getpid());

        sem_v(sem_id);  // V操作：释放信号量
        del_sem(sem_id); // 删除信号量
    }

    exit(EXIT_SUCCESS);
}