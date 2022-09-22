#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* gethostname */
#include <netinet/in.h> /* struct sockaddr_in */

#include "socket.h"

/*
 * Initialize a server address associated with the required port.
 * Create and setup a socket for a server to listen on.
 */
void setup_server_socket(struct listen_sock *s)
{
    // printf("entry setup_server_socket\n");
    if (!(s->addr = malloc(sizeof(struct sockaddr_in))))
    {
        perror("malloc");
        exit(1);
    }
    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(SERVER_PORT);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0)
    {
        perror("server socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
                            (const char *)&on, sizeof(on));
    if (status < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0)
    {
        perror("server: bind");
        close(s->sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0)
    {
        perror("server: listen");
        close(s->sock_fd);
        exit(1);
    }
}

/* Insert Tutorial 10 helper functions here. */

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int inbuf)
{
    // printf("entry find_network_newline\n");
    // printf("find_network_newline buf '%s'\n", buf);
    const char tag[2] = "\r\n";
    for (int i = 0; i < strlen(buf); i++)
        if (memcmp(buf + i, tag, 2) == 0)
        {
            // printf("find_network_newline index '%d'\n", i + 1);
            return i + 1;
        }
    // printf("find_network_newline index '-1'\n");
    return -1;
}

/*
 * Reads from socket sock_fd into buffer *buf containing *inbuf bytes
 * of data. Updates *inbuf after reading from socket.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf)
{
    // printf("entry read_from_socket\n");
    // printf("read_from_socket inbuf '%d'\n", *inbuf);
    char current[BUF_SIZE];
    strncpy(current, "\0", BUF_SIZE);
    // printf("read_from_socket buf '%s'\n", buf);
    // printf("\t\tread_from_socket strlen(buf) '%ld'\n", strlen(buf));
    // printf("2 buf siz '%ld'\n", sizeof(buf));
    int status = read(sock_fd, current, sizeof(current) - strlen(buf) - 1);
    // printf("read_from_socket current '%s'\n", current);
    // printf("\t\tread_from_socket strlen(current) '%ld'\n", strlen(current));
    // printf("2 cur siz '%ld'\n", sizeof(current));
    strncat(buf, current, strlen(current));
    // printf("read_from_socket buf '%s'\n", buf);
    if (status == -1)
        return -1;
    else if (status == 0)
        return 1;
    *inbuf = find_network_newline(buf, status);
    // printf("read_from_socket inbuf '%d'\n", *inbuf);
    if (*inbuf != -1)
        return 0;
    else
        return 2;
}

/*
 * Search src for a network newline, and copy complete message
 * into a newly-allocated NULL-terminated string **dst.
 * Remove the complete message from the *src buffer by moving
 * the remaining content of the buffer to the front.
 *
 * Return 0 on success, 1 on error.
 */
int get_message(char **dst, char *src, int *inbuf)
{
    // printf("entry get_message\n");
    if (*inbuf == 0)
        return 1;
    // printf("\t\tget_message inbuf '%d'\n", *inbuf);
    // printf("get_message src '%s'\n", src);
    *dst = malloc(*inbuf);
    strncpy(*dst, "\0", *inbuf);
    // printf("\t\tget_message dst '%s'\n", *dst);
    memmove(*dst, src, *inbuf - 1);
    // printf("get_message dst '%s'\n", *dst);
    // printf("\t\tget_message src '%s'\n", src);
    // printf("\t\tget_message inbuf '%d'\n", *inbuf);
    // printf("\t\tget_message sizeof(src) '%ld'\n", sizeof(src));
    // printf("\t\tget_message sizeof(src) - inbuf '%ld'\n", sizeof(src) - *inbuf);
    if ((sizeof(src) - *inbuf) <= 0 || (*inbuf + 1) >= sizeof(src))
    {
        // printf("1\n");
        strncpy(src, "\0", (*inbuf) + 1);
    }
    else
    {
        // printf("2\n");
        memmove(src, src + *inbuf + 1, sizeof(src) - *inbuf);
    }
    // printf("\t\tget_message src '%s'\n", src);
    *inbuf = 0;
    return 0;
}

/* Helper function to be completed for Tutorial 11. */

/*
 * Write a string to a socket.
 *
 * Return 0 on success.
 * Return 1 on error.
 * Return 2 on disconnect.
 *
 * See Robert Love Linux System Programming 2e p. 37 for relevant details
 */
int write_to_socket(int sock_fd, char *buf, int len)
{
    // printf("entry write_to_socket\n");
    int status = write(sock_fd, buf, len);
    // printf("write_to_socket status [%d]\n", status);
    if (status == -1)
        return 1;
    else if (status == 0)
        return 2;
    else
        return 0;
}
