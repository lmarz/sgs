#ifndef AUTH_H
#define AUTH_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <sqlite3.h>

#include "request_parser.h"
#include "server.h"

void auth_init(const char* filename);
void auth_destroy();
int check_auth(Request* request, SSL* ssl);
void add_user(const char* user, const char* password);

#endif /* AUTH_H */