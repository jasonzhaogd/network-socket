/**
 * 多进程版 echo 服务端，有以下改进：
 * - 使用 fork 创建子进程，承载每个 accept 出来的 客服端套接字的通讯
 * - 使用 信号注册 sigaction 函数，注册<子进程结束>信号，有两个好处：
 *   * 避免了僵尸进程，
 *   * 避免了父进程为了 wait 子进程的阻塞等待
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

void read_child_proc(int sig);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buf[BUF_SIZE];
    int str_len;

    pid_t pid;
    struct sigaction act;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 设置信号处理程序
    act.sa_handler = read_child_proc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);

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

    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue;
        else
            puts("new client connected......");

        pid = fork();
        if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        else if (pid == 0)
        {
            close(serv_sock);
            while ((str_len = read(clnt_sock, buf, BUF_SIZE)) != 0) {
                printf("Message from client: %s", buf);
                write(clnt_sock, buf, str_len);
            }
            close(clnt_sock);
            puts("client disconnected...");

            return 0;
        }
        else
        {
            printf("created child proc id = %d\n", pid);
            /**
             * fork 时，文件描述符也会被复制，但是套接字还是一个，也就是一个套接字会对应多个文件描述符
             * 只有套接字对应的所有描述都被关闭时，套接字才会被销毁，
             * 所以主进程先把不需要的客服端套接字描述符关闭。 
            */
            close(clnt_sock);
        }
    }

    close(serv_sock);
    return 0;
}

void read_child_proc(int sig)
{
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
    printf("removed child proc id: %d\n", pid);
}
