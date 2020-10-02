#ifndef RESPONSE_H
#define RESPONSE_H

#include <sys/socket.h>
#include "client.h"

typedef struct
{
    // client id
    ClientId client_id;
    // message
    char *message;

} CommandResponse;

CommandResponse DEFAULT_CMD_RESPONSE = {-1, NULL};

void make_response(CommandResponse *cmd_reponses, int clifd, int code, char *message);

#endif
