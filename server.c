#include "server.h"

char *get_real_ip()
{
	struct ifaddrs *addrs;
	getifaddrs(&addrs);
	struct ifaddrs *tmp = addrs;
	char *res = NULL;

	while (tmp)
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			char *tmp_buf = inet_ntoa(pAddr->sin_addr);
			if (strcmp(tmp_buf, "127.0.0.1") != 0)
			{
				res = copy(tmp_buf);
				break;
			}
		}
		tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);

	if (res == NULL)
	{
		res = (char *)malloc(IP_LENGTH);
		bzero(res, IP_LENGTH);
		sprintf(res, "127.0.0.1");
	}
	return res;
}

void init_socket(int socket_id)
{
	// int keepalive = 1;
	// int keepidle = 600;
	// int keepinterval = 5;
	// int keepcount = 3;
	// setsockopt(socket_id, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
	// setsockopt(socket_id, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
	// setsockopt(socket_id, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
	// setsockopt(socket_id, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));
}

int main(int argc, char **argv)
{
	struct Server_RC server_rc;
	// clients
	server_rc.clients = NULL;

	// default root dir: /tmp
	server_rc.root_dir = "/tmp";

	// default port: 21
	server_rc.port = 21;

	server_rc.ip = get_real_ip();

	// buf
	char buf[MAXLINE+1];

	// get options include port and root dir
	// struct option long_options[] =
	// 	{
	// 		{"root", required_argument, NULL, 'r'},
	// 		{"port", required_argument, NULL, 'p'},
	// 		{0, 0, 0, 0}};

	// int arg;
	// int option_index = 0;
	// while ((arg = getopt_long(argc, argv, ":r:p:", long_options, &option_index)) != -1)
	// {
	// 	switch (arg)
	// 	{
	// 	case 'r':
	// 	{
	// 		server_rc.root_dir = optarg;
	// 		break;
	// 	}
	// 	case 'p':
	// 	{
	// 		sscanf(optarg, "%d", &server_rc.port);
	// 		break;
	// 	}
	// 	case ':':
	// 	{
	// 		printf("%s need a value", long_options[option_index].name);
	// 		break;
	// 	}
	// 	case '?':
	// 	{
	// 		printf("Unknown option\n");
	// 	}
	// 	default:
	// 	{
	// 		break;
	// 	}
	// 	}
	// }
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-root") == 0)
		{
			if (i + 1 == argc)
			{
				log_warn("need specify root argument!");
				continue;
			}
			// server_rc.root_dir=argv[i+1];
			server_rc.root_dir = (char *)malloc(PATH_MAX);
			realpath(argv[i + 1], server_rc.root_dir);
		}
		if (strcmp(argv[i], "-port") == 0)
		{
			if (i + 1 == argc)
			{
				log_warn("need specify port argument!");
				continue;
			}
			sscanf(argv[i + 1], "%d", &server_rc.port);
		}
	}

	log_info("set root_dir to %s", server_rc.root_dir);
	log_info("set port to %d", server_rc.port);

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
	addr.sin_port = htons(server_rc.port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int reuse_addr_flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_flag, sizeof(reuse_addr_flag));

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

	log_info("listening...");

	int max_client_id = -1;
	server_rc.maxfd = listenfd;
	fd_set rset, wset;
	FD_ZERO(&server_rc.all_rset);
	FD_ZERO(&server_rc.all_wset);
	FD_SET(listenfd, &server_rc.all_rset);

	while (1)
	{
		// check dead client
		log_info("check dead client...");
		for (int i = 0; i <= max_client_id; i++)
		{
			struct Client *client = &server_rc.clients[i];
			if (client->socket_fd < 0)
			{
				continue;
			}
			time_t now = time(NULL);
			if (now - client->last_check_time >= 600)
			{
				log_info("find client %d dead", client->socket_fd);
				client->cmd_response = make_response(421, "Connection time out\r\n");
			}
		}

		// Check read socket
		rset = server_rc.all_rset;
		wset = server_rc.all_wset;

		// struct timeval timeout = {5, 0};

		log_info("current max clients: %d", max_client_id + 1);
		log_info("select fds (maxfd: %d)", server_rc.maxfd);

		int n;
		if ((n = select(server_rc.maxfd + 1, &rset, &wset, NULL, NULL)) < 0)
		{
			log_error("select: %s", strerror(errno));
		}

		log_info("%d ready", n);

		log_info("check new client");
		if (FD_ISSET(listenfd, &rset))
		{

			int clifd;
			// accept socket connection
			if ((clifd = accept(listenfd, NULL, NULL)) < 0)
			{
				log_error("accept: %s", strerror(errno));
			}

			// add client
			log_info("add client %d", clifd);
			init_socket(clifd);

			int i = client_add(&server_rc, clifd);
			server_rc.clients[i].current_dir = copy(server_rc.root_dir);

			// add clifd to all_rset
			FD_SET(clifd, &server_rc.all_rset);

			// add clifd to all_wset
			if (clifd > server_rc.maxfd)
			{
				server_rc.maxfd = clifd;
				log_info("set maxfd %d", server_rc.maxfd);
			}
			if (i > max_client_id)
			{
				max_client_id = i;
			}

			// add cmd response
			struct Command_Response *cmd_response = make_response(220, "ftp.ssast.org FTP server ready\r\n");
			server_rc.clients[i].cmd_response = cmd_response;

			// add clifd to all_wset
			FD_SET(clifd, &server_rc.all_wset);
			continue;
		}

		log_info("check clients command");

		// check client command request and response
		for (int i = 0; i <= max_client_id; i++)
		{
			int clifd;
			struct Client *client = &server_rc.clients[i];
			if ((clifd = client->socket_fd) < 0)
			{
				continue;
			}

			// check read
			if (FD_ISSET(clifd, &rset))
			{
				int nread;
				if ((nread = read(clifd, buf, MAXLINE)) < 0)
				{
					printf("Error read: %d", clifd);
				}
				// client close
				else if (nread == 0)
				{
					clear_data_conn(client, &server_rc);
					client_del(server_rc.clients, i);
					log_info("delete client (%d)", i);
					FD_CLR(clifd, &server_rc.all_rset);
					if (server_rc.maxfd == clifd)
					{
						server_rc.maxfd -= 1;
						log_info("set maxfd %d", server_rc.maxfd);
					}
					close(clifd);
				}
				else
				{
					buf[nread] = '\0';
					log_info("handle command %s", buf);
					if (client->cmd_response != NULL || client->command_status != IDLE)
					{
						log_info("shield command %s", buf);
						continue;
					}
					struct Command_Response *cmd_response = handle_command(client, buf, &server_rc);
					client->cmd_response = cmd_response;
				}
			}

			// check write
			if (FD_ISSET(clifd, &wset))
			{
				struct Command_Response *cmd_response = client->cmd_response;
				if (cmd_response != NULL)
				{
					int nwrite = write(clifd, cmd_response->message, strlen(cmd_response->message));
					if (nwrite == strlen(cmd_response->message))
					{
						log_info("response cmd: \n%s", cmd_response->message);
					}
					else
					{
						log_error("response cmd: \n%s", strerror(errno));
					}
					if (client->command_status != IDLE && client->cmd_response->code==226)
					{
						if (client->data_conn == NULL)
						{
							log_info("set command status %d", IDLE);
							client->command_status = IDLE;
						}
					}
					client->cmd_response = NULL;
					FD_CLR(clifd, &server_rc.all_wset);
					if (cmd_response->code == 221 || cmd_response->code == 421)
					{
						clear_data_conn(client, &server_rc);
						client_del(server_rc.clients, i);
						log_info("delete client (%d)", i);
						FD_CLR(clifd, &server_rc.all_rset);
						if (server_rc.maxfd == clifd)
						{
							server_rc.maxfd -= 1;
							log_info("set maxfd %d", server_rc.maxfd);
						}
						close(clifd);
					}
					free(cmd_response->message);
					free(cmd_response);
				}
			}
		}

		log_info("check clients LIST RETR PORT again");
		// LIST RETR STOR need process again
		for (int i = 0; i <= max_client_id; i++)
		{
			struct Client *client = &server_rc.clients[i];
			if (client->socket_fd < 0)
			{
				continue;
			}

			if (client->cmd_response != NULL)
			{
				continue;
			}

			if (client->command_status != IDLE)
			{
				// process again
				time_t now = time(NULL);
				if (client->data_conn == NULL)
				{
					FD_SET(client->socket_fd, &server_rc.all_wset);
					client->cmd_response = make_response(425, "No data connection established\r\n");
					client->command_status = IDLE;
					continue;
				}
				struct Data_Conn *data_conn = client->data_conn;
				// PASV　
				if (data_conn->mode == PASV)
				{
					if (now - client->last_check_time >= 10)
					{
						if (!check_data_conn(client))
						{
							FD_SET(client->socket_fd, &server_rc.all_wset);
							client->cmd_response = make_response(425, "No data connection established\r\n");
							client->command_status = IDLE;
							clear_data_conn(client, &server_rc);
						}
						continue;
					}
				}
				// PORT connection refused
				if (data_conn->mode == PORT)
				{
					int error = 0;
					socklen_t len = sizeof(error);
					getsockopt(client->data_conn->clifd, SOL_SOCKET, SO_ERROR, &error, &len);
					if (error != 0)
					{
						log_error("%s", strerror(error));
						FD_SET(client->socket_fd, &server_rc.all_wset);
						client->cmd_response = make_response(425, "No data connection established\r\n");
						client->command_status = IDLE;
						clear_data_conn(client, &server_rc);
						continue;
					}
				}
			}
		}

		log_info("check clients data connection");

		// check client data connection
		for (int i = 0; i <= max_client_id; i++)
		{
			struct Client *client = &server_rc.clients[i];
			if (client->socket_fd < 0)
			{
				continue;
			}
			if (client->data_conn == NULL)
			{
				continue;
			}

			struct Data_Conn *data_conn = client->data_conn;

			if (data_conn->mode == PORT)
			{
				// int error = 0;
				// socklen_t len = sizeof(error);

				// PORT connection refused
				// getsockopt(data_conn->clifd, SOL_SOCKET, SO_ERROR, &error, &len);
				// if (error != 0)
				// {
				// 	log_error("%s", strerror(error));

				// 	clear_data_conn(client, &server_rc);
				// 	continue;
				// }

				if (FD_ISSET(data_conn->clifd, &wset))
				{
					int nread = handle_read(client, &server_rc);
					if (nread > 0)
					{
						log_info("data transfer to %d (%d bytes)", client->socket_fd, nread);
					}
				};
				if (FD_ISSET(data_conn->clifd, &rset))
				{
					int nwrite = handle_write(client, &server_rc);
					if (nwrite > 0)
					{
						log_info("data transfer from %d (%d bytes)", client->socket_fd, nwrite);
					}
				};
			}
			if (data_conn->mode == PASV)
			{
				if (data_conn->clifd == -1 && FD_ISSET(data_conn->listenfd, &rset))
				{
					data_conn->clifd = accept(data_conn->listenfd, NULL, NULL);
					FD_CLR(data_conn->listenfd, &server_rc.all_rset);
					if (server_rc.maxfd < data_conn->clifd)
					{
						server_rc.maxfd = data_conn->clifd;
						log_info("set maxfd %d", server_rc.maxfd);
					}

					if (client->command_status == RETR || client->command_status == LIST)
					{
						FD_SET(data_conn->clifd, &server_rc.all_wset);
						continue;
					}
					if (client->command_status == STOR || client->command_status == APPE)
					{
						FD_SET(data_conn->clifd, &server_rc.all_rset);
						continue;
					}
				}

				if (FD_ISSET(data_conn->clifd, &server_rc.all_wset))
				{
					int nread = handle_read(client, &server_rc);
					if (nread > 0)
					{
						log_info("data transfer to %d (%d bytes)", client->socket_fd, nread);
					}
				}
				if (FD_ISSET(data_conn->clifd, &server_rc.all_rset))
				{
					int nwrite = handle_write(client, &server_rc);
					if (nwrite > 0)
					{
						log_info("data transfer from %d (%d bytes)", client->socket_fd, nwrite);
					}
				}
			}
		}

		log_info("------\n");
	}

	close(listenfd);
}
