/**
 * 验证 服务端 tcp 的传输方式
 *   tcp 是没有边界的流
 *   服务器端写数据的次数和 客户端读数据的次数是不相等的
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[30];
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
    /**
     * inet_addr 的用法参考04目录
    */
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    /**
     * connect 函数也定义在 <sys/socket.h> 声明为：
     * - int connect(int sock, const struct sockaddr *servaddr, socklen_t addrlen);
     *      sock:     客户端套接字
     *      servaddr: 保存目标服务器地址信息的变量地址值
     *      addrlen:  上面 servaddr 结构体 变量长度
     *   -> 成功返回 0， 失败返回 -1。 
     *   ### 注意，这里的成功返回，并不意味着服务器端调用 accept 函数，
     *         其实是服务器端把连接请求放入到等待队列，
     *         因此此函数返回后并不立即进行数据交换。
     *   ### 客户端整个代码中都没有给套接字分配地址，网络数据交换必须分配 IP 和 端口，
     *         其实是有的，就是在 connect 函数中，由操作系统(内核)自动分配的。
     *         不需要像服务器端那样手动调用 bind 函数为 sock 分配地址，
     *         主要是因为这个地址只是操作系统自己使用，所以IP地址就用客服端主机IP，端口号随机。
     *          当然，在调用 connect 之前也可以调用bind绑定具体的地址和端口，但是没必要，还徒增了端口冲突的危险。
    */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("connect() error");
    }

    int idx = 0, read_len = 0;
    while (read_len = read(sock, &message[idx++], 1)) {
        if (read_len == -1)
        {
            error_handling("read() error");
        }
        str_len += read_len;
    }
    

    printf("Message from server: %s\n", message);
    printf("Function read call count: %d\n", str_len);
    close(sock);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}