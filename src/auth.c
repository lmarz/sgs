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

char* users;
int user_amount;

void auth_init(const char* filename) {
	int fd = open(filename, O_RDONLY);
	struct stat filesize;
	fstat(fd, &filesize);
	users = mmap(NULL, filesize.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	user_amount = filesize.st_size / SHA512_DIGEST_LENGTH;
}

void auth_destroy() {
	munmap(users, user_amount * SHA512_DIGEST_LENGTH);
}

static int compare_shas(char* sha1, char* sha2) {
	for(int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
		if(sha1[i] != sha2[i]) {
			return 0;
		}
	}
	return 1;
}
/* admin:admin */
int check_auth(Request* request, SSL* ssl) {
    char* service = request->query_string;
    if(request->query_string == NULL) {
        service = "";
    }
    if(strcmp(service, "service=git-receive-pack") == 0 || 
       strstr(request->uri, "git-receive-pack") != NULL) {
        if(request->authorization == NULL) {
            send_401(ssl);
            return 0;
        } else {
            size_t scheme_len = strcspn(request->authorization, " ");
            request->authorization += scheme_len+1;
            request->authorization = base64_decode(request->authorization, strlen(request->authorization), &request->authorization_len);

			char hash[SHA512_DIGEST_LENGTH];
			SHA512(request->authorization, strlen(request->authorization), hash);
			for(int i = 0; i < user_amount; i++) {
				if(compare_shas(hash, users + i * SHA512_DIGEST_LENGTH)) {
					size_t user = strcspn(request->authorization, ":");
                	request->authorization[user] = '\0';
                	return 1;
				}
			}
            send_403(ssl);
            return 0;
        }
    }
    return 1;
}