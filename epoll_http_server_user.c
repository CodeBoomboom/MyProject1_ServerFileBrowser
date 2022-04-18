/********************************************************************
@FileName:epoll_server_user.c
@Version: 1.0
@Notes:   None
@Author:  XiaoDexin
@Email:   xiaodexin0701@163.com
@Date:    2022/04/17 15:14:43
********************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<dirent.h>
#include<errno.h>
#include"wrap.h"
#include"epoll_http_server_user.h"

#define MAXSIZE 2000

/********************************************************************
@FunName:void epoll_run(int port)
@Input:  port:端口号
@Output: None
@Retuval:None
@Notes:  epoll_http服务器启动函数
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/17 15:15:38
********************************************************************/
void epoll_run(int port)
{
    int i = 0;

    //创建一个epoll树的根节点
    int epfd = Epoll_create(MAXSIZE);

    //添加监听lfd，并将其加到epoll树上
    int lfd = init_listen_fd(port,epfd);

    //委托内核检测添加到树上的节点
    struct epoll_event all[MAXSIZE];
    while(1)
    {
        int ret = Epoll_wait(epfd, all, MAXSIZE, 0);//返回值：实际满足监听的个数

        //遍历发生变化的节点
        for(i=0; i < ret; ++i)
        {
            struct epoll_event *pev = &all[i];
            if(!pev->events & EPOLLIN){
                //不是读事件
                continue;
            }
            if(pev->data.fd == lfd){
                //接受连接请求
                do_accept(lfd,epfd);
            }
            else{

                //读数据
                printf("********开始读数据之前，实际满足监听的个数(Epoll_wait返回值ret)=%d********\n",ret);
                do_read(pev->data.fd, epfd);
                printf("********读数据结束********\n");
            }
        }
    }
}

/********************************************************************
@FunName:int init_listen_fd(int port, int epfd)
@Input:  port:端口号
         epfd:epoll根节点
@Output: None
@Retuval:lfd:监听lfd
@Notes:  None
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 09:57:17
********************************************************************/
int init_listen_fd(int port, int epfd)
{
    int lfd = Socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in serv;
    memset(&serv,0,sizeof(serv));   //用0填充，与bzero功能一致
    //bzero(&serv, sizeof(serv));
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);

    //设置端口复用
    int opt = 1;  
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));

    //地址绑定
    Bind(lfd,(struct sockaddr *)&serv,sizeof(serv));
    //设置监听
    Listen(lfd,64);

    //lfd添加到epoll树上
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    Epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);

    return lfd;
}

/********************************************************************
@FunName:void do_accept(int lfd, int epfd)
@Input:  lfd:监听lfd
         epfd:epoll树根节点
@Output: None
@Retuval:None
@Notes:  接收新连接处理，即当有新客户端连接时调用此函数接受连接，并将其添加到epoll树上
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 11:00:17
********************************************************************/
void do_accept(int lfd, int epfd)
{
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int cfd = Accept(lfd,(struct sockaddr*)&client,&len);

    //打印连接的客户端信息
    char ip[64] = {0};
    printf("新连接的客户端IP：%s, 端口号：%d, cfd =%d\n",
    inet_ntop(AF_INET,&client.sin_addr.s_addr,ip,sizeof(ip)),
    ntohs(client.sin_port),cfd);

    //设置cfd为非阻塞
    int flag = fcntl(cfd,F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    //得到的新节点挂到epoll树上
    struct epoll_event ev;
    ev.data.fd = cfd;
    //边沿非阻塞模式
    ev.events = EPOLLIN | EPOLLET;
    Epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
}

/********************************************************************
@FunName:void do_read(int cfd, int epfd)
@Input:  cfd:要读取数据的客户端cfd
         epfd:epoll树根节点
@Output: None
@Retuval:None
@Notes:  从指定客户端中读取数据
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 11:18:28
********************************************************************/
void do_read(int cfd, int epfd)
{
    //将浏览器发来的数据读到buf（line）中
    char line[1024] = {0};
    //读请求行 
    int len = get_line(cfd, line, sizeof(line));
    if(len == 0){
        printf("客户端断开了连接.........\n");
        disconnect(cfd, epfd);  //关闭套接字，cfd从epoll上del
    } else {
        printf("请求行数据：%s", line);
        printf("******** 请求头 ********\n");
        //还有数据没读完，继续读走
        while(1){
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));
            if(buf[0] == '\n'){
                break;
            }else if (len == -1){
                break;
            }
        }
        printf("******** The End ********");
    }

    //判断get请求
    if(strncasecmp("get",line, 3) == 0){
        // 请求行: get /hello.c http/1.1 
        //处理http请求
        http_request(line, cfd);

        //处理完关闭
        disconnect(cfd, epfd);
    }

}


/********************************************************************
@FunName:int get_line(int cfd, char *buf, int size)
@Input:  cfd:要读的文件描述符
         line:数据缓冲区
         size:缓冲区大小
@Output: None
@Retuval:实际读到的字节数
@Notes:  解析http请求消息的每一行内容
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 14:47:57
********************************************************************/
int get_line(int cfd, char *buf, int size)
{
    //注意：http协议每一行结束是\r\n
    int i = 0;
    int n;
    char c = '\0';
    while((i<size-1) && (c != '\n')){
        n = Recv(cfd, &c, 1 ,0); //一个一个字符的读
        if(n > 0){
            if(c == '\r'){
                n = Recv(cfd, &c, 1, MSG_PEEK); //模拟读一次，实际并没有读，管道内数据还存在
                if((n > 0) && (c == '\n')){
                    Recv(cfd, &c, 1, 0);
                }else{
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        } else {
            c = '\n';
        } 
    }
    buf[i] = '\0';
    return i;
}

/********************************************************************
@FunName:void disconnect(int cfd, int epfd)
@Input:  cfd:断开连接的套接字
         epfd：epoll根节点
@Output: None
@Retuval:None
@Notes:  关闭套接字, cfd从epoll上del
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 15:21:10
********************************************************************/
void disconnect(int cfd, int epfd)
{
    int ret = Epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    Close(cfd);
}

/********************************************************************
@FunName:void http_request(const char* request, int cfd)
@Input:  request:请求命令
         cfd:客户端套接字
@Output: None
@Retuval:None
@Notes:  None
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 15:38:31
********************************************************************/
void http_request(const char* request, int cfd)
{
    //拆分http请求行
    char method[12], path[1024], protocol[12];
    sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol); //取request中的制定数据，正则表达式[^ ]:依次取除空格之外的数据
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);

    //若请求路径中有中文，则转码 将不能识别的中文乱码(URL) -> 中文
    //解码 %23 %24 %5f
    decode_str(path,path);

    char *file = path+1; //去掉path中的/获取访问文件名，+1是因为path第一个字符是/

    //如果没有指定访问的资源，默认显示资源目录中的内容
    if(strcmp(path, "/") == 0){
        // file的值, 资源目录的当前位置
        file = "./";
    }

    //用stat函数获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if(ret == -1){
        //若出错（无此文件或目录），则跳转到404 not found
        send_error(cfd, "404", "Not Found", "NO such file or direntry");
        return;
    }

    //判断是目录还是文件
    if(S_ISDIR(st.st_mode)){//目录
        //发送头信息
        send_respond_head(cfd, 200, "OK", get_file_type(".html"), -1);
        //发送目录信息
        send_dir(cfd, file);
    }else if(S_ISREG(st.st_mode)){  //文件
        //发送消息报头
        send_respond_head(cfd, 200, "OK", get_file_type(file), st.st_size);
        //发送文件内容
        send_file(cfd, file);
    }
    

}

/********************************************************************
@FunName:void send_error(int cfd, int status, char *title, char *text)
@Input:  cfd:客户端cfd
         status:状态码。404
         title:错误码描述/HTML标题。Not Found
         text:描述信息。NO such file or direntry
@Output: None
@Retuval:None
@Notes:  出错页面，如404 not found
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 20:30:14
********************************************************************/
void send_error(int cfd, int status, char *title, char *text)
{
    //使用HTML语言写到buf中，再将buf发到客户端，注意每行是以\r\n结束
    char buf[4096] = {0};
    
    sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
    sprintf(buf+strlen(buf), "Content-Type:%s\r\n", "text/html");
    sprintf(buf+strlen(buf), "Content-Length:%d\r\n", -1);
    sprintf(buf+strlen(buf), "Connection: close\r\n");
    Send(cfd, buf, strlen(buf), 0);
    Send(cfd, "\r\n", 2, 0); //最后发送空行，后面才是数据

    memset(buf, 0 , sizeof(buf));

    sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
    sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
    sprintf(buf+strlen(buf), "%s\n", text);
    sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
    send(cfd, buf, strlen(buf), 0);

    return;
}

/********************************************************************
@FunName:void send_respond_head(int cfd, int no, const char * desp, const char *type, long len)
@Input:  cfd:客户端cfd
         no:状态码
         desp:状态码描述
         type:网络文件类型。需通过get_file_type函数获取
         len:数据报Content Length长度
@Output: None
@Retuval:None
@Notes:  向http客户端发送响应头
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 21:06:08
********************************************************************/
void send_respond_head(int cfd, int no, const char * desp, const char *type, long len)
{
    char buf[1024] = {0};
    //状态行
    sprintf(buf, "http/1.1 %d %s\r\n", no, desp);
    Send(cfd, buf, strlen(buf), 0);
    //消息报头
    sprintf(buf, "Content-Type:%s\r\n", type);
    sprintf(buf+strlen(buf), "Content-Length:%ld\r\n", len);
    Send(cfd, buf, strlen(buf), 0);
    //空行
    Send(cfd, "\r\n", 2, 0);
}

/********************************************************************
@FunName:send_dir
@Input:  cfd:客户端cfd
         dirname:目录名
@Output: None
@Retuval:None
@Notes:  发送目录内容
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 21:16:31
********************************************************************/
void send_dir(int cfd, const char* dirname)
{
    printf("请求的是目录，开始发送目录信息...");
    int i, ret;
    //拼一个html页面<table></table>
    char buf[4096] = {0};
    sprintf(buf,"<html><head><title>目录名: %s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>当前目录: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};

    //目录项二级指针
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);//目录遍历/扫描函数：参1：目录名  参2：传出参数，传出目录下内容 ，实际上是一个二维数组指针(模拟目录下仍有目录) 参3：过滤器，没有填NULL 参4：比较（排序）alphasort：按字符排序   返回值：与epoll_wait相似，实际就是数组的个数？

    //遍历
    for(i = 0;i < num; ++i){
        char * name = ptr[i]->d_name;

        //拼接文件的完整路径
        sprintf(path, "%s/%s", dirname, name);
        printf("path = %s\n", path);
        struct stat st;
        stat(path, &st);

        // 编码生成 %E5 %A7 之类的东西
        encode_str(enstr, sizeof(enstr), name);

        //如果是文件
        if(S_ISREG(st.st_mode)){
            sprintf(buf+strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_mode);
        }else if(S_ISDIR(st.st_mode)){  // 如果是目录 
            sprintf(buf+strlen(buf),
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        ret = Send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                continue;
            } else if (errno == EINTR) {
                continue;
            } else {
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf)); 
    }

    sprintf(buf+strlen(buf),"</table></body></html>");
    Send(cfd, buf, strlen(buf), 0);

    printf("目录信息发送完成！");
}

/********************************************************************
@FunName:void send_file(int cfd, const char* filename)
@Input:  cfd:客户端cfd
         filename:要发送的文件名   
@Output: None
@Retuval:None
@Notes:  发送文件到客户端
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 22:15:31
********************************************************************/
void send_file(int cfd, const char* filename)
{
    //打开文件
    int fd = open(filename, O_RDONLY);
    if(fd == -1) {   
        send_error(cfd, 404, "Not Found", "NO such file or direntry");
        exit(1);
    }

    //循环读文件
    char buf[4096] = {0}; //注意：若文件很大，这个buf要加大
    int len = 0, ret = 0;
    while((len = Read(fd, buf, sizeof(buf))) > 0){
        //发送出读的数据
        ret = Send(cfd, buf, len, 0);
        if(ret == -1){
            if (errno == EAGAIN) {
                continue;
            } else if (errno == EINTR) {
                continue;
            } else {
                exit(1);
            }            
        }
    }
    Close(fd);
}

/********************************************************************
@FunName:const char *get_file_type(const char *name)
@Input:  name:文件名
@Output: None
@Retuval:返回文件类型
@Notes:  通过本地文件名得到网络文件类型
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 20:56:05
********************************************************************/
const char *get_file_type(const char *name)
{
    char* dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL，成功返回该字符及之后的字符
    dot = strrchr(name, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

/********************************************************************
@FunName:void encode_str(char *to, int tosize, const char *from)
@Input:  from:输入字符
@Output: to: 输出URL码
@Retuval:None
@Notes:  URL编码函数，汉字——>URL码
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 20:12:30
********************************************************************/
void encode_str(char *to, int tosize, const char *from)
{
    int tolen;
    for(tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from){
        if(isalnum(*from) || strchr("/_.-~", *from) != (char*)0){
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x",(int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

/********************************************************************
@FunName:void decode_str(char *to, char *from)
@Input:  from：传入参数
@Output: to：传出参数
@Retuval:None
@Notes:  URL解码函数，URL码——>汉字
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 19:57:21
********************************************************************/
void decode_str(char *to, char *from)
{
    for( ;*from != '\0'; ++to, ++from){
        if(from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) { //isxdigit：检查是否为16进制
            *to = hexit(from[1])*16 + hexit(from[2]);
            from +=2;
        }else {
            *to = *from;
        }
    }
    *to = '\0';
}

/********************************************************************
@FunName:int hexit(char c)
@Input:  c:输入16进制数
@Output: None
@Retuval:输出对应10进制数
@Notes:  None
@Author: XiaoDexin
@Email:  xiaodexin0701@163.com
@Time:   2022/04/18 20:09:06
********************************************************************/
int hexit(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    
    return 0;
}

