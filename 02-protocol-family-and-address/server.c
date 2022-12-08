/**
 * 详细了解构建 socket 中涉及到的：
 * 1. 协议栈
 * 2. 地址数据结构
 * 3. 套接字类型
 * 4. <sys/socket.h> 中 socket, bind, listen, accept 各函数参数和返回值
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
    int serv_sock;
    int clnt_sock;

    /**
     * 结构体 sockaddr_in 定义在 <arpa/inet.h>，定义为：
     * struct sockaddr_in
     * {
     *    sa_family_t     sin_family;     // 地址族（Address Family）
     *    uint16_t        sin_port;       // 16 位的 端口号
     *    struct in_addr  sin_addr;       // 32 位 IP 地址
     *    char            sin_zero[8];    // 不使用
     * }
     * 其中用到了另外一个结构体 in_addr ,它的定义如下：
     * struct in_addr
     * {
     *    in_addr_t       s_addr;         // 32 位 IPv4 地址，是一个 uint32_t, 网络字节序
     * }
     * 以上两个结构体具体使用的细节知识点，看下面给地址赋值处注释
     */
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size; // 定义在 <sys/socket.h>

    char message[] = "Hello World!";

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    /**
     * socket() 函数, PF_INET, SOCK_STREAM 等常量都定义在 <sys/socket.h>，socket() 函数的声明为：
     * int socket(int domain, int type, int protocol);
     *  - domain：套接字使用的协议族（Protocol Family）
     *      PF_INET     IPv4 IP protocol family
     *      PF_INET6    IPv6 IP protocol family
     *      PF_LOCAL    本地通讯的 UNIX 协议族
     *      PF_PACKET   底层套接字的协议族
     *      PF_IPX      IPX Novell 协议族
     *  - type：套接字数据传输类型，决定数据传输方式
     *      SOCK_STREAM 面向连接的套接字：Sequenced, reliable, connection-based byte streams.
     *      SOCK_DGRAM  面向消息的套接字：Connectionless, unreliable datagrams of fixed maximum length.
     *  - protocol：计算机间通讯中使用的协议信息
     *      其实前面两个参数基本可以确定套接字类型，所以大多数情况下此参数都是0。
     *      这是因为 IPv4 下的 SOCK_STREAM 传输方式对应的协议信息只有 IPPROTO_TCP
     *            而 IPv4 下的 SOCK_DGRAM 传输方式对应的协议信息只有 IPPROTO_UDP
     *      所以对于常见的 tcp 和 udp 套接字，我们都直接把第三个参数赋值为0，glibc 自己都可以确定具体的协议信息。
     *      注意1：对于某些协议族中一种传输方式有多个协议实现的，指定具体的第三个参数就有意义了。
     *      注意2：IPPROTO_TCP 和 IPPROTO_UDP 是定义在 <arpa/inet.h> 中的，而不是 <sys/socket.h> 中。
     */
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket() error");
    }

    // 后面会操作地址结构的特殊字段，在此之前，要先清零，保存其他没有被特殊设置的字段为0
    memset(&serv_addr, 0, sizeof(serv_addr)); // 定义在 <string.h>

    /**
     * 每种协议族适用的地址族不同，比如 IPv4使用 4 字节的地址族，而 IPv6使用 16 字节地址族：
     * - AF_INET   IPv4网络协议中使用的地址族
     * - AF_INET6  IPv6网络协议中使用的地址族
     * - AF_LOCAL  本地通讯中采用的 UNIX 协议的地址族
     */
    serv_addr.sin_family = AF_INET;
    /**
     * IP地址占 4 字节，由网络地址和主机地址组成。按照 网络地址和主机地址 占用的位数不同又分为 A、B、C、D类型。
     * 使用 INADDR_ANY 常数，可自动获取本服务器的 IP 地址，
     * 如果本服务器有多个 IP 地址，只要端口号一致，就可以从不同 IP 地址接收数据。
     * 因此，服务器端多采用这种方式，而客户端中要指定具体的服务器 IP 地址（参考 client.c）
     */
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /**
     * 16位的端口号，重点是，它以网络字节序保存，字节序的例子在 03 目录
     * 端口号占 2 字节，可分配的端口号范围是 0-65535，但是 0-1023 的端口是知名端口（Well-known Port），一般分配给特定应用程序。
     * 一般而言，大于 5000 的端口可以作为我们自己应用程序的端口使用。
     * 虽然通一台机器的端口号不能重复，但是 TCP 套接字和 UDP 套接字不会共用端口号，是允许重复的，因为端口号就是在同一操作系统内为区分不同套接字而设置的。
     * 如果这里设置端口号为0，就相当于把端口的选择权交给操作系统内核来处理，操作系统内核会根据一定的算法选择一个空闲的端口
     */
    serv_addr.sin_port = htons(atoi(argv[1]));
    /**
     * serv_addr.sin_zero 字段不需要特别设置，它只是为了使结构体 sockadd_in 的大小于 sockaddr 结构体保持一致
     * 这里它必须设置为0，所以上面会调用 memset 函数批量设置0
     */

    /**
     * bind 函数将第二个参数指定的地址信息分配给第一个参数中的相应的套接字
     * 第二个参数期望得到的是 sockaddr 结构体变量地址.  sockaddr 结构体定义为：
     * struct sockaddr
     * {
     *    sa_family_t   sin_family;    // 地址族（Address Family）
     *    char          sa_data[14];   // 地址信息，包括了 IP地址 和 端口号，剩余部分应填0
     * }
     * 虽然接收的是通用地址格式，实际上传入的参数可能是 IPv4、IPv6 或者本地套接字格式。比如 IPv4 就是 sockaddr_in，
     * bind 函数会根据 len 字段判断传入的参数 addr 该怎么解析。其实可以定义为 bind(int fd, void * addr, socklen_t len)
     * 不过 BSD 设计套接字的时候大约是 1982 年，那个时候的 C 语言还没有void *的支持，
     * 为了解决这个问题，BSD 的设计者们创造性地设计了通用地址格式来作为支持 bind 和 accept 等这些函数的参数。
     * 对于使用者来说，每次需要将 IPv4、IPv6 或者本地套接字格式转化为通用套接字格式，就像下面的 IPv4 套接字地址格式的例子一样：
     * 对于实现者来说，可根据该地址结构的前两个字节(sin_family字段)判断出是哪种地址。
     * 为了处理长度可变的结构，需要读取函数里的第三个参数，也就是 len 字段，这样就可以对地址进行解析和判断了。
     */
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("bind() error");
    }

    /**
     * 调用 listen 函数，进入等待连接请求状态，客户端才能调用 connect 函数，函数声明为：
     * - int listen(int sock, int backlog);
     *     backlog: 未完成连接队列的大小，这个参数的大小决定了可以接收的并发数目。这个参数越大，并发数目理论上也会越大。但是参数过大也会占用过多的系统资源
     *    -> 成功时返回0，失败时返回 -1
     */
    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen error");
    }

    clnt_addr_size = sizeof(clnt_addr);
    /**
     * 当连接请求队列不为空时，此函数就会被执行，处理成功后，就会返回系统为服务这个客户端请求而生成的socket，
     * - int accept(int sock, struct sockaddr * addr, socklen_t *addrlen);
     *     sock:    服务器套接字
     *     addr:    保存发起连接请求的客服端地址信息的变量地址
     *     addrlen: 存放上面 addr 结构体的长度的变量地址，函数调用完后，该变量即被填入客服端地址长度
     *   -> 成功时返回创建的套接字文件描述符，失败时返回 -1。 
     *   
     *   ### 如果等待连接队列为空，会进入阻塞状态，直到有新的连接请求(客户端调用 connect 函数)到来。
     *   ### 返回的套接字是由系统自动创建的，并自动与发起连接请求的客服端建立连接。
     *   ### 最后一个参数是要传地址，因为这个函数设计时要支持通用性，所以要支持进来的连接对应的地址数据结构长度是不同的
     *              在连接完成后，确定了对方的地址类型后能确定其长度，这里就要修改它的值。
     *              但是我们这里的例子没用到这个地址，有些程序中可能会对地址进行逻辑处理。
     */
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
    {
        error_handling("accept() eeror");
    }

    // 下面几个的低级io操作定义在 <unistd.h>
    write(clnt_sock, message, sizeof(message));
    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/**
 * AF_ 表示的含义是 Address Family，是地址族
 * PF_ 的意思是 Protocol Family，也就是协议族
 * 用 AF_xxx 这样的值来初始化 socket 地址，用 PF_xxx 这样的值来初始化 socket。
 * 我们在 <sys/socket.h> 头文件中可以清晰地看到，这两个值本身就是一一对应的，例如：
 *      #define AF_INET   PF_INET
 *      #define AF_LOCAL  PF_LOCAL
 *      #define AF_UNIX   PF_UNIX
*/