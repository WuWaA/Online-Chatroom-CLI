#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"
#include "chat_helpers.h"

/*
 * Send a string to a client.
 *
 * Input buffer must contain a NULL-terminated string. The NULL
 * terminator is replaced with a network-newline (CRLF) before
 * being sent to the client.
 *
 * On success, return 0.
 * On error, return 1.
 * On client disconnect, return 2.
 */
int write_buf_to_client(struct client_sock *c, char *buf, int len)
{
    // printf("entry write_buf_to_client\n");
    const int length = strlen(buf);
    const char tag_space = ' ';
    // printf("\t\twrite_buf_to_client buf [%s]\n", buf);
    // printf("\t\twrite_buf_to_client len [%d]\n", len);
    // printf("\t\twrite_buf_to_client length [%d]\n", length);
    for (int i = 0; i < length; i++)
        if (*(buf + i) == tag_space)
        {
            memmove((buf + i + 1), (buf + i), length - i);
            *(buf + i) = ':';
            break;
        }
    buf[length + 1] = '\r';
    buf[length + 2] = '\n';
    buf[length + 3] = '\0';
    // printf("\t\twrite_buf_to_client buf [%s]\n", buf);
    // printf("\t\twrite_buf_to_client len [%d]\n", len);
    // printf("\t\twrite_buf_to_client length [%d]\n", length);
    int status = write_to_socket(c->sock_fd, buf, len + 3);
    return status;
}

/*
 * Remove client from list. Return 0 on success, 1 on failure.
 * Update curr pointer to the new node at the index of the removed node.
 * Update clients pointer if head node was removed.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients)
{
    // printf("entry remove_client\n");
    // struct client_sock *all = *clients;
    // while (all != NULL)
    // {
    //     printf("[name '%s']\n", (*all).username);
    //     all = (all)->next;
    // }
    // printf("remove curr\n");
    // **curr = *(**curr).next;
    // all = *clients;
    // while (all != NULL)
    // {
    //     printf("[name '%s']\n", (*all).username);
    //     all = (all)->next;
    // }
    if (*clients == NULL || *curr == NULL) // (*clients == NULL) ((**clients).username == NULL)
        return 1;                          // Couldn't find the client in the list, or empty list

    // printf("test\n");
    // printf("(*temp)->sock_fd [%d]\n", (*temp)->sock_fd);
    // printf("(*curr)->sock_fd [%d]\n", (*curr)->sock_fd);
    // printf("temp == NULL ? [%d]\n", temp == NULL);
    // printf("*temp == NULL ? [%d]\n", *temp == NULL);
    // printf("\t\ttemp name '%s'\n", (**temp).username);
    // printf("\t\tcurr name '%s'\n", (**curr).username);
    // printf("\t\tclients name '%s'\n", (**clients).username);

    if ((*curr)->next != NULL)
    {
        // printf("remove mode 1\n");
        // printf("\t\ttemp next name '%s'\n", (**temp).next->username);
        // printf("\t\tcurr next name '%s'\n", (**curr).next->username);
        free((**curr).username);
        // free((**temp).username);
        // printf("\t\tcurrent\n");
        // printf("\t\tcurrent null? '%d'\n", (*curr) == NULL);
        // if ((*curr) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*curr)->username);
        // printf("\t\tclients\n");
        // printf("\t\tclients null? '%d'\n", (*clients) == NULL);
        // if ((*clients) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*clients)->username);
        // *curr = (*curr)->next;
        (**curr) = *(**curr).next;
        // printf("\t\tcurrent\n");
        // printf("\t\tcurrent null? '%d'\n", (*curr) == NULL);
        // if ((*curr) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*curr)->username);
        // printf("\t\tclients\n");
        // printf("\t\tclients null? '%d'\n", (*clients) == NULL);
        // if ((*clients) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*clients)->username);
        // *temp = (*temp)->next; // (**temp) = (**temp).next
        // printf("exit remove_client success\n");
        // // printf("\t\tcurrent\n");
        // printf("\t\tcurrent null? '%d'\n", (*curr) == NULL);
        // if ((*curr) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*curr)->username);
        // // printf("\t\tclients\n");
        // printf("\t\tclients null? '%d'\n", (*clients) == NULL);
        // if ((*clients) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*clients)->username);
        // struct client_sock **all = clients;
        // while (*all != NULL)
        // {
        //     printf("\t\tallname '%s'\n", (**all).username);
        //     *all = (*all)->next;
        // }
        // struct client_sock *all = *clients;
        // while (all != NULL)
        // {
        //     printf("[name '%s']\n", (*all).username);
        //     all = (all)->next;
        // }
        // printf("done\n");
        return 0;
    }
    else if ((*curr)->next == NULL)
    {
        // printf("remove mode 2\n");
        struct client_sock **temp = clients;
        if ((*curr)->sock_fd != (*clients)->sock_fd)
        {
            while (*temp != NULL && ((*temp)->next)->sock_fd != (*curr)->sock_fd)
            {
                // printf("temp != NULL ? [%d]\n", temp != NULL);
                // printf("*temp != NULL ? [%d]\n", *temp != NULL);
                *temp = (*temp)->next;
            }
            if (*temp == NULL)
                return 1; // Couldn't find the client in the list, or empty list
            (**temp).next = NULL;
        }
        else
        {
            free((**clients).username);
            *clients = NULL;
            clients = NULL;
        }

        // free((**curr).username);
        // printf("free\n");
        // free((**temp).username);
        *curr = NULL;
        // printf("*curr\n");
        curr = NULL;
        // printf("curr\n");
        // *clients = NULL;
        // printf("*clients\n");
        // clients = NULL;
        // printf("clients\n");
        // printf("exit remove_client success\n");
        // printf("\t\tcurrent\n");
        // printf("\t\tcurrent null? '%d'\n", (*curr) == NULL);
        // if ((*curr) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*curr)->username);
        // printf("\t\tclients\n");
        // printf("\t\tclients null? '%d'\n", (*clients) == NULL);
        // if ((*clients) != NULL)
        //     printf("\t\tcurrent '%s'\n", (*clients)->username);
        // struct client_sock **all = clients;
        // while (*all != NULL)
        // {
        //     printf("\t\tallname '%s'\n", (**all).username);
        //     *all = (*all)->next;
        // }
        // struct client_sock *all = *clients;
        // while (all != NULL)
        // {
        //     printf("[name '%s']\n", (*all).username);
        //     all = (all)->next;
        // }
        // printf("done\n");
        return 0;
    }
    // if (strcmp((**curr).username, (**clients).username) == 0) // necessary?
    // (*clients) = (*clients)->next; // necessary?
    // printf("exit remove_client fail\n");
    return 1;
}

/*
 * Read incoming bytes from client.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if client socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_client(struct client_sock *curr)
{
    // printf("entry read_from_client\n");
    // int state = 1, i = 2;
    // while (state && i)
    // {
    //     i--;
    //     state = read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
    //     if (state == -1)
    //     {
    //         return -1;
    //     }
    // }
    // return 0;
    // printf("read_from_client buf '%s' '%s' '%d'\n", curr->buf, curr->username, curr->sock_fd);
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

/*
 * Set a client's user name.
 * Returns 0 on success.
 * Returns 1 on either get_message() failure or
 * if user name contains invalid character(s).
 */
int set_username(struct client_sock *curr)
{
    // printf("entry set_username\n");
    char *name;
    name = malloc(MAX_NAME);
    strncpy(name, "\0", MAX_NAME);
    get_message(&(name), curr->buf, &(curr->inbuf));
    // printf("name [%s]\n", name);

    // // find name index
    // int index = -1;
    // for (int i = 0; i < strlen(name); i++)
    //     if (name[i] == '\0')
    //     {
    //         index = i;
    //         break;
    //     }

    // // did not find name
    // if (index == -1)
    //     return 1;

    // printf("check invalid character\n");
    // check invalid character
    int length = strlen(name);
    for (int i = 0; i < length; i++)
    {
        char c = name[i];
        if (c < 32 || c > 126) // visible ascii characters
        {
            free(name);
            return 1;
        }
    }

    // assign to username
    curr->username = malloc(length + 1);
    strncpy(curr->username, "\0", length + 1);
    // get_message(&(curr->username), name, &length);
    // memcpy(curr->username, name, length);
    // printf("malloc success\n");
    memmove(curr->username, name, length);
    // printf("memmove success\n");
    // printf("name [%s]\n", name);
    // printf("\t\tusername [%ld]\n", strlen(curr->username));
    // printf("\t\tbuf [%ld]\n", strlen(curr->buf));
    free(name);
    // printf("free success\n");

    return 0;
}
