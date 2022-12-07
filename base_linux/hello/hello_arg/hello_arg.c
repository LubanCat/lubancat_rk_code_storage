#include <stdio.h>
/* 执行命令: ./hello lubancat
* argc = 2
* argv[0] = ./hello
* argv[1] = lubancat
*/
int main(int argc,char **argv)
{
    int i;
	if(argc >= 2){
		printf("you enter: \""); 
		//打印每一个参数
		for(i = 0 ; i<argc ;i++)
			printf("%s ",argv[i]);
		printf("\"\n");
		//打印参数的个数
		printf("you enter %d strings\n",argc);
	}
	else
		printf("hello, world! This is a C program.\n");
    return 0;
}
