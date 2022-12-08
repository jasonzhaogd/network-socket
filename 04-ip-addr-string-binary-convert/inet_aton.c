/**
 * inet_aton 函数与 inet_addr 函数在功能上完全相同，也是将字符串形式 ip 地址转换为 32 位网络字节序整数
 * 只不过该函数是直接操作 in_addr 结构体，更方便，所以使用频率更高。其声明为：
 * - int inet_aton(const char *string, struct in_addr *addr);
 *     返回值有点反常规：成功返回 1（true），失败返回 0（false）
*/

#include <stdio.h>
#include <arpa/inet.h>

int main(void)
{
    char *addr = "1.2.3.4";
    struct sockaddr_in addr_inet;

    if (!inet_aton(addr, &addr_inet.sin_addr))
        printf("Invalid addr\n");
    else
        printf("Network ordered integer addr: %#x \n", addr_inet.sin_addr.s_addr);

    return 0;
}
