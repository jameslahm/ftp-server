#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "client.h"
#include "handle.h"
#include "response.h"
#include "log.h"

#define MAXLINE 4096

int main(int argc, char **argv)
{
	// clients
	Client *clients = NULL;

	// command responses
	CommandResponse *cmd_responses = NULL;

	// default root dir: /tmp
	char *rootDir = "/tmp";

	// default port: 12345
	int port = 12345;

	// buf
	char buf[MAXLINE];

	// get options include port and root dir
	struct option long_options[] =
		{
			{"root", required_argument, NULL, 'r'},
			{"port", required_argument, NULL, 'p'},
			{0, 0, 0, 0}};

	int arg;
	int option_index = 0;
	while ((arg = getopt_long(argc, argv, ":r:p:", long_options, &option_index)) != -1)
	{
		switch (arg)
		{
		case 'r':
		{
			rootDir = optarg;
			break;
		}
		case 'p':
		{
			sscanf(optarg, "%d", &port);
			break;
		}
		case ':':
		{
			printf("%s need a value", long_options[option_index].name);
			break;
		}
		case '?':
		{
			printf("Unknown option\n");
		}
		default:
		{
			break;
		}
		}
	}

	log_info("Set rootDir to %s...\n", rootDir);
	log_info("Set port to %d...\n", port);

	// listen socket
	int listenfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{

		return 1;
	}

	// set socket addr
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //监听"0.0.0.0"

	// bind socket
	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		log_error("bind: %s", strerror(errno));
		return 1;
	}

	if (listen(listenfd, 10) == -1)
	{
		log_error("listen: %s", strerror(errno));
		return 1;
	}

	int n, clifd, i, nread;
	int maxi = -1;
	int maxfd = listenfd;
	fd_set rset, all_rset, wset, all_wset;
	FD_ZERO(&all_rset);
	FD_ZERO(&all_wset);
	FD_SET(listenfd, &all_rset);
	struct timeval timeout = {0, 0};

	while (1)
	{
		// Check read socket
		rset = all_rset;
		wset = all_wset;
		if ((n = select(maxfd + 1, &rset, &wset, NULL, &timeout)) < 0)
		{
			log_err("select: %s", strerror(errno));
		}

		if (FD_ISSET(listenfd, &rset))
		{
			// accept socket connection
			if ((clifd = accept(listenfd, NULL, NULL)) < 0)
			{
				log_err("accept: %s", strerror(errno));
			}

			// add client
			i = client_add(clients, clifd);

			// add clifd to all_rset
			FD_SET(clifd, &all_rset);

			// add clifd to all_wset
			if (clifd > maxfd)
			{
				maxfd = clifd;
			}
			if (i > maxi)
			{
				maxi = i;
			}
			log_info("new connection %d", clifd);

			// add cmd response
			make_response(cmd_responses, i, 220, "ftp.ssast.org FTP server ready");

			// add clifd to all_wset
			FD_SET(clifd, &all_wset);
			continue;
		}

		// check client can read
		for (i = 0; i <= maxi; i++)
		{
			if ((clifd = clients[i].socket_fd) < 0)
			{
				continue;
			}
			if (FD_ISSET(clifd, &rset))
			{
				if ((nread = read(clifd, buf, MAXLINE)) < 0)
				{
					printf("Error read: %d", clifd);
				}
				// client close
				else if (nread == 0)
				{
					client_del(clients, i);
					FD_CLR(clifd, &all_rset);
					close(clifd);
				}
				else
				{
					handle_command(&clients[i], buf, nread);
				}
			}
		}

		// check client can write
		for (i = 0; i < cmd_responses; i++)
		{
			ClientId client_id = cmd_responses[i].client_id;
			int socket_fd = clients[client_id].socket_fd;
			if (FD_ISSET(socket_fd, &wset))
			{
				// write socket
				write(socket_fd, cmd_responses[i].message, strlen(cmd_responses[i].message));
			}
		}

		// check file read complete
	}

	close(listenfd);
}
