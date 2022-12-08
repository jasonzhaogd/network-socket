/**
 * IO多路复用和多线程模式的比较，用老师和学生的例子：
 * - 多线程模式：一个学生配一个专门的老师，有问题随时可以提问
 * - io多路复用：一班学生配一个老师，学生要提问前要举手，老师选择其中一个举手的回答其问题
 *
 * select 函数的声明为：
 * int select(int maxfd, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout);
 * - maxfd     : 监视对象文件描述符数量。
 * - readset   : 将所有关注 “是否有待读取数据” 的 fd 注册到 fd_set 类型变量，并传递其地址值
 * - writeset  : 将所有关注 “是否可传输无阻塞数据” 的 fd 注册到 fd_set 类型变量，并传递其地址值
 * - exceptset : 将所有关注 “是否发生异常” 的 fd 注册到 fd_set 类型变量，并传递其地址值
 * - timeout   : 调用 select 函数后，为了防止陷入无限阻塞的状态，设置超时时间。
 * ==> 返回值   : 发生错误返回-1，超时返回0，因发生关注时间返回大于0的值，该值是发生事件的 fd
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include "../00-lib/error.h"

#define BUF_SIZE 100

void read_child_proc(int sig);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buf[BUF_SIZE];

    struct timeval timeout;
    fd_set reads, cpy_reads;
    int fd_max, str_len, fd_num;

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
     * 操作关注事件的描述符集的函数有：
     * - FD_ZERO(fd_set *fdset)         : 将 fd_set 变量的所有位初始化为0
     * - FD_SET(int fd, fd_set *fdset)  : 在参数 fdset 指向的变量中注册 fd
     * - FD_CLR(int fd, fd_set *fdset)  : 在参数 fdset 指向的变量中取消注册 fd
     * - FD_ISSET(int fd, fd_set *fdset): 测试 参数 fdset 指向的变量中是否注册了 fd
     */
    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    // 设置 timeout 时间
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while (1)
    {
        // 一次select 操作中，把对 fdset 的操作分开，读取数据用 cpy_reads, 写入新数据用 reads
        cpy_reads = reads;

        /**
         * 第一个参数是 fd_max + 1，是因为 fd 从 0 开始
         */
        if ((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
            break;       // exception
        if (fd_num == 0) // timeout
            continue;

        for (int i = 0; i < fd_max + 1; i++)
        {
            if (FD_ISSET(i, &cpy_reads))
            {
                if (i == serv_sock) // connection requets
                {
                    clnt_addr_size = sizeof(clnt_addr);
                    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                    // 把新受理的连接请求对应的socket放入关注事件fd集合
                    FD_SET(clnt_sock, &reads);
                    if (fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("connected client fd: %d\n", clnt_sock);
                }
                else // read message
                {
                    str_len = read(i, buf, BUF_SIZE);
                    if (str_len == 0) // close request
                    {
                        // 把即将要关闭的socket从关注事件fd集合清除
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client fd: %d\n", i);
                    }
                    else
                        write(i, buf, str_len);
                }
            }
        }
    }

    close(serv_sock);
    return 0;
}

// 客户端可以用 05/echo_client.c

/**
 * select 的io多路复用方式由来已久，但是性能极差，无论如何优化程序也很难同时接入上百个客户端，
 * 这种方式并不适合以 web 服务器端开发。它性能差的原因是：
 * - 调用 select 函数后，需要针对文件描述符循环判断
 * - 每次调用 select 函数时都要向该函数传递监视对象信息。
*/
