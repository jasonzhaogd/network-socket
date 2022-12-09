/**
 * epoll 模式实现的 echo 服务端， 主要是3个函数：
 * - epoll_create
 * - epoll_ctl
 * - epoll_wait
 * 
 * epoll 模式的优点：
 * - epoll_wait 只返回有变化的fd集合，无需遍历所有fd
 * - 调用 epoll_wait 函数时，无需每次给 OS 传递监视对象集合，而是在需要时针对每个监视对象单独操作
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "../00-lib/error.h"

#define BUF_SIZE 100
#define EPOLL_SIZE 50

void read_child_proc(int sig);

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

    
    /**
     * epoll 初始化，创建了一个 epoll 实例，原型是：
     * int epoll_create(int size);
     * - 一开始的 epoll_create 实现中，是用来告知内核期望监控的文件描述字大小，然后内核使用这部分的信息来初始化内核数据结构，
     *   在新的实现中，这个参数不再被需要，因为内核可以动态分配需要的内核数据结构。我们只需要注意，每次将 size 设置成一个大于 0 的整数就可以了。
    */
    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    // 把 serv_sock 放入监视列表
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);
    /**
     * epoll_ctl 第二个参数可选：
     * - EPOLL_CTL_ADD
     * - EPOLL_CTL_DEL
     * - EPOLL_CTL_MOD
     * 
     * epoll_event 结构体的 events 类型有：
     * - EPOLLIN      : 需要读数据的情况
     * - EPOLLOUT     : 输出缓冲为空，可以立即发送数据的情况
     * - EPOLLPRI     : 收到 OOB 数据的情况
     * - EPOLLRDHUP   : 断开连接或半关闭的情况，这在边缘触发方式下非常有用
     * - EPOLLERR     : 发送错误的情况
     * - EPOLLET      : 以边缘触发的方式得到事件通知
    */

    while (1)
    {
        /**
         * int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
         * - epfd       : 需要监视的 epoll fd
         * - events     : 保存发生事件的事件信息集合，地址
         * - maxevents  : 第二个参数中可以保存的最大事件数，即 events 集合的大小
         * - timeout    : 以毫秒为单位的等待时间，-1 代表一直等待。
         * ==> 返回值 : 成功时返回发生事件的数量，失败时返回 -1。 如果到了timeout时间没有事件发生，返回0
        */
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
                clnt_addr_size = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                // 把新受理的连接请求对应的socket放入监视列表
                // 奇怪：这里的 event 可以复用，而不会出现冲突？特别是下面 epoll_ctl 还是用的 event 的地址。
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
                    // 把即将要关闭的socket从监视列表中清除
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

// 客户端可以用 05/echo_client.c

/**
 * epoll 机制是 Linux 特有的
 * 早在 Linux 实现 epoll 之前，Windows 系统就已经在 1994 年引入了 IOCP，这是一个异步 I/O 模型，用来支持高并发的网络 I/O，
 * 而著名的 FreeBSD 在 2000 年引入了 Kqueue，也是一个 I/O 事件分发框架。
*/
