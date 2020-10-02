#pragma once
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <aio.h>

#include "config.h"

typedef int ClientId;

// User struct
struct User
{
    // User username
    char *username;
    // User password
    char *password;
};

// response msg
struct CommandResponse
{
    char *message;
};

enum Data_Conn_Mode
{
    PORT,
    PASV
};

enum Data_Conn_Status
{
    READ,
    WRITE
};

// data connection
struct Data_Conn
{
    struct sockaddr_in *addr;
    int listenfd;
    int clifd;
    enum Data_Conn_Mode mode;
    struct aiocb *acb;
    enum Data_Conn_Status status;
};

// cmd_response

// Client struct
struct Client
{
    // User
    struct User *user;

    // data connection
    struct Data_Conn *data_conn;

    // cmd response
    struct CommandResponse *cmd_response;
    // connection socket
    int socket_fd;
    char *current_dir;
};

extern struct Data_Conn DEFAULT_DATA_CONN;
extern struct Client DEFAULT_CLIENT;
// default alloc client nums
extern int DEFAULT_CLIENT_ALLOC_SIZE;
// clients size
extern int clients_size;

int client_add(struct Server_RC *server_rc, int socket_fd);

void client_del(struct Client *clients, ClientId client_id);

void client_data_conn_del(struct Client *client);