/********************************************************************
@FileName:wrap.h
@Version: 1.0
@Notes:   封装Socket+epoll相关函数，加入错误提示
@Author:  XiaoDexin
@Email:   xiaodexin0701@163.com
@Date:    2022/04/17 17:10:21
********************************************************************/
#ifndef _WRAP_H_
#define _WRAP_H_

void perr_exit(const char *s);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
int Close(int fd);
ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
static ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);
int Epoll_create(int size);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
ssize_t Recv(int fd, void *buf, size_t n, int flags);
ssize_t Send(int __fd, const void *__buf, size_t __n, int __flags);

#endif
