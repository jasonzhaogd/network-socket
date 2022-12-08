
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
    int sock;
    char msg1[] = "Hi !";
    char msg2[] = "I'm another UDP host!";
    char msg3[] = "Nice to meet you.";

    struct sockaddr_in peer_addr;

    if (argc != 3)
    {
        printf("Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(argv[1]);
    peer_addr.sin_port = htons(atoi(argv[2]));

    connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    // sendto(sock, msg1, strlen(msg1), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    // sendto(sock, msg2, strlen(msg2), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    // sendto(sock, msg3, strlen(msg3), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    write(sock, msg1, strlen(msg1));
    write(sock, msg2, strlen(msg2));
    write(sock, msg3, strlen(msg3));

    close(sock);
    return 0;
}
