#pragma once
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#define MAXLINE 4096
#define IP_LENGTH 16

struct Server_RC
{
    char *root_dir;
    int port;
    fd_set all_rset;
    fd_set all_wset;
    int maxfd;
    struct Client *clients;
    char* ip;
};

char* copy(char* src);