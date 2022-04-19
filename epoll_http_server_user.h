/********************************************************************
@FileName:epoll_server_user.h
@Version: 1.0
@Notes:   None
@Author:  XiaoDexin
@Email:   xiaodexin0701@163.com
@Date:    2022/04/17 15:14:07
********************************************************************/   
#ifndef _EPOLL_SERVER_USER_H
#define _EPOLL_SERVER_USER_H


void epoll_run(int port);
int init_listen_fd(int port, int epfd);
void do_accept(int lfd, int epfd);
void do_read(int cfd, int epfd);
int get_line(int cfd, char *buf, int size);
void disconnect(int cfd, int epfd);
void http_request(const char* request, int cfd);
void send_error(int cfd, int status, char *title, char *text);
void send_respond_head(int cfd, int no, const char * desp, const char *type, long len);
void send_dir(int cfd, const char* dirname);
void send_file(int cfd, const char* filename);
const char *get_file_type(const char *name);
void encode_str(char *to, int tosize, const char *from);
void decode_str(char *to, char *from);
int hexit(char c);



#endif
