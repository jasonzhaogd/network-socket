/**
 * level-triggered: 条件触发，只要有数据，服务器端不断地从 epoll_wait 中苏醒
 * edge-triggered : 边缘触发，服务器端只从 epoll_wait 中苏醒过一次
 * 
 * 一般我们认为，边缘触发的效率比条件触发的效率要高，这一点也是 epoll 的杀手锏之一。
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

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    // 受理连接的监听还是采用 默认的 level-triggered 模式
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    int wakeup_cnt;

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        printf("epoll_wait wakeup times: %d\n", wakeup_cnt++);
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
                
                // event.events = EPOLLIN;
                // 切换到上面那句就是默认的 条件触发，切换成下面一句就是 边缘触发
                event.events = EPOLLIN | EPOLLET;

                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client fd: %d\n", clnt_sock);
            }
            else // read message
            {
                // 这里特意不把数据读出来，让输入缓存一直保持有数据
                printf("get event on socket fd == %d \n", ep_events[i].data.fd);
                sleep(1);
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
