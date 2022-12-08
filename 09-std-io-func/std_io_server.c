/**
 * 基于 标准io 函数 实现的简单回声服务，std io 有以下优点：
 * - 标准 io 函数具有良好的可移植性
 * - 标注 io 函数内部实现了缓冲，在很多情况下可以提高性能
 *     注意，这里提到的 标准 io 函数提供的缓冲和 套接字内部提供的缓冲不是同一个，
 *     套接字内部提供的缓冲是为了实现 TCP 协议而存在的
 * 使用 std io 有以下几个缺点：
 * - 不容易进行双向通信
 * - 切换读写状态时需要调用 fflush 函数
 * - 需要以 FILE 结构体指针的形式操作
 *
 * 从 fd 转 FILE* 的函数
 * - FILE * fdopen(int fd, const char *mode);
 * 从 FILE* 转 fd 函数：
 * - int fileno(FILE *stream);
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char message[BUF_SIZE];

    FILE *readfp, *writefp;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen error");
    }

    clnt_addr_size = sizeof(clnt_addr);
    int str_len;
    for (int i = 0; i < 5; i++)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() eeror");
        else
            printf("Connected client %d\n", i + 1);

        readfp = fdopen(clnt_sock, "r");
        writefp = fdopen(clnt_sock, "w");

        while (!feof(readfp))
        {
            fgets(message, BUF_SIZE, readfp);
            fputs(message, writefp);
            // 由于 std io 内部提供了缓冲，如果不调用 fflush 则无法保证立即把数据传出去。
            fflush(writefp);
        }

        fclose(readfp);
        fclose(writefp);
    }

    close(serv_sock);
    return 0;
}
