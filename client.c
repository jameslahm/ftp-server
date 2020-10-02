#include <stdlib.h>
#include "client.h"
#include <stdio.h>

// default alloc client nums
static int DEFAULT_CLIENT_ALLOC_SIZE = 10;
// clients size
static int clients_size = 0;

void client_alloc(Client *clients)
{
    if (clients == NULL)
    {
        clients = malloc(DEFAULT_CLIENT_ALLOC_SIZE * sizeof(Client));
    }
    else
    {
        clients = realloc(clients, (clients_size + DEFAULT_CLIENT_ALLOC_SIZE) * sizeof(Client));
    }
    if (clients == NULL)
    {
        printf("Error client_alloc(): can't alloc for clients array");
    }
    for (int i = clients_size; i < clients_size + DEFAULT_CLIENT_ALLOC_SIZE; i++)
    {
        clients[i] = DEFAULT_CLIENT;
    }
    clients_size += DEFAULT_CLIENT_ALLOC_SIZE;
}

int client_add(Client *clients, int socket_fd)
{
    if (clients == NULL)
    {
        client_alloc(clients);
    }
    int i;
    for (i = 0; i < clients_size; i++)
    {
        if (clients[i].socket_fd == -1)
        {
            clients[i].socket_fd = socket_fd;
            return i;
        }
    }
    client_alloc(clients);
    clients[i].socket_fd = socket_fd;
    return i;
}

void client_del(Client *clients, ClientId client_id)
{
    clients[client_id].socket_fd = -1;
    free(clients[client_id].user);
    clients[client_id].user = NULL;
    return;
}