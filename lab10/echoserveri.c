#include "csapp.h"

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv)
{
    // int listenfd, connfd;
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    // char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2)
    {
        fprintf(stderr, "usg: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        // clientlen = sizeof(struct sockaddr_storage);
        // connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        // printf("Connected to (%s , %s)\n", client_hostname, client_port);
        // echo(connfd);
        // Close(connfd);

        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }

    exit(0);
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(Pthread_self());
    Free(vargp);
    echo(connfd);
    Close(connfd);
    return NULL;
}

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    n = Rio_readnb(&rio, buf, MAXLINE);
    // while ((n = Rio_readnb(&rio, buf, MAXLINE)) != 0)
    // {
    // printf("server received %d bytes\n", (int)n);
    printf("server received %s\n", buf);
    Rio_writen(connfd, buf, n);
    // }
}