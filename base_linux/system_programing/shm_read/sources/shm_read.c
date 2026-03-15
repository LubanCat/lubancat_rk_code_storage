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

// 定义ftok公共路径和不同项目ID
#define FTOK_PATH "/opt"        // 定义ftok的路径，确保存在
#define FTOK_SHM_PROJ_ID 10     // 自定义共享内存proj_id
#define FTOK_SEM_PROJ_ID 66     // 自定义信号量专属proj_id
#define SHM_SIZE 4096           // 共享内存大小

int main(void)
{
    int running = 1;            // 程序是否继续运行的标志
    char *shm = NULL;           // 分配的共享内存的原始首地址
    int shmid;                  // 共享内存标识符
    int semid;                  // 信号量标识符
    key_t shm_key, sem_key;     // ftok生成的共享内存/信号量键值

    // ========== 1. 生成共享内存IPC键值 ==========
    if ((shm_key = ftok(FTOK_PATH, FTOK_SHM_PROJ_ID)) == -1) {
        fprintf(stderr, "ftok for shm failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Generate shm key via ftok: 0x%x\n", shm_key);

    // 创建/获取共享内存
    shmid = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        fprintf(stderr, "shmget failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 将共享内存连接到当前进程的地址空间
    shm = shmat(shmid, 0, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, "shmat failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("\nMemory attached at %p\n", shm);

    // ========== 2. 生成信号量IPC键值 ==========
    if ((sem_key = ftok(FTOK_PATH, FTOK_SEM_PROJ_ID)) == -1) {
        fprintf(stderr, "ftok for sem failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Generate sem key via ftok: 0x%x\n", sem_key);

    // 打开/创建信号量
    semid = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        fprintf(stderr, "semget failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    init_sem(semid, 0);  // 初始化信号量

    // ========== 3. 读取共享内存数据 ==========
    while (running) {
        // 等待信号量（P操作：写端释放后才读取）
        if (sem_p(semid) == 0) {
            printf("You wrote: %s", shm);
            sleep(rand() % 3);  // 模拟处理延迟
            
            // 输入了end，退出循环
            if (strncmp(shm, "end", 3) == 0) {
                running = 0;
            }
        }
    }

    // ========== 4. 资源清理 ==========
    del_sem(semid);  /** 删除信号量 */

    // 把共享内存从当前进程中分离
    if (shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // 删除共享内存
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Shared memory and semaphore deleted successfully.\n");
    exit(EXIT_SUCCESS);
}