#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"

int init_server(Config config);
int get_request(int server_socket, char* msg, int size);
void begin_response_success(int client_socket);
void begin_response_fail(int client_socket);
void send_401(int client_socket);
void send_403(int client_socket);

#endif /* SERVER_H */