/**
 * 当应用程序调用阻塞 I/O 完成某个操作时，应用程序会被挂起，等待内核完成操作，感觉上应用程序像是被“阻塞”了一样。
 * 实际上，内核所做的事情是将 CPU 时间切换给其他有需要的进程，网络应用程序在这种情况下就会得不到 CPU 时间做该做的事情。
 *
 * 非阻塞 I/O 则不然，当应用程序调用非阻塞 I/O 完成某个操作时，内核立即返回，不会把 CPU 时间切换给其他进程，
 * 应用程序在返回后，可以得到足够的 CPU 时间继续完成其他事情。
 *
 * 事实上，非阻塞 I/O 配合 I/O 多路复用，是高性能网络编程中的常见技术。
 */

/**
 * 通过一张表来总结一下 read 和 write 在阻塞模式和非阻塞模式下的不同行为特性：
 * ==============================================================================================|
 * |   操作   | 系统内核缓冲区状态 |       阻塞模式          |               非阻塞模式                 |
 * ==============================================================================================|
 * |         | 接收缓冲区有数据   | 立即返回                | 立即返回                                 |
 * | read()  |-----------------|-----------------------------------------------------------------|
 * |         | 接收缓冲区没数据   | 一直等数据到来           | 立即返回，带有 EWOULDBLOCK 或 EAGAIN 错误 |
 * |-------------------------------------------------------------------------------------------- |
 * |         | 发送缓冲区空闲     | 全部数据都写入缓冲区才返回 | 能写入多少就写入多少，立即返回              |
 * | write() |----------------—|-----------------------------------------------------------------|
 * |         | 发送缓冲区不空闲   | 等待发送缓冲区空闲        | 立即返回，带有 EWOULDBLOCK 或 EAGAIN 错误 |
 * |---------------------------------------------------------------------------------------------|
 *
 */

/**
 * 当 accept 和 I/O 多路复用 select、poll 等一起配合使用时，
 * 如果在监听套接字上触发事件，说明有连接建立完成，此时调用 accept 肯定可以返回已连接套接字。
 *
 * 但是如果在 多路复用被监听套接字上的新连接唤醒，到真正执行 accept 函数之间发生了某些异常，
 * 导致新连接请求还未被accept就被从等待队列中清除（nonblocking_client.c 中模拟了此情况），
 * 而真正执行 accept 函数时，就被阻塞在 accept 函数（假设没有其他连接进来）
 * 这样 多路复用 就失效了，其他 read 和 write 的事件就没有机会被唤醒相应。（下面的代码就是这样一个模拟）
 *
 * 所以，在实际工作中，一定要将监听套接字设置为非阻塞的！
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include "../00-lib/error.h"

#define BUF_SIZE 100
#define EPOLL_SIZE 50

void set_nonblocking_mode(int fd);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buf[BUF_SIZE];
    int str_len;

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

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

    // 把监听的套接字设置为非阻塞模式
    // set_nonblocking_mode(serv_sock);

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        printf("epoll_wait got: %d\n", event_cnt);
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        for (int i = 0; i < event_cnt; i++)
        {

            if (ep_events[i].data.fd == serv_sock) // connection requets
            {
                printf("listening socket readable\n");
                sleep(3);

                clnt_addr_size = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

                // 把数据传输套接字设置为非阻塞模式
                set_nonblocking_mode(clnt_sock);

                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client fd: %d\n", clnt_sock);
            }
            else // read message
            {
                str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
                if (str_len == 0) // close request
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client fd: %d\n", ep_events[i].data.fd);
                }
                else
                    write(ep_events[i].data.fd, buf, str_len);
            }
        }
    }

    close(serv_sock);
    // 记得关闭 epoll fd
    close(epfd);
    // 记得释放内存
    free(ep_events);
    return 0;
}

void set_nonblocking_mode(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

/**
 * 测试运行步骤：
 * - 先运行 nonblocking_server
 * - 再允许 05/echo_client, 发送消息
 * - 最后运行 nonblocking_client
 * - 再切换到 05/echo_client 发送消息时，应该没有反应了。
 * 
 * 切换到非阻塞模式再试试
*/

/**
 * 目前测试的效果 阻塞模式下，并没有 卡在 accept 函数，可能是客户端场景没有模拟好。
*/
