#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

//打印帮助信息
void usage(const char *argv_0)
{
	printf("\nUsage %s: [-option] \n", argv_0);
	printf("[-a] hello!\n");
	printf("[-b] i am lubancat\n");
	printf("[-c<str>] str\n");;
	printf("[-d<num>] printf num of '*' (num<100)\n");
	printf("[-h] get help\n");
	exit(1);
}


//打印n个*号
void d_option(char *num_str)
{
	int num,i;
	int ge,shi;
	shi = (char)num_str[0] - 48 ;
	ge = (char)num_str[1] - 48 ;
	num = shi*10+ge;
	for(i = 0 ;i <num;i++ )
		printf("*");
	printf("*\n");
}


int main(int argc,char **argv)
{
    int i;
	int opt;
	//轮询获取选项
	while((opt = getopt(argc, argv, "c:d:abh")) != -1) {	//字母后面带冒号的需要有参数的
		switch (opt) {
		case 'a':
			printf("hello!\n");
			break;

		case 'b':
			printf("i am lubancat\n");
			break;

		case 'c':
			if(optarg)
				if(optarg[0] == '-')
					usage(argv[0]);
				else
					printf("%s\n",optarg);
			else
				usage(argv[0]);
			break;

		case 'd':
			if(optarg)					//后面没有参数
				if(optarg[0] == '-')	//后面不能跟'-'
					usage(argv[0]);
				else
					if(strlen(optarg) < 3)		//不超过100
						d_option(optarg);
					else
						usage(argv[0]);
			else
				usage(argv[0]);
			break;
			
		default:
			usage(argv[0]);
			break;
		}

	}
	if(argc < 2)
		printf("welcome to this demo\n");
    return 0;
}
