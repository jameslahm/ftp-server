#ifndef HANDLE_H
#define HANDLE_H

#include "client.h"

typedef struct
{
    // command type
    char *type;
    // command args
    char *args;
} Command;

CommandResponse *handle_command(Client *client, char *buf);

#endif