#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "socket.h"
#include "chat_helpers.h"

int sigint_received = 0;

void sigint_handler(int code)
{
    sigint_received = 1;
}

/*
 * Wait for and accept a new connection.
 * Return -1 if the accept call failed.
 */
int accept_connection(int fd, struct client_sock **clients)
{
    // printf("entry accept_connection\n");
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int num_clients = 0;
    struct client_sock *curr = *clients;
    while (curr != NULL && num_clients < MAX_CONNECTIONS && curr->next != NULL)
    {
        curr = curr->next;
        num_clients++;
    }

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);

    if (client_fd < 0)
    {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (num_clients == MAX_CONNECTIONS)
    {
        close(client_fd);
        return -1;
    }

    struct client_sock *new_client = malloc(sizeof(struct client_sock));
    new_client->sock_fd = client_fd;
    new_client->inbuf = new_client->state = 0;
    new_client->username = NULL;
    new_client->next = NULL;
    memset(new_client->buf, 0, BUF_SIZE);
    if (*clients == NULL)
    {
        *clients = new_client;
    }
    else
    {
        curr->next = new_client;
    }

    return client_fd;
}

/*
 * Close all sockets, free memory, and exit with specified exit status.
 */
void clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status)
{
    // printf("entry clean_exit\n");
    struct client_sock *tmp;
    while (clients)
    {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp->username);
        free(tmp);
    }
    close(s.sock_fd);
    free(s.addr);
    exit(exit_status);
}

int main(void)
{
    // This line causes stdout not to be buffered.
    // Don't change this! Necessary for auto testing.
    setbuf(stdout, NULL);

    // Linked list of clients
    struct client_sock *clients = NULL;

    struct listen_sock s;
    setup_server_socket(&s);

    // Set up SIGINT handler
    struct sigaction sa_sigint;
    memset(&sa_sigint, 0, sizeof(sa_sigint));
    sa_sigint.sa_handler = sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);

    int exit_status = 0;

    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;

    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    do
    {
        // struct client_sock *all = clients;
        // printf("[clients null '%d']", all == NULL);
        // while (all != NULL)
        // {
        //     printf("[server_allname '%s']", (*all).username);
        //     if ((all)->next != NULL)
        //         all = (all)->next;
        //     else
        //         break;
        // }
        // printf("[loop_1]");
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        // printf("\nnready '%d'\n", nready);
        if (sigint_received)
            break;
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;
            perror("server: select");
            exit_status = 1;
            break;
        }

        /*
         * If a new client is connecting, create new
         * client_sock struct and add to clients linked list.
         */
        if (FD_ISSET(s.sock_fd, &listen_fds))
        {
            // printf("[sock_fd]");
            int client_fd = accept_connection(s.sock_fd, &clients);
            if (client_fd < 0)
            {
                printf("Failed to accept incoming connection.\n");
                continue;
            }
            if (client_fd > max_fd)
            {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection\n");
        }

        if (sigint_received)
            break;

        /*
         * Accept incoming messages from clients,
         * and send to all other connected clients.
         */
        struct client_sock *curr = clients;
        while (curr)
        {
            // printf("[loop_2]");
            if (!FD_ISSET(curr->sock_fd, &listen_fds))
            {
                curr = curr->next;
                continue;
            }
            int client_closed = read_from_client(curr);

            // If error encountered when receiving data
            if (client_closed == -1)
            {
                perror("read");
                client_closed = 1; // Disconnect the client
            }

            // If received at least one complete message
            // and client is newly connected: Get username
            if (client_closed == 0 && curr->username == NULL)
            {
                if (set_username(curr))
                {
                    printf("Error processing user name from client %d.\n", curr->sock_fd);
                    client_closed = 1; // Disconnect the client
                }
                else
                {
                    printf("Client %d user name is %s.\n", curr->sock_fd, curr->username);
                }
            }

            char *msg;
            // Loop through buffer to get complete message(s)
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf)))
            {
                // printf("[loop_3]");
                printf("Echoing message from %s.\n", curr->username);
                char write_buf[BUF_SIZE];
                write_buf[0] = '\0';
                strncat(write_buf, curr->username, MAX_NAME);
                strncat(write_buf, " ", MAX_NAME);
                strncat(write_buf, msg, MAX_USER_MSG);
                free(msg);
                int data_len = strlen(write_buf);

                struct client_sock *dest_c = clients;
                while (dest_c)
                {
                    // printf("[loop_4]");
                    if (dest_c != curr)
                    {
                        int ret = write_buf_to_client(dest_c, write_buf, data_len);
                        if (ret == 0)
                        {
                            printf("Sent message from %s (%d) to %s (%d).\n",
                                   curr->username, curr->sock_fd,
                                   dest_c->username, dest_c->sock_fd);
                        }
                        else
                        {
                            printf("Failed to send message to user %s (%d).\n", dest_c->username, dest_c->sock_fd);
                            if (ret == 2)
                            {
                                printf("User %s (%d) disconnected.\n", dest_c->username, dest_c->sock_fd);
                                assert(remove_client(&dest_c, &clients) == 0); // If this fails, bug exist
                                continue;
                            }
                        }
                    }
                    dest_c = dest_c->next;
                }
            }

            if (client_closed == 1)
            {
                // Client disconnected
                // Note: Never reduces max_fd when client disconnects
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                printf("Client %d disconnected\n", curr->sock_fd);
                assert(remove_client(&curr, &clients) == 0); // If this fails, bug exist
            }
            else
            {
                curr = curr->next;
            }
        }
    } while (!sigint_received);

    clean_exit(s, clients, exit_status);
}
