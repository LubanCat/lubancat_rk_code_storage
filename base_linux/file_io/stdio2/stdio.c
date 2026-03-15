#include <stdio.h>

int main() {
    // 定义存储用户信息的变量
    char name[30];
    int age;

    // 标准输出打印提示信息，行缓冲遇换行符刷新，实时显示
    printf("===== 用户信息录入系统 =====\n");
    printf("请输入姓名：");

    // 从标准输入读取姓名，限制长度避免缓冲区溢出
    fgets(name, sizeof(name), stdin);

    // 处理fgets读取的换行符，优化输出效果
    for (int i = 0; name[i] != '\0'; i++) {
        if (name[i] == '\n') {
            name[i] = '\0';
            break;
        }
    }

    printf("请输入年龄：");

    // 从标准输入读取年龄，接收返回值用于校验
    int ret = scanf("%d", &age);

    // 错误处理：输入格式错误，通过标准错误流实时输出
    if (ret != 1) {
        // 标准错误流无缓冲，信息立即打印
        fprintf(stderr, "输入错误：年龄必须为合法整数！\n");
        return -1;
    }
    
    // 年龄合法性校验
    if (age < 0 || age > 150) {
        fprintf(stderr, "输入错误：年龄超出合法范围（0-150）！\n");
        return -1;
    }

    // 打印最终录入结果
    printf("\n===== 录入结果 =====\n");
    printf("姓名：%s\n", name);
    printf("年龄：%d\n", age);
    printf("信息录入成功！\n");

    return 0;
}