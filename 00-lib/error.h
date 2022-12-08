#ifndef _ERROR_H
#define _ERROR_H 1

#include <stdio.h>
#include <stdlib.h>

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

#endif  /* error.h */
