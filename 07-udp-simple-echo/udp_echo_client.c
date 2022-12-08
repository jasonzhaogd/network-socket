
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

    while (1)
    {
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;
        /**
         * 没有调用 connect 函数把 IP端口号 分配给 socket，就直接调用 sendto 发送数据
         * 因为调用 sendto 函数时，系统会自动分配 IP和端口号，但是每次调用 sendto 都会重新分配 IP和端口号。
         * 这种未注册目标地址信息的套接字称为未连接套接字，反之被称作连接套接字 参考 udp_echo_client_connect.c
        */
        sendto(sock, message, strlen(message), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        addr_size = sizeof(from_addr);
        str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr *)&from_addr, &addr_size);

        message[str_len] = 0;
        printf("Message from server: %s", message);
    }

    close(sock);
    return 0;
}
