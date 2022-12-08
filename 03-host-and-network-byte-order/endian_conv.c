/**
 * 字节序转换（Endian Conversions）的系列函数 定义在   <arpa/inet.h>
 * - unsigned short htons(unsigned short);
 * - unsigned short ntohs(unsigned short);
 * - unsigned long htonl(unsigned long);
 * - unsigned long ntohl(unsigned long);
 *  h 代表 host
 *  n 代表 network
 *  s 代表 short
 *  l 代表 long
 */

#include <stdio.h>
#include <arpa/inet.h>

int main(void)
{
    unsigned short host_port = 0x1234;
    unsigned short net_port;
    unsigned long host_addr = 0x12345678;
    unsigned long net_addr;

    net_port = htons(host_port);
    net_addr = htonl(host_addr);

    printf("Host ordered port: %#x\n", host_port);
    printf("Network ordered port: %#x\n", net_port);
    printf("Host ordered addr: %#lx\n", host_addr);
    printf("Network ordered addr: %#lx\n", net_addr);

    return 0;
}