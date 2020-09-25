#ifndef AUTH_H
#define AUTH_H

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "request_parser.h"
#include "server.h"

int check_auth(Request* request, SSL* ssl);

#endif /* AUTH_H */