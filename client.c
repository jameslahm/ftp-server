#include "client.h"

struct Data_Conn DEFAULT_DATA_CONN = {NULL, -1, -1, PORT, NULL};
struct Client DEFAULT_CLIENT = {NULL, NULL, NULL, -1, NULL, NULL, IDLE};
// default alloc client nums
int DEFAULT_CLIENT_ALLOC_SIZE = 10;
// clients size
int clients_size = 0;

struct Client *client_alloc(struct Client *clients)
{
    if (clients == NULL)
    {
        clients = (struct Client *)malloc(DEFAULT_CLIENT_ALLOC_SIZE * sizeof(struct Client));
    }
    else
    {
        clients = (struct Client *)realloc(clients, (clients_size + DEFAULT_CLIENT_ALLOC_SIZE) * sizeof(struct Client));
    }
    if (clients == NULL)
    {
        printf("Error client_alloc(): can't alloc for clients array");
    }
    for (int i = clients_size; i < clients_size + DEFAULT_CLIENT_ALLOC_SIZE; i++)
    {
        clients[i] = DEFAULT_CLIENT;
        struct Data_Conn *data_conn = (struct Data_Conn *)malloc(sizeof(struct Data_Conn));
        *data_conn = DEFAULT_DATA_CONN;
        clients[i].data_conn=data_conn;
    }
    clients_size += DEFAULT_CLIENT_ALLOC_SIZE;
    return clients;
}

int client_add(struct Server_RC *server_rc, int socket_fd)
{
    if (server_rc->clients == NULL)
    {
        server_rc->clients = client_alloc(server_rc->clients);
    }
    int i;
    for (i = 0; i < clients_size; i++)
    {
        if (server_rc->clients[i].socket_fd == -1)
        {
            server_rc->clients[i].socket_fd = socket_fd;
            return i;
        }
    }
    server_rc->clients = client_alloc(server_rc->clients);
    server_rc->clients[i].socket_fd = socket_fd;
    return i;
}

void client_del(struct Client *clients, ClientId client_id)
{
    clients[client_id].socket_fd = -1;
    free(clients[client_id].user);
    clients[client_id].user = NULL;
    return;
}