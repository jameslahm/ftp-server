#pragma once

#include <sys/socket.h>
#include "client.h"

struct CommandResponse *make_response(int code, char *message);

struct CommandResponse *make_mark_response(int code, char *message);

struct CommandResponse *make_multiline_response(int code, char message[][MAXLINE]);
