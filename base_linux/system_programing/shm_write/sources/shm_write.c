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

// 定义ftok的公共路径和项目ID
#define FTOK_PATH "/opt"        // 定义ftok的路径，确保存在
#define FTOK_SHM_PROJ_ID 10     // 自定义共享内存proj_id
#define FTOK_SEM_PROJ_ID 66     // 自定义信号量专属proj_id
#define SHM_SIZE 4096           // 共享内存大小

int main()
{
    int running = 1;
    void *shm = NULL;
    char buffer[BUFSIZ + 1];    // 用于保存输入的文本
    int shmid;
    int semid;                  // 信号量标识符
    key_t shm_key, sem_key;     // 共享内存/信号量的ftok键值

    // ========== 1. 生成共享内存的IPC键值 ==========
    if ((shm_key = ftok(FTOK_PATH, FTOK_SHM_PROJ_ID)) == -1) {
        fprintf(stderr, "ftok for shm failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Generate shm key via ftok: 0x%x\n", shm_key);

    // 创建共享内存
    shmid = shmget(shm_key, SHM_SIZE, 0644 | IPC_CREAT);
    if (shmid == -1) {
        fprintf(stderr, "shmget failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 将共享内存连接到当前进程的地址空间
    shm = shmat(shmid, (void*)0, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, "shmat failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Memory attached at %p\n", shm);

    // ========== 2. 生成信号量的IPC键值 ==========
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

    // ========== 3. 向共享内存写数据 ==========
    while (running) {
        // 读取用户输入
        printf("Enter some text: ");
        if (fgets(buffer, BUFSIZ, stdin) == NULL) {
            fprintf(stderr, "fgets failed\n");
            running = 0;
            continue;
        }

        // 直接拷贝数据到共享内存
        // 注意：strncpy第三个参数是“最多拷贝的字节数”，减1留位置给字符串结束符
        strncpy((char*)shm, buffer, SHM_SIZE - 1);
        // 强制添加字符串结束符，避免内存中无终止符导致读端乱码
        ((char*)shm)[SHM_SIZE - 1] = '\0';

        sem_v(semid);  // 释放信号量，通知读端读取

        // 输入了end，退出循环
        if (strncmp(buffer, "end", 3) == 0) {
            running = 0;
        }
    }

    // ========== 4. 资源释放 ==========
    // 把共享内存从当前进程中分离
    if (shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    sleep(2);
    exit(EXIT_SUCCESS);
}