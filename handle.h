#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <aio.h>
#include <sys/socket.h>
#include <errno.h>

#include "client.h"
#include "response.h"
#include "list_dir.h"

typedef struct
{
    // command type
    char *type;
    // command args
    char *args;
} Command;

struct Command_Response *handle_command(struct Client *client, char *buf, struct Server_RC *server_rc);

int handle_read(struct Client *client, struct Server_RC *server_rc);

int handle_write(struct Client *client, struct Server_RC *server_rc);

void clear_data_conn(struct Client *client, struct Server_RC *server_rc);

bool check_data_conn(struct Client *client);

