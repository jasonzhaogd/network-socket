
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    FILE *fp;
    char buf[BUF_SIZE];

    int read_cnt;

    if (argc != 3)
    {
        printf("Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    fp = fopen("receive.dat", "wb");

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    while((read_cnt = read(sock, buf, BUF_SIZE)) != 0)
        fwrite(buf, 1, read_cnt, fp);
    fclose(fp);
    puts("Received file data.");

    write(sock, "Thank you", 10);
    close(sock);
    return 0;
}
