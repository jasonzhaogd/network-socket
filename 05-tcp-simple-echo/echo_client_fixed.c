/**
 * 修正上一个版本的问题
 * 由于每次从 sock 读数据时，都提前知道数据长度，所以可以很简单的修复问题
 * 但是在更多情况下是无法预先知道要接收数据的长度的，（基于TCP）此时就需要在应用层协议定义数据通讯的格式和规则。
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
    int str_len, recv_len, recv_cnt;

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

        str_len = write(sock, message, strlen(message));
        recv_len = 0;
        // 把之前只 read 一次的写法，改成了循环读，因为预先知道长度，所以要读到预期的长度才结束。
        // 注意这里不要写成 recv_len != str_len， 这种写法在一些特殊情况下会出现死循环
        while (recv_len < str_len)
        {
            recv_cnt = read(sock, &message[recv_len], BUF_SIZE - recv_len);
            if (recv_cnt == -1)
                error_handling("read() error ");

            recv_len += recv_cnt;
        }
        message[str_len] = 0;
        printf("Message from server: %s", message);
    }

    // 调用 close 函数会向相应套接字发送 EOF，即意味着中断连接。
    close(sock);
    return 0;
}
