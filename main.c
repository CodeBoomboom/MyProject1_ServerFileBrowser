#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include"epoll_http_server_user.h"

int main(int argc, const char* argv[])
{
    if(argc < 3)
    {
        printf("请按以下格式输入:./a.out port path\n");
        exit(1);
    }

    //采用指定端口,将输入的char转化为int
    int port = atoi(argv[1]);

    //修改工作目录,当前目录->指定目录
    int ret = chdir(argv[2]);
    if (ret == -1)
    {
        perror("chdir error");
        exit(1);
    }

    //开启服务器
    epoll_run(port);
    return 0;
}
