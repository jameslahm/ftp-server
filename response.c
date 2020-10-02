#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

#include "response.h"


struct CommandResponse *make_response(int code, char *message)
{
    int message_length = strlen(message);
    int code_length = snprintf(NULL, 0, "%d ", code);
    char *buf = (char *)malloc(sizeof(char) * (code_length + message_length + 1));
    snprintf(buf, code_length + 1, "%d ", code);
    snprintf(buf + code_length, message_length + 1, "%s", message);
    struct CommandResponse *cmd_response = (struct CommandResponse *)malloc(sizeof(struct CommandResponse));
    cmd_response->message = buf;
    return cmd_response;
}

struct CommandResponse *make_multiline_response(int code, char message[][MAXLINE])
{
    int code_length = snprintf(NULL, 0, "%d ", code);

    char *buf = (char *)malloc(sizeof(char) * MAXLINE);
    int buf_base = 0;

    int i = 0;
    while (strlen(message[i]) != 0)
    {
        int message_length = strlen(message[i]);
        char *buf = (char *)realloc(buf, sizeof(char) * (code_length + message_length + 1));
        if (strlen(message[i]) != 0)
        {
            snprintf(buf + buf_base, code_length + 1, "%d-", code);
        }
        else
        {
            snprintf(buf + buf_base, code_length + 1, "%d", code);
        }
        snprintf(buf + buf_base + code_length, message_length + 1, "%s", message[i]);
        buf_base += code_length + message_length + 1;
    }
    struct CommandResponse *cmd_response = (struct CommandResponse *)malloc(sizeof(struct CommandResponse));
    cmd_response->message = buf;
    return cmd_response;
}