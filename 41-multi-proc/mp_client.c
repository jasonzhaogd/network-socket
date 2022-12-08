/**
 * 之前的 echo_client 有一个瓶颈，就是从 socket 读和写不能同时进行，
 * 其实 tcp socket 是双信道的可以同时进行读和写。
 * 就是因为双信道的读和写是两个不同的缓冲区，所以才能支持同时读和写不冲突。
 *
 * 本程序利用两个不同的进程来对应 socket 的读和写操作。
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    // 如果两个进程共用一个缓存区，就会出现冲突，下面两个缓存 user_buf 用于用户输入缓存， net_buf 用于接收网络数据缓存
    char user_buf[BUF_SIZE];
    char net_buf[BUF_SIZE];

    pid_t pid;

    if (argc != 3)
    {
        printf("Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    pid = fork();
    if (pid == 0)
        write_routine(sock, user_buf);
    else
        read_routine(sock, net_buf);

    /**
     * 套接字关闭的过程：
     * - 先在 write routine 中（子进程），用户输入 q/Q 后主动退出程序前 调用 shutdown 关闭输出流
     * - 在 read routine 中读到 EOF 退出循环，到此处（父进程）调用 close 关闭剩余的输入流。
     */
    close(sock);
    return 0;
}

void write_routine(int sock, char *buf)
{
    while (1)
    {
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n"))
        {
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}

void read_routine(int sock, char *buf)
{
    int str_len;
    while (1)
    {
        if ((str_len = read(sock, buf, BUF_SIZE)) == 0)
            return;
        buf[str_len] = 0;
        printf("Message from server: %s", buf);
    }
}
