/** exec 函数族 */

/**
 * exec 函数族就提供了一个在进程中启动另一个程序执行的方法。它可以根据指定的文件名或
 * 目录名找到可执行文件，并用它来取代原调用进程的数据段、代码段和堆栈段，在执行完之后，原调用进
 * 程的内容除了进程号外，其他全部被新的进程替换了。另外，这里的可执行文件既可以是二进制文件，也
 * 可以是 Linux 下任何可执行的脚本文件。
 * 
 * 在 Linux 中使用 exec 函数族主要有两种情况。
 *    当进程认为自己不能再为系统和用户做出任何贡献时， 就可以调用 exec 函数族中的任意一个函数让自己重生。
 *    如果一个进程想执行另一个程序，那么它就可以调用 fork()函数新建一个进程，然后调用 exec 函
 *     数族中的任意一个函数，这样看起来就像通过执行应用程序而产生了一个新进程（这种情况非常普遍）。 
 * 
 */

/**
 * ************************** exec 函数族成员函数语法 *****************************
 * 所需头文件 #include <unistd.h>
 * 函数原型
 * int execl(const char *path, const char *arg, ...)
 * int execv(const char *path, char *const argv[])
 * int execle(const char *path, const char *arg, ..., char *const envp[])
 * int execve(const char *path, char *const argv[], char *const envp[])
 * int execlp(const char *file, const char *arg, ...)
 * int execvp(const char *file, char *const argv[])
 * 函数返回值 -1：出错 
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* 选择一个要测试的函数示例，把对应的宏为1即可 */
#define     EXECL   1
#define     EXECLP  0
#define     EXECLE  0
#define     EXECV   0
#define     EXECVP  0
#define     EXECVE  0

#if ((EXECL | EXECLP | EXECLE | EXECV | EXECVP | EXECVE) == 0)
#error "must choose a function to compile!"
#endif

#if ((EXECL & EXECLP) || (EXECL & EXECLE) || (EXECL & EXECV) || (EXECL & EXECVP) || (EXECL & EXECVE) || \
    (EXECLP & EXECLE) || (EXECLP & EXECV) || (EXECLP & EXECVP) || (EXECLP & EXECVE) || \
    (EXECLE & EXECV) || (EXECLE & EXECVP) || (EXECLE & EXECVE) || \
    (EXECV & EXECVP) || (EXECV & EXECVE) || \
    (EXECVP & EXECVE))
#error "have and can only choose one of the functions to compile"
#endif


#if EXECL
int main(void)
{
    int err;

    printf("this is a execl function test demo!\n\n");

    /*
        execl()函数用于执行参数path字符串所代表的文件路径（必须指定路径），
        接下来是一系列可变参数，它们代表执行该文件时传递过去的 ``argv[0]、argv[1]… argv[n]`` ，
        最后一个参数必须用空指针NULL作为结束的标志。
    */
    err = execl("/bin/ls", "ls", "-la", NULL);

    if (err < 0) {
        printf("execl fail!\n\n");
    }
    
    printf("Done!\n\n");
}
#endif

#if EXECLP
int main(void)
{
    int err;

    printf("this is a execlp function test demo!\n\n");

    /*
        与execl的差异是，execlp()函数会从PATH环境变量所指的目录中
        查找符合参数file的文件名（不需要指定完整路径）。
    */
    err = execlp("ls", "ls", "-la", NULL);

    if (err < 0) {
        printf("execlp fail!\n\n");
    }

    printf("Done!\n\n");

}
#endif

#if EXECLE
int main(void)
{
    int err;
    char *envp[] = {
        "/bin", NULL
    };

    printf("this is a execle function test demo!\n\n");
    /*
        与execl的差异是，execle()函数会通过最后一个参数（envp）指定新程序使用的环境变量。
    */
    err = execle("/bin/ls", "ls", "-la", NULL, envp);

    if (err < 0) {
        printf("execle fail!\n\n");
    }

    printf("Done!\n\n");
}
#endif

#if EXECV
int main(void)
{
    int err;
    char *argv[] = {
        "ls", "-la", NULL
    };

    printf("this is a execv function test demo!\n\n");

    /*
       与execl的差异是，直接使用数组来装载要传递给子程序的参数/   
    */
    err = execv("/bin/ls", argv);

    if (err < 0) {
        printf("execv fail!\n\n");
    }

    printf("Done!\n\n");
}
#endif

#if EXECVP
int main(void)
{
    int err;
    char *argv[] = {
        "ls", "-la", NULL
    };

    printf("this is a execvp function test demo!\n\n");

    /*
       是execlp，execv函数的结合体
    */
    err = execvp("ls", argv);

    if (err < 0) {
        printf("execvp fail!\n\n");
    }

    printf("Done!\n\n");
}
#endif

#if EXECVE
int main(void)
{
    int err;
    char *argv[] = {
        "ls", "-la", NULL
    };
    char *envp[] = {
        "/bin", NULL
    };

    printf("this is a execve function test demo!\n\n");


    /*
       是execle，execv函数的结合体
    */
    err = execve("/bin/ls", argv, envp);

    if (err < 0) {
        printf("execve fail!\n\n");
    }

    printf("Done!\n\n");
}
#endif