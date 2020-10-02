#include "handle.h"


Command *parse_command(char *buf)
{
    char *type = (char *)malloc(sizeof(char) * MAXLINE);
    bzero(type, MAXLINE);
    char *args = (char *)malloc(sizeof(char) * MAXLINE);
    bzero(args, MAXLINE);
    sscanf(buf, "%s %s", type, args);
    Command *cmd = (Command *)malloc(sizeof(Command));
    cmd->type = type;
    cmd->args = args;
    return cmd;
}

void parse_address(char *args, char *ip, int *port)
{
    char ip_1[4] = {0};
    char ip_2[4] = {0};
    char ip_3[4] = {0};
    char ip_4[4] = {0};
    int port_1;
    int port_2;

    sscanf(args, "%[^,],%[^,],%[^,],%[^,],%d,%d", ip_1, ip_2, ip_3, ip_4, &port_1, &port_2);

    snprintf(ip, IP_LENGTH, "%s.%s.%s.%s", ip_1, ip_2, ip_3, ip_4);

    *port = 256 * port_1 + port_2;
}

char *stringify_address(struct sockaddr_in *addr)
{
    char *ip_tmp = inet_ntoa(addr->sin_addr);
    char *ip = (char *)malloc(sizeof(IP_LENGTH));
    bzero(ip, IP_LENGTH);
    strncpy(ip, ip_tmp, IP_LENGTH);
    int i = 0;
    while (ip[i] != '\0')
    {
        if (ip[i] == '.')
        {
            ip[i] = ',';
        }
    }
    int port = ntohs(addr->sin_port);
    int port_1 = port / 256;
    int port_2 = port % 256;
    char *buf = (char *)malloc(MAXLINE);
    snprintf(buf, MAXLINE, "%s,%d,%d", ip, port_1, port_2);
    free(ip);
    return buf;
}

void handle_read(struct Client *client, struct Server_RC *server_rc)
{
    struct Data_Conn *data_conn = client->data_conn;
    int nread = aio_return(data_conn->acb);
    write(data_conn->clifd, (char *)data_conn->acb->aio_buf, nread);
    if (nread == data_conn->acb->aio_nbytes)
    {
        data_conn->acb->aio_offset += data_conn->acb->aio_offset + nread;
        aio_read(data_conn->acb);
    }
    else
    {
        close(data_conn->clifd);
        free(data_conn->acb);
        client->cmd_response = make_response(226, "Transfer complete\r\n");
        FD_SET(client->socket_fd, &server_rc->all_wset);
    }
}

void handle_write(struct Client *client, struct Server_RC *server_rc)
{
    struct Data_Conn *data_conn = client->data_conn;
    aio_return(data_conn->acb);
    data_conn->acb->aio_nbytes = BUFSIZ;
    int nwrite = read(data_conn->clifd, (char *)data_conn->acb->aio_buf, data_conn->acb->aio_nbytes);
    if (nwrite == 0)
    {
        close(data_conn->clifd);
        free(data_conn->acb);
        client->cmd_response = make_response(226, "Transfer complete\r\n");
        FD_SET(client->socket_fd, &server_rc->all_wset);
    }
    else
    {
        data_conn->acb->aio_nbytes = nwrite;
        aio_write(data_conn->acb);
    }
}

struct CommandResponse *handle_command(struct Client *client, char *buf, struct Server_RC *server_rc)
{
    Command *cmd = parse_command(buf);

    // handle User command
    if (strcmp(cmd->type, "USER") == 0)
    {
        if (strcmp(cmd->args, "anonymous") == 0)
        {
            struct User *user = (struct User *)malloc(sizeof(struct User));
            user->username = (char *)malloc(strlen(cmd->args) + 1);
            bzero(user->username, strlen(cmd->args) + 1);
            strncpy(user->username, cmd->args, strlen(cmd->args));
            client->user = user;
            return make_response(331, "Guest login ok,send your complete e-mail address as password\r\n");
        }
    }

    // handle Pass command
    if (strcmp(cmd->type, "PASS") == 0)
    {
        struct User *user = client->user;
        user->password = (char *)malloc(strlen(cmd->args) + 1);
        bzero(user->password, strlen(cmd->args) + 1);
        strncpy(user->password, cmd->args, strlen(cmd->args));
        char message[][MAXLINE] = {
            "\r\n"
            "Welcome to",
            "School of Software",
            "FTP Archives at ftp.ssast.org",
            "\r\n",
            "This site is provided as a public service by School of\r\n",
            "Software. Use in violation of any applicable laws is strictly\r\n",
            "prohibited. We make no guarantees, explicit or implicit, about the\r\n",
            "contents of this site. Use at your own risk.\r\n",
            "\r\n",
            "Guest login ok, access restrictions apply.\r\n"
            "\0"};
        return make_multiline_response(230, message);
    }

    if (strcmp(cmd->type, "SYST") == 0)
    {
        return make_response(215, "UNIX Type: I\r\n");
    }

    if (strcmp(cmd->type, "TYPE") == 0)
    {
        if (strcmp(cmd->args, "I") == 0)
        {
            return make_response(200, "Type set to I\r\n");
        }
    }

    if (strcmp(cmd->type, "PORT") == 0)
    {
        client_data_conn_del(client);

        char *ip = (char *)malloc(sizeof(char) * IP_LENGTH);
        bzero(ip, sizeof(char) * IP_LENGTH);
        int *port = (int *)malloc(sizeof(int));
        parse_address(cmd->args, ip, port);

        struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

        bzero(addr, sizeof(*addr));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(*port);
        inet_aton(ip, &addr->sin_addr);

        struct Data_Conn *data_conn = (struct Data_Conn *)malloc(sizeof(struct Data_Conn));
        *data_conn = DEFAULT_DATA_CONN;
        data_conn->addr = addr;
        data_conn->mode = PORT;
        client->data_conn = data_conn;
        return make_response(200, "PORT command successful\r\n");
    }

    if (strcmp(cmd->type, "PASV") == 0)
    {
        client_data_conn_del(client);

        struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

        bzero(addr, sizeof(*addr));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(0);
        addr->sin_addr.s_addr = htonl(INADDR_ANY);

        struct Data_Conn *data_conn = (struct Data_Conn *)malloc(sizeof(struct Data_Conn));
        *data_conn = DEFAULT_DATA_CONN;
        data_conn->addr = addr;
        data_conn->mode = PASV;

        data_conn->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        bind(data_conn->listenfd, (struct sockaddr *)data_conn->addr, sizeof(struct sockaddr_in));
        listen(data_conn->listenfd, 10);
        if (data_conn->listenfd > server_rc->maxfd)
        {
            server_rc->maxfd = data_conn->listenfd;
        }

        socklen_t sockaddr_in_length = sizeof(struct sockaddr_in);
        getsockname(data_conn->listenfd, (struct sockaddr *)data_conn->addr, &sockaddr_in_length);

        char *address = stringify_address(data_conn->addr);
        char *buf = (char *)malloc(MAXLINE);
        snprintf(buf, MAXLINE, "Entering Passive Mode (%s)\r\n", address);
        free(address);
        return make_response(227, buf);
    }

    if (strcmp(cmd->type, "RETR") == 0)
    {
        client->data_conn->status = READ;
        int filename_length = strlen(client->current_dir) + strlen(cmd->args) + 1;
        char *filename = (char *)(malloc(strlen(client->current_dir) + strlen(cmd->args)) + 1);
        bzero(filename, filename_length + 1);
        snprintf(filename, filename_length, "%s/%s", client->current_dir, cmd->args);
        struct aiocb *acb = (struct aiocb *)malloc(sizeof(struct aiocb));
        bzero(acb, sizeof(*acb));
        int fd = open(filename, O_RDONLY);
        acb->aio_fildes = fd;
        acb->aio_buf = (char *)malloc(BUFSIZ + 1);
        acb->aio_offset = 0;
        acb->aio_nbytes = BUFSIZ;

        client->data_conn->acb = acb;
        aio_read(acb);

        if (client->data_conn->mode == PORT)
        {
            client->data_conn->clifd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (client->data_conn->listenfd > server_rc->maxfd)
            {
                server_rc->maxfd = client->data_conn->listenfd;
            }
            int flags = fcntl(client->data_conn->clifd, F_GETFL, 0);
            fcntl(client->data_conn->clifd, F_SETFL, flags | O_NONBLOCK);

            connect(client->data_conn->clifd, (struct sockaddr *)client->data_conn->addr, sizeof(struct sockaddr_in));
            FD_SET(client->data_conn->clifd, &server_rc->all_wset);
            // client->data_conn->filename = cmd->args;
            return make_response(50, "Opening Binary mode data connection\r\n");
        }
        if (client->data_conn->mode == PASV)
        {
            FD_SET(client->data_conn->listenfd, &server_rc->all_rset);
            // client->data_conn->filename = cmd->args;
            return make_response(120, "Opening Binary mode data connection\r\n");
        }
    };

    if (strcmp(cmd->type, "STOR") == 0)
    {
        client->data_conn->status = WRITE;

        int filename_length = strlen(client->current_dir) + strlen(cmd->args) + 1;
        char *filename = (char *)(malloc(strlen(client->current_dir) + strlen(cmd->args)) + 1);
        bzero(filename, filename_length + 1);
        snprintf(filename, filename_length, "%s/%s", client->current_dir, cmd->args);
        struct aiocb *acb = (struct aiocb *)malloc(sizeof(struct aiocb));
        bzero(acb, sizeof(*acb));
        int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT);
        acb->aio_fildes = fd;
        acb->aio_buf = (char *)malloc(BUFSIZ + 1);
        acb->aio_nbytes = 0;

        client->data_conn->acb = acb;
        aio_write(acb);
        if (client->data_conn->mode == PORT)
        {
            client->data_conn->clifd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (client->data_conn->listenfd > server_rc->maxfd)
            {
                server_rc->maxfd = client->data_conn->listenfd;
            }
            int flags = fcntl(client->data_conn->clifd, F_GETFL, 0);
            fcntl(client->data_conn->clifd, F_SETFL, flags | O_NONBLOCK);

            connect(client->data_conn->clifd, (struct sockaddr *)client->data_conn->addr, sizeof(struct sockaddr_in));
            FD_SET(client->data_conn->clifd, &server_rc->all_rset);
            // client->data_conn->filename = cmd->args;
            return make_response(50, "Opening Binary mode data connection\r\n");
        }
        if (client->data_conn->mode == PASV)
        {
            FD_SET(client->data_conn->listenfd, &server_rc->all_rset);
            // client->data_conn->filename = cmd->args;
            return make_response(120, "Opening Binary mode data connection\r\n");
        }
    };
    free(cmd->args);
    free(cmd->type);
    free(cmd);
    return NULL;
}
