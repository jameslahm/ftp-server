#ifndef CLIENT_H
#define CLIENT_H
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLINE 4096
#define IP_LENGTH 16

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

typedef struct
{
    struct sockaddr_in addr;
    int listenfd;
    int clifd;
} Data_Conn;

// Client struct
typedef struct
{
    // just index
    ClientId client_id;
    // User
    User *user;
    // connection socket
    int socket_fd;
    // data connection socket
    

} Client;

Client DEFAULT_CLIENT = {-1, NULL, -1, -1};

int client_add(Client *clients, int socket_fd);

void client_del(Client *clients, ClientId client_id);

#endif