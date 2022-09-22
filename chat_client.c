#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"

struct server_sock
{
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

int main(void)
{
    struct server_sock s;
    s.inbuf = 0;
    int exit_status = 0;

    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0)
    {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1)
    {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("client: connect");
        close(s.sock_fd);
        exit(1);
    }

    char *buf = NULL; // Buffer to read name from stdin
    int name_valid = 0;
    while (!name_valid)
    {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        size_t name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0)
        {
            perror("getline");
            fprintf(stderr, "Error reading username.\n");
            free(buf);
            exit(1);
        }

        if (name_len - 1 > MAX_NAME)
        {
            // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
        }
        else
        {
            // Replace LF+NULL with CR+LF
            buf[name_len - 1] = '\r';
            buf[name_len] = '\n';
            if (write_to_socket(s.sock_fd, buf, name_len + 1))
            {
                fprintf(stderr, "Error sending username.\n");
                free(buf);
                exit(1);
            }
            name_valid = 1;
            free(buf);
        }
    }

    /*
     * See here for why getline() is used above instead of fgets():
     * https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152445
     */

    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */

    // printf("first step\n");
    int max_fd = s.sock_fd;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds); // stdin
    FD_SET(s.sock_fd, &fds);
    // printf("\t\ts.sock_fd [%d]\n", s.sock_fd);
    char buf_stdin[2560];
    // char buf_socket[65536];
    strncpy(buf_stdin, "\0", 2560);
    // strncpy(buf_socket, "\0", 65536);
    // printf("zu se ?\n");
    // int status = read(stdin, buf, BUF_SIZE - 1);
    // int status_socket = read(s.sock_fd, buf_socket, BUF_SIZE - 1);
    // printf("bu zu se\n");
    // printf("first step end\n");

    /*
     * Step 2: Using select, monitor the socket for incoming messages
     * from the server and stdin for data typed in by the user.
     */
    while (1)
    {
        // printf("loop start\n");
        // printf("Part 2\n");
        int nready = select(max_fd + 1, &fds, NULL, NULL, NULL);
        // printf("[nready '%d']", nready);
        if (nready == -1)
        {
            perror("client: select");
            exit(1);
        }
        // printf("Part 2 End\n");
        /*
         * Step 3: Read user-entered message from the standard input
         * stream. We should read at most MAX_USR_MSG bytes at a time.
         * If the user types in a message longer than MAX_USR_MSG,
         * we should leave the rest of the message in the standard
         * input stream so that we can read it later when we loop
         * back around.
         *
         * In other words, an oversized messages will be split up
         * into smaller messages. For example, assuming that
         * MAX_USR_MSG is 10 bytes, a message of 22 bytes would be
         * split up into 3 messages of length 10, 10, and 2,
         * respectively.
         *
         * It will probably be easier to do this using a combination of
         * fgets() and ungetc(). You probably don't want to use
         * getline() as was used for reading the user name, because
         * that would read all the bytes off of the standard input
         * stream, even if it exceeds MAX_USR_MSG.
         */
        if (FD_ISSET(0, &fds))
        {
            // printf("third step start\n");
            // printf("waiting for input\n");
            // printf("\tbuf_stdin [%s]\n", buf_stdin);
            scanf("%[^\n]", buf_stdin);
            setbuf(stdin, NULL);
            // printf("\tbuf_stdin [%s]\n", buf_stdin);
            // int flag = 0;
            int length = strlen(buf_stdin);
            // printf("\tlength [%d]\n", length);
            while (length > 0)
            {
                if (length < MAX_USER_MSG - 3)
                {
                    // printf("[0]buf_stdin [%s]\n", buf_stdin);
                    // printf("\tPart 3 length [%d]\n", length);
                    buf_stdin[length] = '\r';
                    // printf("\tPart 3 length [%d]\n", length);
                    buf_stdin[length + 1] = '\n';
                    // printf("\tPart 3 length [%d]\n", length);
                    // printf("\tPart 3 length [%d]\n", length);
                    buf_stdin[length + 2] = '\0';
                    // printf("[0]strlen(buf_stdin) [%ld]\n", strlen(buf_stdin));
                    // printf("[0]buf_stdin [%s]\n", buf_stdin);
                    write_to_socket(s.sock_fd, buf_stdin, strlen(buf_stdin));
                    break;
                }
                else
                {
                    // printf("[1]buf_stdin [%s]\n", buf_stdin);
                    memmove(buf_stdin + MAX_USER_MSG, buf_stdin + MAX_USER_MSG - 3, length - MAX_USER_MSG + 3);
                    buf_stdin[MAX_USER_MSG - 2] = ' ';
                    buf_stdin[MAX_USER_MSG - 1] = ' ';
                    buf_stdin[MAX_USER_MSG] = ' ';
                    char temp[2560];
                    strncpy(temp, "\0", 2560);
                    memmove(temp, buf_stdin + MAX_USER_MSG + 1, length - MAX_USER_MSG + 3);
                    // printf("[2]temp [%s]\n", temp);
                    // printf("[2]buf_stdin [%s]\n", buf_stdin);
                    buf_stdin[MAX_USER_MSG - 2] = '\r';
                    buf_stdin[MAX_USER_MSG - 1] = '\n';
                    buf_stdin[MAX_USER_MSG] = '\0';
                    strncpy(buf_stdin + MAX_USER_MSG, "\0", length - MAX_USER_MSG + 3);
                    // printf("[3]buf_stdin [%s]\n", buf_stdin);
                    write_to_socket(s.sock_fd, buf_stdin, MAX_USER_MSG);
                    length = length - MAX_USER_MSG + 3 - 1;
                    strncpy(buf_stdin, "\0", 2560);
                    memmove(buf_stdin, temp, length);
                    // sleep(1);
                    for (int i = 0; i < 80000; i++)
                        ;
                    // printf("[4]buf_stdin [%s]\n", buf_stdin);
                    // printf("[4]length [%d]\n", length);
                }
                // if (length > 0)
                // {
                // memmove(buf_stdin, buf_stdin + MAX_USER_MSG, length);
                // printf("try to continue\n");
                // }
                // else
                // if (length < 0)
                // {
                //     // printf("try to empty\n");
                //     strncpy(buf_stdin, "\0", 65536);
                //     length = 0;
                //     flag = 1;
                // }
            }
            strncpy(buf_stdin, "\0", 2560);
            length = 0;
            // flag = 1;
            // printf("third step end\n");
            // if (flag)
            // {
            FD_ZERO(&fds);
            FD_SET(0, &fds); // stdin
            FD_SET(s.sock_fd, &fds);
            continue;
            // }
            if (length == 0)
            {
                // printf("shouldn't be here\n");
                setbuf(stdin, NULL);
                FD_ZERO(&fds);
                FD_SET(0, &fds); // stdin
                FD_SET(s.sock_fd, &fds);
                continue;
            }
        }

        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
        if (FD_ISSET(s.sock_fd, &fds))
        {
            // printf("fourth step start\n");
            char *msg;
            int server_closed = read_from_socket(s.sock_fd, s.buf, &s.inbuf);
            int state = get_message(&msg, s.buf, &(s.inbuf));
            // Loop through buffer to get complete message(s)
            // while (server_closed == 2 && state == 0)
            // {
            //     printf("While Loop\n");
            //     server_closed = read_from_socket(s.sock_fd, s.buf, &s.inbuf);
            //     state = get_message(&msg, s.buf, &(s.inbuf));
            //     // strncat(msg, s.buf, MAX_PROTO_MSG - 1);
            //     // int data_len = strlen(write_buf);
            // }
            if (server_closed == 1)
            {
                printf("Server disconnected\n");
                close(s.sock_fd);
                exit(exit_status);
            }
            if (state == 0)
            {
                printf("%s\n", msg);
                free(msg);
                // printf("fourth step end\n");
                FD_ZERO(&fds);
                FD_SET(0, &fds); // stdin
                FD_SET(s.sock_fd, &fds);
                continue;
            }
            // printf("read done\n");
        }
    }

    close(s.sock_fd);
    exit(exit_status);
}
