/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
// doit func
void doit(int connfd);
// some rio func without exit imediately
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
void Rio_writen_w(int fd, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
// func for request headers
void read_requesthdrs(rio_t *rp);
// client error msg
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/*
 * main - Main routine for the proxy program
 */
// int main(int argc, char **argv)
// {
//     /* Check arguments */
//     if (argc != 2)
//     {
//         fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
//         exit(0);
//     }

//     exit(0);
// }
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2)
    {
        fprintf(stderr, "usg: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s , %s)\n", client_hostname, client_port);
        doit(connfd);
        Close(connfd);
    }

    exit(0);
}

/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0)
    {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':')
    {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    }
    else
    {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL)
    {
        pathname[0] = '\0';
    }
    else
    {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 12, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %zu", time_str, a, b, c, d, uri, size);
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
    {
        fprintf(stderr, "Rio_readlineb error: %s\n", strerror(errno));
        return 0;
    }

    return rc;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
        fprintf(stderr, "Rio_writen error: %s\n", strerror(errno));
}

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
    {
        fprintf(stderr, "Rio_readn error: %s\n", strerror(errno));
        return 0;
    }

    return n;
}

void doit(int connfd)
{
    // size_t n;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    char request[MAXLINE];
    int uri_state = 0;
    int content_length = 0;
    rio_t rio;

    // read from client
    Rio_readinitb(&rio, connfd);
    if (Rio_readlineb_w(&rio, buf, MAXLINE) == 0)
    {
        fprintf(stderr, "Readline Wrong!\n");
        Close(connfd);
        return;
    }
    printf("Request headers:\n");
    printf("%s", buf);

    if (sscanf(buf, "%s %s %s", method, uri, version) != 3)
    {
        fprintf(stderr, "Input Wrong!\n");
        Close(connfd);
        return;
    }
    printf("method: %s  uri: %s  version: %s\n", method, uri, version);

    uri_state = parse_uri(uri, hostname, pathname, port);
    if (uri_state < 0)
    {
        fprintf(stderr, "Uri Wrong!\n");
        Close(connfd);
        return;
    }
    printf("hostname: %s  pathname: %s  port: %s\n", hostname, pathname, port);

    // read_requesthdrs(&rio);

    // write to server
    int clientfd;
    rio_t client_rio;

    clientfd = Open_clientfd(hostname, port);
    Rio_readinitb(&client_rio, clientfd);

    // request
    sprintf(request, "%s /%s %s\r\n", method, pathname, version);
    printf("request: %s", request);
    Rio_writen_w(clientfd, request, strlen(request));

    // request headers
    while (Rio_readlineb_w(&rio, buf, MAXLINE) != 0)
    {
        printf("%s", buf);
        Rio_writen_w(clientfd, buf, strlen(buf));
        // printf("%s", buf);
        if (strcmp(buf, "\r\n") == 0)
            break;

        // get the content length if method is not get
        if (strncmp(buf, "Content-Length: ", 16) == 0)
        {
            sscanf(buf + 16, "%d", &content_length);
            printf("content_length: %d\n", content_length);
        }
    }

    // request body
    if (strcmp(method, "GET") != 0)
    {
        // read and write the content byte by byte
        if (content_length != 0)
        {
            for (int i = 0; i < content_length; i++)
            {
                if (Rio_readlineb_w(&rio, buf, 1) != 0)
                {
                    Rio_writen_w(clientfd, buf, 1);
                }
            }
        }
        // if content_length == 0, read "\r\n" then end
        else
        {
            while (Rio_readlineb_w(&rio, buf, MAXLINE) != 0)
            {
                printf("%s", buf);
                Rio_writen_w(clientfd, buf, strlen(buf));

                if (strcmp(buf, "\r\n") == 0)
                    break;
            }
        }
    }

    content_length = 0;

    // response
    // response headers
    while (Rio_readlineb_w(&client_rio, buf, MAXLINE) != 0)
    {
        printf("response: %s", buf);
        Rio_writen_w(connfd, buf, MAXLINE);

        if (strcmp(buf, "\r\n") == 0)
            break;

        if (strncmp(buf, "Content-Length: ", 16) == 0)
        {
            sscanf(buf + 16, "%d", &content_length);
            printf("content_length: %d\n", content_length);
        }
    }

    // response body
    if (content_length != 0)
    {
        printf("content not zero: ");
        for (int i = 0; i < content_length; i++)
        {
            if (Rio_readlineb_w(&client_rio, buf, 1) != 0)
            {
                Rio_writen_w(connfd, buf, 1);
            }
        }
    }
    else
    {
        printf("content zero: ");
        while (Rio_readlineb_w(&client_rio, buf, MAXLINE) != 0)
        {
            printf("%s", buf);
            Rio_writen_w(connfd, buf, strlen(buf));

            if (strcmp(buf, "\r\n") == 0)
                break;
        }
    }

    // Close(connfd);
    Close(clientfd);
    return;
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
// {
//     char buf[MAXLINE], body[MAXBUF];

//     /* build the HTTP response body */
//     sprintf(body, "<html><title>Error</title>");
//     sprintf(body, "%s<body bgcolor="
//                   "ffffff"
//                   ">\r\n",
//             body);
//     sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
//     sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
//     sprintf(body, "%s<hr><em>Tiny Web server</em>\r\n", body);

//     /* print the HTTP response */
//     sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Content-type: text/html\r\n");
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
//     Rio_writen(fd, buf, strlen(buf));
//     Rio_writen(fd, body, strlen(body));
// }
