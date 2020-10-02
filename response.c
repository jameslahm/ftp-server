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

int cmd_response_add(CommandResponse *cmd_responses, ClientId client_id, char *buf)
{
    if (cmd_responses == NULL || cmd_responses[cmd_responses_size - 1].client_id != -1)
    {
        cmd_response_alloc(cmd_responses);
    }
    for (int i = 0; i < cmd_responses_size; i++)
    {
        if (cmd_responses[i].client_id == -1)
        {
            cmd_responses[i].client_id = client_id;
            cmd_responses[i].message = buf;
            return i;
        }
    }
}

void cmd_response_del(CommandResponse *cmd_responses, int cmd_response_id)
{
    int i = cmd_response_id;
    cmd_responses[i].client_id = -1;
    free(cmd_responses[i].message);
    cmd_responses[i].message = NULL;
    return;
}

void make_response(CommandResponse *cmd_responses, ClientId client_id, int code, char *message)
{
    int message_length = strlen(message);
    int code_length = snprintf(NULL, 0, "%d ", code);
    char *buf = (char *)malloc(sizeof(char) * (code_length + message_length + 1));
    snprintf(buf, code_length + 1, "%d ", buf);
    snprintf(buf + code_length, message_length + 1, "%s", buf);
    cmd_response_add(cmd_responses, client_id, buf);
}