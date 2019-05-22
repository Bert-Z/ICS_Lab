#include "csapp.h"

int main(int argc, char **argv)
{
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int  flags;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Getaddrinfo(argv[1], NULL, &hints, &listp);

    flags = NI_NUMERICHOST;
    for (p = listp; p; p = p->ai_next)
    {
        Getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        printf("%s\n", buf);
    }

    Freeaddrinfo(listp);

    exit(0);
}
