/**
 * select 方法是多个 UNIX 平台支持的非常常见的 I/O 多路复用技术,
 * 但是 select 有一个缺点，那就是所支持的文件描述符的个数是有限的。
 * 在 Linux 系统中，select 的默认最大值为 1024。
 */

/**
 * poll 是除了 select 之外，另一种普遍使用的 I/O 多路复用技术，
 * 和 select 相比，它和内核交互的数据结构有所变化，另外，也突破了文件描述符的个数限制。
 * 下面是 poll 函数的原型：
 * int poll(struct pollfd *fds, unsigned long nfds, int timeout);
 * - nfds    描述的是数组 fds 的大小，简单说，就是向 poll 申请的事件检测的个数。
 * - timeout  <0 表示在有事件发生之前永远等待；
 *            =0 表示不阻塞进程，立即返回；
 *            >0 表示 poll 调用方等待指定的毫秒数后返回。
 * ==> 返回值：若有就绪描述符则为其数目，若超时则为 0，若出错则为 -1
 *
 * 第一个参数是一个 pollfd 的数组。其中 pollfd 的结构如下：
 * struct pollfd {
 *  int    fd;       // file descriptor
    short  events;   // 描述符上待检测的事件类型, 可以表示多个不同的事件，具体的实现可以通过使用二进制掩码位操作来完成
    short  revents;  // 和 select 非常不同的地方在于，poll 每次检测之后的结果不会修改原来的传入值，而是将结果保留在 revents 字段中，
                        这样就不需要每次检测完都得重置待检测的描述字和感兴趣的事件。我们可以把 revents 理解成“returned events”
 * }
 * events 的可读类型有：
 * - POLLIN     0x0001    // any readable data available
 * - POLLPRI    0x0002    // OOB/Urgent readable data
 * - POLLRDNORM 0x0040    // non-OOB/URG data available
 * - POLLRDBAND 0x0080    // OOB/Urgent readable data
 *
 * events 的可写类型有：
 * - POLLOUT    0x0004    // file descriptor is writeable
 * - POLLWRNORM POLLOUT   // no write type differentiation
 * - POLLWRBAND 0x0100    // OOB/Urgent data can be written
 *
 * returned events 的类型有：
 * - POLLERR    0x0008    // 一些错误发送
 * - POLLHUP    0x0010    // 描述符挂起
 * - POLLNVAL   0x0020    // 请求的事件无效
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/poll.h>
#include "../00-lib/error.h"

#define BUF_SIZE 100
#define POLL_SIZE 128

void read_child_proc(int sig);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buf[BUF_SIZE];

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
     * 初始化 pollfd 数组，这个数组的第一个元素是 listen_fd，其余的用来记录将要连接的 connect_fd
     */
    struct pollfd event_set[POLL_SIZE];
    event_set[0].fd = serv_sock;
    event_set[0].events = POLLRDNORM;
    // 用 -1 表示这个数组位置还没有被占用
    for (int i = 1; i < POLL_SIZE; i++)
    {
        event_set[i].fd = -1;
    }

    int ready_num, str_len;

    while (1)
    {
        if ((ready_num = poll(event_set, POLL_SIZE, -1)) < 0)
            break; // exception
printf("poll return: %d\n", ready_num);
        if (event_set[0].revents & POLLRDNORM) // connection requets
        {
            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
            // 找到一个可以记录该连接套接字的位置
            int i;
            for (i = 1; i < POLL_SIZE; i++)
            {
                if (event_set[i].fd < 0)
                {
                    event_set[i].fd = clnt_sock;
                    event_set[i].events = POLLRDNORM;
                    break;
                }
            }
            if (i == POLL_SIZE)
                error_handling("can not hold so many clients");
            else
                printf("connected client fd: %d\n", clnt_sock);

            if (--ready_num <= 0) // 如果没有其他事件了
                continue;
        }
        for (int i = 1; i < POLL_SIZE && ready_num > 0; i++) // 循环判断每个 poll 是否有事件发生
        {
            int socket_fd = event_set[i].fd;
            if (socket_fd < 0)
                continue;
            if (event_set[i].revents & (POLLRDNORM | POLLERR))
            {
                str_len = read(socket_fd, buf, BUF_SIZE);
                if (str_len == 0) // close request
                {
                    close(socket_fd);
                    event_set[i].fd = -1;
                    printf("closed client fd: %d\n", socket_fd);
                }
                else
                    write(socket_fd, buf, str_len);

                ready_num--;
            }
        }
    }

    close(serv_sock);
    return 0;
}

// 客户端可以用 05/echo_client.c
