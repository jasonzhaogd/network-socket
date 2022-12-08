/**
 * 注意 UDP 不同于 TCP，不存在请求连接和连接受理，因此是没有本质意义上的服务器和客服端之分。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_addr_size;

    struct sockaddr_in serv_addr, clnt_addr;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    while (1)
    {
        /**
         * recvfrom 函数的声明是：
         * ssize_t recvfrom(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr *from, socklen_t *addrlen);
         * 
         * 后面两个参数 from 和 addrlen，实际上是返回对端发送方的地址和端口等信息，这和 TCP 非常不一样，
         * TCP 是通过 accept 函数拿到的描述字信息来决定对端的信息。
         * 而 UDP 报文每次接收都会获取对端的信息，也就是说报文和报文之间是没有上下文的。
        */
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        sendto(serv_sock, message, str_len, 0, (struct sockaddr *)&clnt_addr, clnt_addr_size);
    }

    // 代码无法到达，象征性关闭
    close(serv_sock);
    return 0;
}
