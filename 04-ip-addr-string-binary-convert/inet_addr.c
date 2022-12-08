/**
 * inet_addr 函数把 字符串形式的 IP 地址转换为 32 位整数，同时进行了网络字节序转换
 * 它定义在 <arpa/inet.h> 中，声明为：
 * - in_addr_t inet_addr(const char * string);
 * 返回值类型正好是对应到地址结构体的嵌套字段的类型一致： sockaddr_in.sin_addr.s_addr
 * 失败时返回: INADDR_NONE
 */

#include <stdio.h>
#include <arpa/inet.h>

int main(void)
{
    char *addr1 = "1.2.3.4";
    char *addr2 = "1.2.3.999";

    unsigned long conv_addr = inet_addr(addr1);
    if (conv_addr == INADDR_NONE)
        printf("Invalid addr\n");
    else
        printf("Network ordered integer addr: %#lx\n", conv_addr);

    conv_addr = inet_addr(addr2);
    if (conv_addr == INADDR_NONE)
        printf("Invalid addr\n");
    else
        printf("Network ordered integer addr: %#lx\n", conv_addr);

    return 0;
}