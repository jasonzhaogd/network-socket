/**
 * POSIX 线程是现代 UNIX 系统提供的处理线程的标准接口。
 * POSIX 定义的线程函数大约有 60 多个，这些函数可以帮助我们创建线程、回收线程。
 * 主要的有：
 * - int pthread_create(pthread_t *tid, const pthread_attr_t *attr, void *(*func)(void *), void *arg); 创建线程
 * - pthread_t pthread_self(void) 返回自己的线程 TID
 * - void pthread_exit(void *status) 调用这个函数之后，父线程会等待其他所有的子线程终止，之后父线程自己终止。
 * - int pthread_cancel(pthread_t tid) 和 pthread_exit 不同的是，它可以指定某个子线程终止。
 * - int pthread_join(pthread_t tid, void ** thread_return) 主线程会阻塞，直到对应 tid 的子线程自然终止
 * - int pthread_detach(pthread_t tid) 一个分离的线程不能被其他线程杀死或回收资源，
 *                      在高并发的例子里，每个连接都由一个线程单独处理，在这种情况下，服务器程序并不需要对每个子线程进行终止，
 *                      这样的话，每个子线程可以在入口函数开始的地方，把自己设置为分离的，这样就能在它终止后自动回收相关的线程资源了，
 *                      就不需要调用 pthread_join 函数了。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

void* thread_run(void *arg);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    // char buf[BUF_SIZE];
    // int str_len;

    pthread_t tid;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // 打开 SO_REUSEADDR
    int option = 1;
    int optlen = sizeof(option);
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");

    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue;
        else
            puts("new client connected......");

        pthread_create(&tid, NULL, thread_run, (void*)&clnt_sock);
    }

    close(serv_sock);
    return 0;
}

void* thread_run(void *arg)
{
    // 把自己分离，自己负责资源回收
    pthread_detach(pthread_self());

    int fd = *(int *)arg;

    char buf[BUF_SIZE];
    int str_len;

    while ((str_len = read(fd, buf, BUF_SIZE)) > 0)
    {
        buf[str_len] = '\0';
        printf("(%lu) Message from client: %s", (unsigned long)pthread_self(), buf);
        write(fd, buf, str_len);
    }
    if (str_len < 0)
        perror("read error");
    close(fd);
    puts("client disconnected...");

    return 0;
}
