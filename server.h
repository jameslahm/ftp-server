#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ifaddrs.h>

#include "client.h"
#include "handle.h"
#include "response.h"
#include "log.h"