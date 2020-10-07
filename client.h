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
struct Command_Response
{
    int code;
    char *message;
};

enum Data_Conn_Mode
{
    PORT,
    PASV
};

enum Command_Status
{
    LIST,
    RETR,
    STOR,
    APPE,
    IDLE
};

// data connection
struct Data_Conn
{
    struct sockaddr_in *addr;
    int listenfd;
    int clifd;
    enum Data_Conn_Mode mode;
    struct aiocb *acb;

    char* buf;
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
    struct Command_Response *cmd_response;
    // connection socket
    int socket_fd;
    time_t last_check_time;

    char *current_dir;
    char *current_filename;
    int offset;

    enum Command_Status command_status;
};

extern struct User DEFAULT_USER;
extern struct Data_Conn DEFAULT_DATA_CONN;
extern struct Client DEFAULT_CLIENT;
// default alloc client nums
extern int DEFAULT_CLIENT_ALLOC_SIZE;
// clients size
extern int clients_size;

int client_add(struct Server_RC *server_rc, int socket_fd);

void client_del(struct Client *clients, ClientId client_id);
