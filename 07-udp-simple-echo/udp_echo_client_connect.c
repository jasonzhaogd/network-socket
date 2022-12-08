/**
 * 连接套接字版本的 udp echo 客户端
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
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t addr_size;

    struct sockaddr_in serv_addr, from_addr;

    if (argc != 3)
    {
        printf("Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    /**
     * 像发起 TCP 连接一样 调用 connect 函数，但是这里的 socket 类型为 SOCK_DGRAM，
     * 针对 UDP 套接字调用 connect 函数并不意味着要与对方 UDP 套接字连接，
     * 这只是向 UDP 套接字注册 目标IP和端口信息。
     * 在要向同一个地址发送多次数据时，可以节省每次 sendto 内部重新分配地址和端口号的性能损耗。
    */
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    while (1)
    {
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;
        /**
         * 由于不用再传入地址信息，所以改用 send 和 recv 函数，而不是 sendto 和 recvfrom 函数
         * 甚至可以直接像 TCP 连接那样使用 write 和 read 来收发数据。
        */
        send(sock, message, strlen(message), 0);        // == write(sock, message, strlen(message));
        str_len = recv(sock, message, BUF_SIZE, 0);     // == read(sock, message BUF_SIZE);

        message[str_len] = 0;
        printf("Message from server: %s", message);
    }

    close(sock);
    return 0;
}
