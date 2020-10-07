#include "handle.h"

char *normalize_path(struct Client *client, char *dir)
{
    char *path = (char *)malloc(strlen(dir) + strlen(client->current_dir) + 2);
    if (dir[0] == '/')
    {
        realpath(dir, path);
    }
    else
    {
        char *tmp_path = (char *)malloc(strlen(dir) + strlen(client->current_dir) + 2);
        sprintf(tmp_path, "%s/%s", client->current_dir, dir);
        realpath(tmp_path, path);
    }
    return path;
}

bool check_data_conn(struct Client *client)
{
    if (client->data_conn->mode == PORT)
    {
        int error;
        socklen_t error_len = sizeof(int);
        getsockopt(client->socket_fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
        if (error != 0)
        {
            return false;
        }
    }
    else
    {
        if (client->data_conn->clifd == -1)
        {
            return false;
        }
    }
    return true;
}

// TODO:
void clear_data_conn(struct Client *client, struct Server_RC *server_rc)
{
    struct Data_Conn *data_conn = client->data_conn;
    if (data_conn != NULL)
    {
        FD_CLR(data_conn->clifd, &server_rc->all_wset);
        FD_CLR(data_conn->clifd, &server_rc->all_rset);

        if (server_rc->maxfd == data_conn->clifd)
        {
            server_rc->maxfd -= 1;
        }
        close(data_conn->clifd);

        data_conn->clifd = -1;

        free(data_conn->addr);

        if (client->data_conn->mode == PASV)
        {
            FD_CLR(data_conn->listenfd, &server_rc->all_rset);
            FD_CLR(data_conn->listenfd, &server_rc->all_wset);
            if (server_rc->maxfd == data_conn->listenfd)
            {
                server_rc->maxfd -= 1;
            }
            close(data_conn->listenfd);
        }

        free(data_conn->buf);

        if (data_conn->acb != NULL)
        {
            free(data_conn->acb);
        }
        free(data_conn);
    }
    client->data_conn = NULL;
}

struct Command_Response *init_data_conn(struct Client *client, struct Server_RC *server_rc)
{
    log_info("Init data conn...");
    if (client->data_conn->mode == PORT)
    {
        client->data_conn->clifd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (client->data_conn->clifd > server_rc->maxfd)
        {
            server_rc->maxfd = client->data_conn->clifd;
        }

        int flags = fcntl(client->data_conn->clifd, F_GETFL, 0);
        fcntl(client->data_conn->clifd, F_SETFL, flags | O_NONBLOCK);

        log_info("start connect %s:%d (%d)", inet_ntoa(client->data_conn->addr->sin_addr), ntohs(client->data_conn->addr->sin_port), client->data_conn->clifd);

        int res = connect(client->data_conn->clifd, (struct sockaddr *)client->data_conn->addr, sizeof(struct sockaddr_in));

        if (res == 0)
        {
            log_info("connect successfully");
        }
        else if (res < 0 && errno == EINPROGRESS)
        {
            log_info("connect in progress");
        }
        else
        {
            log_error("conect error: %s", strerror(errno));
        }

        if (client->command_status == STOR)
        {
            FD_SET(client->data_conn->clifd, &server_rc->all_rset);
        }
        else
        {
            FD_SET(client->data_conn->clifd, &server_rc->all_wset);
        }
        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(150, "Opening Binary mode data connection\r\n");
    }
    if (client->data_conn->mode == PASV)
    {
        FD_SET(client->data_conn->listenfd, &server_rc->all_rset);
        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(150, "Opening Binary mode data connection\r\n");
    }
    return make_response(500, "Error\r\n");
}

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

    log_info("parse ip: %s", ip);
    log_info("parse port: %d", *port);
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
        i++;
    }
    int port = ntohs(addr->sin_port);
    int port_1 = port / 256;
    int port_2 = port % 256;
    char *buf = (char *)malloc(MAXLINE);
    snprintf(buf, MAXLINE, "%s,%d,%d", ip, port_1, port_2);
    free(ip);
    return buf;
}

int handle_read(struct Client *client, struct Server_RC *server_rc)
{
    struct Data_Conn *data_conn = client->data_conn;

    if (client->command_status == LIST)
    {
        int nread = strlen(data_conn->buf);
        write(data_conn->clifd, data_conn->buf, nread);
        client->cmd_response = make_response(226, "Transfer complete\r\n");
        FD_SET(client->socket_fd, &server_rc->all_wset);
        clear_data_conn(client, server_rc);
        return nread;
    }

    if (client->command_status == RETR)
    {
        if (aio_error(data_conn->acb) != 0)
        {
            return 0;
        }

        int nread = aio_return(data_conn->acb);
        write(data_conn->clifd, (char *)data_conn->acb->aio_buf, nread);
        if (nread == data_conn->acb->aio_nbytes)
        {
            data_conn->acb->aio_offset += nread;
            aio_read(data_conn->acb);
        }
        else
        {
            client->cmd_response = make_response(226, "Transfer complete\r\n");
            FD_SET(client->socket_fd, &server_rc->all_wset);
            clear_data_conn(client, server_rc);
        }
        return nread;
    }
    return 0;
}

int handle_write(struct Client *client, struct Server_RC *server_rc)
{
    struct Data_Conn *data_conn = client->data_conn;

    if (client->command_status == STOR)
    {
        if (aio_error(data_conn->acb) != 0)
        {
            return 0;
        }

        aio_return(data_conn->acb);
        data_conn->acb->aio_nbytes = BUFSIZ;
        int nwrite = read(data_conn->clifd, (char *)data_conn->acb->aio_buf, data_conn->acb->aio_nbytes);
        if (nwrite == 0)
        {
            client->cmd_response = make_response(226, "Transfer complete\r\n");
            FD_SET(client->socket_fd, &server_rc->all_wset);
            clear_data_conn(client, server_rc);
        }
        else
        {
            data_conn->acb->aio_nbytes = nwrite;
            aio_write(data_conn->acb);
        }
        return nwrite;
    }
    return 0;
}

struct Command_Response *handle_command(struct Client *client, char *buf, struct Server_RC *server_rc)
{
    client->last_check_time = time(NULL);

    Command *cmd = parse_command(buf);

    log_info("command type: %s", cmd->type);
    log_info("command args: %s", cmd->args);

    // handle User command
    if (strcmp(cmd->type, "USER") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);

        // must be anonymous
        if (strcmp(cmd->args, "anonymous") == 0)
        {
            struct User *user = (struct User *)malloc(sizeof(struct User));
            *user = DEFAULT_USER;
            user->username = (char *)malloc(strlen(cmd->args) + 1);
            bzero(user->username, strlen(cmd->args) + 1);
            strncpy(user->username, cmd->args, strlen(cmd->args));
            client->user = user;

            return make_response(331, "Guest login ok,send your complete e-mail address as password\r\n");
        }
        // unacceptable username
        else
        {
            return make_response(530, "Unacceptable username\r\n");
        }
    }

    // handle Pass command
    if (strcmp(cmd->type, "PASS") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        struct User *user = client->user;
        if (user == NULL)
        {
            return make_response(503, "Must request USER first\r\n");
        }

        user->password = (char *)malloc(strlen(cmd->args) + 1);
        bzero(user->password, strlen(cmd->args) + 1);
        strncpy(user->password, cmd->args, strlen(cmd->args));
        char message[][MAXLINE] = {
            "\r\n",
            "Welcome to\r\n",
            "School of Software\r\n",
            "FTP Archives at ftp.ssast.org\r\n",
            "\r\n",
            "This site is provided as a public service by School of\r\n",
            "Software. Use in violation of any applicable laws is strictly\r\n",
            "prohibited. We make no guarantees, explicit or implicit, about the\r\n",
            "contents of this site. Use at your own risk.\r\n",
            "\r\n",
            "Guest login ok, access restrictions apply.\r\n",
            "\0"};
        return make_multiline_response(230, message);
    }

    struct User *user = client->user;
    if (user == NULL || user->password == NULL)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(530, "Not logged in\r\n");
    }

    if (strcmp(cmd->type, "SYST") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(215, "UNIX Type: L8\r\n");
    }

    if (strcmp(cmd->type, "TYPE") == 0)
    {
        if (strcmp(cmd->args, "I") == 0)
        {
            FD_SET(client->socket_fd, &server_rc->all_wset);
            return make_response(200, "Type set to I.\r\n");
        }
        else
        {
            FD_SET(client->socket_fd, &server_rc->all_wset);
            return make_response(502, "Not executed\r\n");
        }
    }

    if (strcmp(cmd->type, "PORT") == 0)
    {
        clear_data_conn(client, server_rc);

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

        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(200, "PORT command successful\r\n");
    }

    if (strcmp(cmd->type, "PASV") == 0)
    {
        clear_data_conn(client, server_rc);

        struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

        bzero(addr, sizeof(*addr));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(0);
        addr->sin_addr.s_addr = htonl(INADDR_ANY);

        struct Data_Conn *data_conn = (struct Data_Conn *)malloc(sizeof(struct Data_Conn));
        *data_conn = DEFAULT_DATA_CONN;
        data_conn->addr = addr;
        data_conn->mode = PASV;
        client->data_conn = data_conn;

        data_conn->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        bind(data_conn->listenfd, (struct sockaddr *)data_conn->addr, sizeof(struct sockaddr_in));
        log_info("bind (%d)", data_conn->listenfd);
        listen(data_conn->listenfd, 10);
        log_info("start listen (%d)", data_conn->listenfd);
        if (data_conn->listenfd > server_rc->maxfd)
        {
            server_rc->maxfd = data_conn->listenfd;
        }

        socklen_t sockaddr_in_length = sizeof(struct sockaddr_in);
        log_info("get socket name");
        getsockname(data_conn->listenfd, (struct sockaddr *)data_conn->addr, &sockaddr_in_length);

        log_info("stringify address");
        char *address = stringify_address(data_conn->addr);
        char *buf = (char *)malloc(MAXLINE);
        snprintf(buf, MAXLINE, "Entering Passive Mode (%s)\r\n", address);
        free(address);
        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(227, buf);
    }

    if (strcmp(cmd->type, "RETR") == 0 || strcmp(cmd->type, "STOR") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);

        bool is_retr = strcmp(cmd->type, "RETR") == 0 ? true : false;
        if (client->data_conn == NULL)
        {
            client->command_status = is_retr ? RETR : STOR;
            return make_response(150, "Opening Binary mode data connection\r\n");
        }

        int filename_length = strlen(client->current_dir) + strlen(cmd->args) + 2;
        char *filename = (char *)(malloc(filename_length));
        bzero(filename, filename_length);
        snprintf(filename, filename_length, "%s/%s", client->current_dir, cmd->args);

        int fd;
        if (is_retr)
        {
            fd = open(filename, O_RDONLY);
            if (fd == -1)
            {
                FD_SET(client->socket_fd, &server_rc->all_wset);
                clear_data_conn(client, server_rc);
                return make_response(550, "Can't open file\r\n");
            }
        }
        else
        {
            fd = open(filename, O_WRONLY | O_APPEND | O_CREAT);
            if (fd == -1)
            {
                FD_SET(client->socket_fd, &server_rc->all_wset);
                clear_data_conn(client, server_rc);
                return make_response(550, "Can't open file\r\n");
            }
        }

        struct aiocb *acb = (struct aiocb *)malloc(sizeof(struct aiocb));
        bzero(acb, sizeof(*acb));

        char *buf = (char *)malloc(BUFSIZ + 1);
        client->data_conn->buf = buf;
        acb->aio_buf = client->data_conn->buf;

        if (is_retr)
        {
            acb->aio_offset = 0;
            acb->aio_nbytes = BUFSIZ;
        }
        else
        {
            acb->aio_nbytes = 0;
        }
        acb->aio_fildes = fd;

        client->data_conn->acb = acb;

        if (is_retr)
        {
            log_info("start read %s", filename);
            aio_read(acb);
        }
        else
        {
            log_info("start write %s", filename);
            aio_write(acb);
        }

        client->command_status = is_retr ? RETR : STOR;
        log_info("set command status %d", client->command_status);
        return init_data_conn(client, server_rc);
    };

    if (strcmp(cmd->type, "QUIT") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        if (strlen(cmd->args) != 0)
        {
            return make_response(501, "Parameter error\r\n");
        }
        clear_data_conn(client, server_rc);
        // FD_SET(client->socket_fd, &server_rc->all_rset);
        return make_response(221, "Goodbye.\r\n");
    }

    if (strcmp(cmd->type, "MKD") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        char *path = normalize_path(client, cmd->args);
        int res = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
        if (res == 0)
        {
            char *buf = (char *)malloc(strlen(cmd->args) + 101);
            sprintf(buf, "created successfully!\r\n");

            return make_response(257, buf);
        }
        else
        {
            return make_response(550, "create failure\r\n");
        }
    }

    if (strcmp(cmd->type, "CWD") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);

        char *path = normalize_path(client, cmd->args);
        client->current_dir = path;

        int res = access(client->current_dir, F_OK | R_OK | X_OK);
        if (res == -1)
        {
            return make_response(550, "CWD failure\r\n");
        }
        return make_response(250, "Okay\r\n");
    }
    if (strcmp(cmd->type, "PWD") == 0)
    {
        char *buf = (char *)malloc(strlen(client->current_dir) + 3);
        bzero(buf, strlen(client->current_dir + 5));
        sprintf(buf, "\"%s\"\r\n", client->current_dir);

        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(257, buf);
    }
    if (strcmp(cmd->type, "LIST") == 0)
    {
        client->command_status = LIST;
        if (client->data_conn == NULL)
        {
            client->command_status = LIST;
            return make_response(150, "Opening Binary mode data connection\r\n");
        }

        char *buf = list_dir(client->current_dir);
        log_info("set command status %d", client->command_status);
        client->data_conn->buf = buf;
        return init_data_conn(client, server_rc);
    }
    if (strcmp(cmd->type, "RMD") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        char *path = normalize_path(client, cmd->args);
        int res = rmdir(path);
        if (res == 0)
        {
            return make_response(250, "Removed successfully\r\n");
        }
        return make_response(550, "Removed failure\r\n");
    }
    if (strcmp(cmd->type, "DELE") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        char *path = normalize_path(client, cmd->args);
        int res = remove(path);
        if (res == 0)
        {
            return make_response(250, "Removed successfully\r\n");
        }
        return make_response(550, "Removed failure\r\n");
    }
    if (strcmp(cmd->type, "RNFR") == 0)
    {
        client->current_filename = normalize_path(client, cmd->args);

        FD_SET(client->socket_fd, &server_rc->all_wset);
        return make_response(350, "Rename start successfully\r\n");
    }
    if (strcmp(cmd->type, "RNTO") == 0)
    {
        FD_SET(client->socket_fd, &server_rc->all_wset);
        char *path = normalize_path(client, cmd->args);

        if (client->current_filename == NULL)
        {
            return make_response(503, "Must request RNFR first\r\n");
        }
        int res = rename(client->current_filename, path);
        if (res == 0)
        {
            return make_response(250, "Rename successfully\r\n");
        }
        return make_response(550, "Rename failure\r\n");
    }

    free(cmd->args);
    free(cmd->type);
    free(cmd);
    return NULL;
}
