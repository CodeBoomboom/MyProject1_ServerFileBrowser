src = $(wildcard *.c)#匹配当前目录下的所有.c文件
targets = $(patsubst %.c, %, $(src))#将.c文件替换为不带.c的文件

CC = gcc
CFLAGS = -Wall -g

All:$(targets)

$(targets):%:%.c#静态模式规则,一个TAB缩进
	$(CC) $< -o $@ $(CFLAGS)

.PHONY:clean all
clean:
	-rm -rf $(targets)