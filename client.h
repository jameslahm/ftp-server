#ifndef CLIENT_H
#define CLIENT_H
#include <stdlib.h>

typedef int ClientId;

// User struct
typedef struct
{
    // User username
    char *username;
    // User password
    char *password;
} User;

User DEFAULT_USER = {NULL, NULL};

// Client struct
typedef struct
{
    // User
    User *user;
    // connection socket
    int socket_fd;
    // data connection socket
    int data_socket_fd;
} Client;

Client DEFAULT_CLIENT = {NULL, -1, -1};

int client_add(Client *clients, int socket_fd);

void client_del(Client *clients, ClientId client_id);

#endif