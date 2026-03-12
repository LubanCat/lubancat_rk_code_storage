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
    key_t key;
    struct message msg;

    /* 1. 用ftok生成和接收端相同的IPC键值 */
    if ((key = ftok(FTOK_PATH, FTOK_PROJ_ID)) == -1)
    {
        perror("ftok failed");
        exit(1);
    }
    printf("Generate key via ftok: %x\n", key);

    /* 2. 获取消息队列 */
    if ((qid = msgget(key, IPC_CREAT|0666)) == -1)
    {
        perror("msgget failed");
        exit(1);
    }

    printf("Open queue %d\n", qid);

    while(1)
    {
        printf("Enter some message to the queue: ");
        if ((fgets(msg.msg_text, BUFFER_SIZE, stdin)) == NULL)
        {
            printf("\nGet message end.\n");
            exit(1);
        }  

        msg.msg_type = getpid();  // 消息类型设为发送进程的PID
        /* 添加消息到消息队列 */
        if ((msgsnd(qid, &msg, strlen(msg.msg_text), 0)) < 0)
        {
            perror("Send message error");
            exit(1);
        }
        else
        {
            printf("Send message success.\n");
        }

        if (strncmp(msg.msg_text, "quit", 4) == 0)
        {
            printf("\nQuit send message.\n");
            break;
        }
    }

    exit(0);
}