/**
 * 计算器服务端，体验定义应用层协议控制通讯过程
 * 服务端从客服端获取多个数字和运算符信息，服务端按照要求计算完后返回结果给客服端
 * 协议定义：
 * - 接收参数格式如下：
 *    1> 1字节，整型，待运算数字个数n
 *    2> n个整数，每个整数占用4字节
 *    3> 1字节，字符，运算符，支持：+、-、*
 * - 服务器运算结果用 4字节 整数返回给客服端
 * - 客户端接收到运算结果后关闭连接。
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 1024
#define OPSZ 4

int calculate(int opnum, int opnds[], char operator);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    char opinfo[BUF_SIZE];

    int result, opnd_cnt;
    char operator;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

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
    for (int i = 0; i < 5; i++)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() eeror");
        else
            printf("Connected client %d\n", i + 1);

        opnd_cnt = 0;
        read(clnt_sock, &opnd_cnt, 1);
        for (int i = 0; i < opnd_cnt; i++)
        {
            read(clnt_sock, (int *)&opinfo[i * OPSZ], OPSZ);
        }
        read(clnt_sock, &operator, 1);

        result = calculate(opnd_cnt, (int *)opinfo, operator);
        write(clnt_sock, &result, sizeof(result));

        close(clnt_sock);
    }

    close(serv_sock);
    return 0;
}

int calculate(int opnum, int opnds[], char operator)
{
    int result = opnds[0];
    switch (operator)
    {
    case '+':
        for (int i = 1; i < opnum; i++)
            result += opnds[i];
        break;
    case '-':
        for (int i = 1; i < opnum; i++)
            result -= opnds[i];
        break;
    case '*':
        for (int i = 1; i < opnum; i++)
            result *= opnds[i];
        break;
    default:
        result = -1;
        break;
    }

    return result;
}
