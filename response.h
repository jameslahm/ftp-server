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

int cmd_response_add(CommandResponse *cmd_responses, CommandResponse *cmd_response);

void cmd_response_del(CommandResponse *cmd_responses, int cmd_response_id);

CommandResponse *make_response(ClientId client_id, int code, char *message);

CommandResponse *make_multiline_response(ClientId client_id, int code, char message[][MAXLINE]);

#endif
