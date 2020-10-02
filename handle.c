#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>

#include "client.h"
#include "handle.h"
#include "response.h"

CommandResponse *handle_command(Client *client, char *buf)
{
    Command *cmd = parse_command(buf);

    // handle User command
    if (strcmp(cmd->type, "USER") == 0)
    {
        if (strcmp(cmd->args, "anonymous") == 0)
        {
            User *user = (char *)malloc(sizeof(User));
            user->username = (char *)malloc(strlen(cmd->args) + 1);
            bzero(user->username, strlen(cmd->args) + 1);
            strncpy(user->username, cmd->args, strlen(cmd->args));
            client->user = user;
            return make_response(client->client_id, 331, "Guest login ok,send your complete e-mail address as password\r\n");
        }
    }

    // handle Pass command
    if (strcmp(cmd->type, "PASS") == 0)
    {
        User *user = client->user;
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
        return make_multiline_response(client->client_id, 230, message);
    }

    if (strcmp(cmd->type, "SYST") == 0)
    {
        return make_response(client->client_id, 215, "UNIX Type: I\r\n");
    }

    if (strcmp(cmd->type, "TYPE") == 0)
    {
        if (strcmp(cmd->args, "I") == 0)
        {
            return make_response(client->client_id, 200, "Type set to I\r\n");
        }
    }

    if (strcmp(cmd->type, "PORT") == 0)
    {
        char *ip = (char *)malloc(sizeof(char) * IP_LENGTH);
        bzero(ip, sizeof(char) * IP_LENGTH);
        int *port = (int *)malloc(sizeof(int));
        parse_address(cmd->args, ip, port);

        struct sockaddr_in addr;

        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(*port);
        inet_aton(ip, &addr.sin_addr);
        return make_response(client->client_id, 200, "PORT command successful\r\n");
    }

    if (strcmp(cmd->type,"RETR")==0){
        
    }

    free(cmd->args);
    free(cmd->type);
    free(cmd);
};

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