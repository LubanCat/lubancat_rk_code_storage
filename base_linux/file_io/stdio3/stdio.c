#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义学生信息结构体，用于存储结构化数据
typedef struct {
    char name[20];
    int id;
    float score;
} Student;

int main() {
    // 初始化待存储的学生数据
    Student stu1 = {"张三", 1001, 95.5};
    Student stu2;  // 用于存储读取的学生数据

    // 1. 以二进制只写模式打开文件，存储结构体数据
    FILE *fp = fopen("student.bin", "wb");
    if (fp == NULL) {
        perror("文件打开失败");
        exit(-1);
    }
    // 二进制写入结构体数据，1表示写入1个数据单元
    fwrite(&stu1, sizeof(Student), 1, fp);
    printf("学生信息二进制写入成功！\n");
    fclose(fp);  // 关闭写流

    // 2. 以二进制只读模式打开文件，读取结构体数据
    fp = fopen("student.bin", "rb");
    if (fp == NULL) {
        perror("文件读取失败");
        exit(-1);
    }
    // 二进制读取数据，存入stu2变量
    fread(&stu2, sizeof(Student), 1, fp);
    // 打印解析后的结构化数据
    printf("===== 学生信息读取结果 =====\n");
    printf("学号：%d\n姓名：%s\n成绩：%.1f\n", stu2.id, stu2.name, stu2.score);
    fclose(fp);

    return 0;
}