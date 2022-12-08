/**
 * 断开连接的过程，先执关闭连接一方的套接字 经过四次挥手过程后并非立即消除，
 * 而是要经历一段时间的 TIME_WAIT 状态，这个时间段是 2MSL。 也就是 两倍的 MSL 时间
 *    MSL = Maximum Segment Lifetime, the time a TCP segment can exist in the internetwork system.
 *    和大多数 BSD 派生的系统一样，Linux 系统里有一个硬编码的字段，名称为TCP_TIMEWAIT_LEN，其值为 60 秒。
 *    也就是说，Linux 系统停留在 TIME_WAIT 的时间为固定的 60 秒。
 * 在 TIME_WAIT 期间，相应的端口是正在使用状态，如果此时再调用 bind 函数就会报错。
 *
 * 先关闭连接的一方是指调用 close 函数，或通过 Ctrl+C 终止程序，或其他方式强制终止程序（由OS关闭套接字）
 * 如果先关闭连接的一方是客服端就问题不大，反正每次建立连接的端口号是随机分配的，
 * 但是如果先关闭连接的是服务器，特别是遇到异常退出的情况，这时重启服务时，就会报端口被占用的错误。
 */

/**
 * 到底为什么会有 TIME-WAIT 状态呢？要看下面的状态图
 *     TCP Peer A                                           TCP Peer B
 * 1.  ESTABLISHED                                          ESTABLISHED
 * 2.  (Close)
 *     FIN-WAIT-1  --> <SEQ=100><ACK=300><CTL=FIN,ACK>  --> CLOSE-WAIT
 * 3.  FIN-WAIT-2  <-- <SEQ=300><ACK=101><CTL=ACK>      <-- CLOSE-WAIT
 * 4.                                                       (Close)
 *     TIME-WAIT   <-- <SEQ=300><ACK=101><CTL=FIN,ACK>  <-- LAST-ACK
 * 5.  TIME-WAIT   --> <SEQ=101><ACK=301><CTL=ACK>      --> CLOSED
 * 6.  (2 MSL)
 *     CLOSED
 * 假设第5步A发送最后一条ACK信息后立即消除套接字，但最后这条ACK信息在路上丢失了，未能到达B，
 * B会以为第4步发的 FIN,ACK 消息A没收到，就会重发第4步的 FIN,ACK 消息，但此时A已经是完全终止的状态，
 * B就会一直重发，而不会结束。
 * 而如果有 TIME-WAIT 状态，A如果在第5步消息发出后的 time wait 期间又收到和第4步一样的 FIN,ACK 信息，
 * A就会重发第5步的信息，除非过了 MSL 的时间段，还没收到 B 要求重发的消息，A的 time wait 状态结束，彻底释放地址端口号。
 *
 * 注意，上面提到的 B 以为第4步消息丢失而重发第四步消息时，A每次收到重发的第四步消息，它的 time wait 的计时器会重启，
 * 如果在网络状况不理想或某些极端情况下，time wait 状态就会一直持续下去，A 和 B 两个都会僵死在这里。
 */

/**
 * TIME_WAIT 的危害:
 * TIME_WAIT 状态的存在是为了实现 TCP 的容错性，是它的有点。但是也有危害：
 * - 第一是内存资源占用，这个目前看来不是太严重，基本可以忽略。
 * - 第二是对端口资源的占用，一个 TCP 连接至少消耗一个本地端口。要知道，端口资源也是有限的，一般可以开启的端口为 32768～61000 ，
 *    也可以通过net.ipv4.ip_local_port_range指定，如果 TIME_WAIT 状态过多，会导致无法创建新连接。
*/

/**
 * 解决方案就是在套接字的选项中设置 SO_REUSEADDR 为 1，即可将 time wait 状态下的端口号重新分配给新的套接字
 * SO_REUSEADDR 默认值是0
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

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size, optlen;

    char message[BUF_SIZE];

    int option, str_len;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    /* 要打开 SO_REUSEADDR，请把下面三行注释取消 */
    // optlen = sizeof(option);
    // option = 1;
    // setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");
    
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

    while ((str_len = read(clnt_sock, message, BUF_SIZE)) != 0)
    {
        write(clnt_sock, message, str_len);
        write(1, message, str_len);
    }
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

// 客户端可以用 05/echo_client.c
