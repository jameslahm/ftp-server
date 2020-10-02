#ifndef HANDLE_H
#define HANDLE_H

#include "client.h"

typedef struct
{
    // command type
    char code;
    // command args
    char *buf;
} Command;

void handle_command(Client *client, char *buf, int nread);

#endif