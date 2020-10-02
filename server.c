#include "server.h"

int main(int argc, char **argv)
{
	struct Server_RC server_rc;
	// clients
	server_rc.clients = NULL;

	// default root dir: /tmp
	server_rc.root_dir = "/tmp";

	// default port: 12345
	server_rc.port = 12345;

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
			server_rc.root_dir = optarg;
			break;
		}
		case 'p':
		{
			sscanf(optarg, "%d", &server_rc.port);
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

	int n, clifd, i, nread;
	int max_client_id = -1;
	int maxfd = listenfd;

	fd_set rset, wset;
	FD_ZERO(&server_rc.all_rset);
	FD_ZERO(&server_rc.all_wset);
	FD_SET(listenfd, &server_rc.all_rset);

	while (1)
	{
		// Check read socket
		rset = server_rc.all_rset;
		wset = server_rc.all_wset;

		struct timeval timeout = {5, 0};

		log_info("current clients: %d", max_client_id + 1);
		log_info("select fds");

		if ((n = select(maxfd + 1, &rset, &wset, NULL, &timeout)) < 0)
		{
			log_error("select: %s", strerror(errno));
		}

		log_info("%d ready", n);

		log_info("check new client");
		if (FD_ISSET(listenfd, &rset))
		{

			// accept socket connection
			if ((clifd = accept(listenfd, NULL, NULL)) < 0)
			{
				log_error("accept: %s", strerror(errno));
			}

			// add client
			log_info("add client %d", clifd);

			i = client_add(&server_rc, clifd);
			server_rc.clients[i].current_dir = server_rc.root_dir;

			// add clifd to all_rset
			FD_SET(clifd, &server_rc.all_rset);

			// add clifd to all_wset
			if (clifd > maxfd)
			{
				maxfd = clifd;
			}
			if (i > max_client_id)
			{
				max_client_id = i;
			}

			// add cmd response
			struct CommandResponse *cmd_response = make_response(220, "ftp.ssast.org FTP server ready");
			server_rc.clients[i].cmd_response = cmd_response;

			// add clifd to all_wset
			FD_SET(clifd, &server_rc.all_wset);
			continue;
		}

		log_info("check clients command");

		// check client command request and response
		for (i = 0; i <= max_client_id; i++)
		{
			if ((clifd = server_rc.clients[i].socket_fd) < 0)
			{
				continue;
			}

			// check read
			if (FD_ISSET(clifd, &rset))
			{
				if ((nread = read(clifd, buf, MAXLINE)) < 0)
				{
					printf("Error read: %d", clifd);
				}
				// client close
				else if (nread == 0)
				{
					client_del(server_rc.clients, i);
					FD_CLR(clifd, &server_rc.all_rset);
					close(clifd);
				}
				else
				{
					buf[nread] = '\0';
					log_info("handle command %s", buf);
					struct CommandResponse *cmd_response = handle_command(&server_rc.clients[i], buf, &server_rc);
					server_rc.clients[i].cmd_response = cmd_response;
				}
			}
			// check write
			if (FD_ISSET(clifd, &wset))
			{
				struct CommandResponse *cmd_response = server_rc.clients[i].cmd_response;
				if (cmd_response != NULL)
				{
					log_info("response command %s", cmd_response->message);
					write(clifd, cmd_response->message, strlen(cmd_response->message));
					free(cmd_response->message);
					free(cmd_response);
					server_rc.clients[i].cmd_response = NULL;
					FD_CLR(clifd, &server_rc.all_wset);
				}
			}
		}

		log_info("check clients data connection");

		// check client data connection
		for (i = 0; i <= max_client_id; i++)
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
				if (FD_ISSET(data_conn->clifd, &wset))
				{
					if (aio_error(data_conn->acb) != 0)
					{
						continue;
					}
					else
					{
						log_info("data transfer to %d", client->socket_fd);
						handle_read(client, &server_rc);
					}
				};
				if (FD_ISSET(data_conn->clifd, &rset))
				{
					if (aio_error(data_conn->acb) != 0)
					{
						continue;
					}
					else
					{
						log_info("data transfer from %d", client->socket_fd);
						handle_write(client, &server_rc);
					}
				};
			}
			if (data_conn->mode == PASV)
			{
				if (data_conn->clifd == -1 && FD_ISSET(data_conn->listenfd, &rset))
				{
					data_conn->clifd = accept(data_conn->listenfd, NULL, NULL);
					FD_CLR(data_conn->listenfd, &server_rc.all_rset);
					if (data_conn->status == READ)
					{
						FD_SET(data_conn->clifd, &server_rc.all_wset);
						continue;
					}
					else
					{
						FD_SET(data_conn->clifd, &server_rc.all_rset);
						continue;
					}
				}
				if (data_conn->status == READ)
				{
					if (FD_ISSET(data_conn->clifd, &server_rc.all_wset))
					{
						if (aio_error(data_conn->acb) != 0)
						{
							continue;
						}
						else
						{
							log_info("data transfer to %d", client->socket_fd);
							handle_read(client, &server_rc);
						}
					}
				}
				if (data_conn->status == WRITE)
				{
					if (FD_ISSET(data_conn->clifd, &server_rc.all_rset))
					{
						if (aio_error(data_conn->acb) != 0)
						{
							continue;
						}
						else
						{
							log_info("data transfer from %d", client->socket_fd);
							handle_write(client, &server_rc);
						}
					}
				}
			}
		}
	}

	close(listenfd);
}
