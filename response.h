#pragma once

#include <sys/socket.h>
#include "client.h"

struct Command_Response *make_response(int code, char *message);

struct Command_Response *make_multiline_response(int code, char message[][MAXLINE]);
