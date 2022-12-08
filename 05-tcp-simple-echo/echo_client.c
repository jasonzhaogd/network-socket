/**
 * 由于 TCP 传输方式是没有边界的， 本客户端存在两个问题：
 * - 多次调用 write 函数传递的字符串有可能一次性传递到服务端
 * - 服务器端发送的字符串太长，要分两个数据包发送
 * 
 * 也就是在 TCP 传输模式下，服务端和客户端 读和写分别一一对应的进行是不能保证的，
 * 在流式传输模式和两端传输数据缓存的特性的参与下， 服务器端和客服端的读/写都是互相隔离开，没有一一对应的调用次数关系。
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUF_SIZE];
    int str_len;

    if (argc != 3)
    {
        printf("Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    else
        puts("Connected.............");

    while (1)
    {
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;

        write(sock, message, strlen(message));
        str_len = read(sock, message, BUF_SIZE);
        message[str_len] = 0;
        printf("Message from server: %s", message);
    }

    // 调用 close 函数会向相应套接字发送 EOF，即意味着中断连接。
    close(sock);
    return 0;
}
