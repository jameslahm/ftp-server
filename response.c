#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

#include "response.h"

// default alloc cmd responses nums
static int DEFAULT_CMD_RESPONSE_ALLOC_SIZE = 10;
// cmd_responses size
static int cmd_responses_size = 0;

void cmd_response_alloc(CommandResponse *cmd_responses)
{
    if (cmd_responses == NULL)
    {
        cmd_responses = malloc(DEFAULT_CMD_RESPONSE_ALLOC_SIZE * sizeof(CommandResponse));
    }
    else
    {
        cmd_responses = realloc(cmd_responses, (cmd_responses_size + DEFAULT_CMD_RESPONSE_ALLOC_SIZE) * sizeof(CommandResponse));
    }
    if (cmd_responses == NULL)
    {
        log_error("cmd_response_alloc: can't alloc for cmd_responses array");
    }
    for (int i = cmd_responses_size; i < cmd_responses_size + DEFAULT_CMD_RESPONSE_ALLOC_SIZE; i++)
    {
        cmd_responses[i] = DEFAULT_CMD_RESPONSE;
    }
    cmd_responses_size += DEFAULT_CMD_RESPONSE_ALLOC_SIZE;
}

int cmd_response_add(CommandResponse *cmd_responses, CommandResponse *cmd_response)
{
    if (cmd_responses == NULL || cmd_responses[cmd_responses_size - 1].client_id != -1)
    {
        cmd_response_alloc(cmd_responses);
    }
    for (int i = 0; i < cmd_responses_size; i++)
    {
        if (cmd_responses[i].client_id == -1)
        {
            cmd_response[i] = *cmd_response;
            return i;
        }
    }
    free(cmd_response);
}

void cmd_response_del(CommandResponse *cmd_responses, int cmd_response_id)
{
    int i = cmd_response_id;
    cmd_responses[i].client_id = -1;
    free(cmd_responses[i].message);
    cmd_responses[i].message = NULL;
    return;
}

CommandResponse *make_response(ClientId client_id, int code, char *message)
{
    int message_length = strlen(message);
    int code_length = snprintf(NULL, 0, "%d ", code);
    char *buf = (char *)malloc(sizeof(char) * (code_length + message_length + 1));
    snprintf(buf, code_length + 1, "%d ", buf);
    snprintf(buf + code_length, message_length + 1, "%s", buf);
    CommandResponse *cmd_response = (CommandResponse *)malloc(sizeof(CommandResponse));
    cmd_response->client_id = client_id;
    cmd_response->message = buf;
    return cmd_response;
}

CommandResponse *make_multiline_response(ClientId client_id, int code, char message[][MAXLINE])
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
            snprintf(buf + buf_base, code_length + 1, "%d-", buf);
        }
        else
        {
            snprintf(buf + buf_base, code_length + 1, "%d", buf);
        }
        snprintf(buf + buf_base + code_length, message_length + 1, "%s", buf);
        buf_base += code_length + message_length + 1;
    }
    CommandResponse *cmd_response = (CommandResponse *)malloc(sizeof(CommandResponse));
    cmd_response->client_id = client_id;
    cmd_response->message = buf;
    return cmd_response;
}