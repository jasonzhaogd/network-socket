/**
 * 优雅地断开 socket 连接
 * 半关闭是指断开双信道中的一个，达到可以传输数据但无法接收，或可以接收数据但无法传输的效果。
 * close 函数是完全断开连接，而 shutdown 函数是半关闭函数。
 * 注意，close 函数关闭连接有两个需要明确的地方：
 * - close 函数只是把套接字引用计数减 1，未必会立即关闭连接；
 * - close 函数如果在套接字引用计数达到 0 时，立即终止读和写两个方向的数据传送。
 *
 * 优雅断开连接的方式为：
 * - 本方传输完数据后，调用 shutodwn 函数，只关闭本方的输出流，保留输入流开着。
 * - 由于本方关闭了输出流，所以会发出 EOF 信号
 * - 对方收到 EOF 信号后，判断对方写完了，此时，对方发送反馈信息给到本方，对方关闭连接
 * - 对方的输出对应本方的输入，由于输入通道还开着，本方就能收到收到反馈信息，本方关闭连接
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    FILE *fp;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    char buf[BUF_SIZE];
    int read_cnt;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    fp = fopen("file-server.c", "rb");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");

    clnt_addr_size = sizeof(clnt_addr);
    int str_len;

    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

    while (1)
    {
        read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
        write(clnt_sock, buf, read_cnt);
        if (read_cnt < BUF_SIZE)
            break;
    }
    // 先关闭文件
    fclose(fp);

    // 半关闭连接，只关闭输出
    shutdown(clnt_sock, SHUT_WR);
    
    // 读取客户端的反馈信息
    read(clnt_sock, buf, BUF_SIZE);
    printf("Feedback msg from client: %s\n", buf);

    close(clnt_sock);
    close(serv_sock);
    return 0;
}