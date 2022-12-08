/**
 * 验证 UDP 数据传输是有边界的
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

    struct sockaddr_in my_addr, peer_addr;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
        error_handling("bind() error");

    for (int i = 0; i < 4; i++)
    {
        sleep(2);
        /**
         * 接收函数调用次数是和 host2 中的发送函数调用次数是一一对应的，
         * host2 连续发送了3次，这里每隔2秒收一次，也只能收到3次，第4次就阻塞了。
         * 哪怕下面接收数据的长度(写死为10)小于发送的长度，多余的数据在本次获取之后也废弃了，不会遗留到下一次获取数据。
        */
        str_len = recvfrom(serv_sock, message, 10, 0, (struct sockaddr *)&peer_addr, &clnt_addr_size);
        message[str_len] = 0;
        printf("Message %d from peer: %s\n", i + 1, message);
    }

    close(serv_sock);
    return 0;
}
