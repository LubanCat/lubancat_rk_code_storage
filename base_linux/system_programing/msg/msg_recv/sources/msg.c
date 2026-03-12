#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 512
#define FTOK_PATH "/opt"    // 定义ftok的路径，发送和接收需一致，即inode号一致
#define FTOK_PROJ_ID 100  // 自定义项目标识

struct message
{
    long msg_type;
    char msg_text[BUFFER_SIZE];
};

int main()
{
    int qid;
    key_t key;  // 存储ftok生成的键值
    struct message msg;

    /* 1. 用ftok生成IPC键值 */
    if ((key = ftok(FTOK_PATH, FTOK_PROJ_ID)) == -1)
    {
        perror("ftok failed");
        exit(1);
    }
    printf("Generate key via ftok: %x\n", key);  // 打印生成的键值

    /* 2. 创建/获取消息队列 */
    if ((qid = msgget(key, IPC_CREAT|0666)) == -1)
    {
        perror("msgget failed");
        exit(1);
    }

    printf("Open queue %d\n", qid);

    do
    {
        /* 读取消息队列 */
        memset(msg.msg_text, 0, BUFFER_SIZE);

        if (msgrcv(qid, (void*)&msg, BUFFER_SIZE, 0, 0) < 0)
        {
            perror("msgrcv failed");
            exit(1);
        }

        printf("The message from process %ld : %s", msg.msg_type, msg.msg_text);

    } while(strncmp(msg.msg_text, "quit", 4));

    /* 从系统内核中删除消息队列 */
    if ((msgctl(qid, IPC_RMID, NULL)) < 0)
    {
        perror("msgctl failed");
        exit(1);
    }
    else
    {
        printf("Delete msg qid: %d.\n", qid);
    }

    exit(0);
}