#include "auth.h"
/*
 * Base64 decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 */

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len) {
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

static void send_401(int client_socket) {
    char* msg = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"sgs\"\r\n\r\n";
    send(client_socket, msg, strlen(msg), 0);
}

static void send_403(int client_socket) {
    char* msg = "HTTP/1.1 403 Forbidden\r\n\r\n";
    send(client_socket, msg, strlen(msg), 0);
}

int check_auth(Request* request, int client_socket) {
    char* service = request->query_string;
    if(request->query_string == NULL) {
        service = "";
    }
    printf("%s\n", request->uri);
    if(strcmp(service, "service=git-receive-pack") == 0 || 
       strstr(request->uri, "git-receive-pack") != NULL) {
        if(request->authorization == NULL) {
            send_401(client_socket);
            return 0;
        } else {
            printf("Test\n");
            size_t scheme_len = strcspn(request->authorization, " ");
            request->authorization += scheme_len+1;
            request->authorization = base64_decode(request->authorization, strlen(request->authorization), &request->authorization_len);
            if(strcmp(request->authorization, "admin:admin") == 0) {
                size_t user = strcspn(request->authorization, ":");
                request->authorization[user] = '\0';
                return 1;
            } else {
                send_403(client_socket);
                return 0;
            }
        }
    }
    return 1;
}