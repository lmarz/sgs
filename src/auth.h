#ifndef AUTH_H
#define AUTH_H

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "request_parser.h"
#include "server.h"

int check_auth(Request* request, int client_socket);

#endif /* AUTH_H */