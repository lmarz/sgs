#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "config.h"

extern SSL_CTX* ctx;

int init_server(Config* config);
SSL* get_request(int server_socket, char* msg, int size);
void begin_response_success(SSL* ssl);
void begin_response_fail(SSL* ssl);
void send_401(SSL* ssl);
void send_403(SSL* ssl);

#endif /* SERVER_H */