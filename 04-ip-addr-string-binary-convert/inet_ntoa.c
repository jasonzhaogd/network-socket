/**
 * inet_ntoa 函数与 inet_aton 函数正好相反，它是把网络字节序整数型 IP 地址转换为我们熟悉的 字符串形式。
 * - char * inet_ntoa(struct in_addr addr);
 *      失败时返回 -1
 * 但是调用时要小心：返回值类型为char指针，返回字符串地址意味着字符串已经保存到了内存空间，但该函数未要求先分配内存，
 * 而是在内部申请了内存并保存了字符串。也就是说，调用完本函数后应该立即将字符串复制到其他内存空间，否则再次调用此函数时，
 * 就可能覆盖之前内部保存的字符串。
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main(void)
{
    struct sockaddr_in addr1, addr2;
    char *str_ptr;
    char str_arr[20];

    addr1.sin_addr.s_addr = htonl(0x01020304);
    addr2.sin_addr.s_addr = htonl(0x05060708);

    str_ptr = inet_ntoa(addr1.sin_addr);
    strcpy(str_arr, str_ptr);
    printf("Dotted-Decimal notation1: %s , it's binary format in network ordered: %#x\n", str_arr, addr1.sin_addr.s_addr);

    // 虽然没有显示赋值，但函数还是把结果放到入到 str_ptr 指向的空间。
    inet_ntoa(addr2.sin_addr);
    printf("Dotted-Decimal notation2: %s , it's binary format in network ordered: %#x\n", str_ptr, addr2.sin_addr.s_addr);

    return 0;
}