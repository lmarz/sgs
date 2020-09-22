#ifndef AUTH_H
#define AUTH_H

#include "request_parser.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int check_auth(Request* request, int client_socket);

#endif /* AUTH_H */