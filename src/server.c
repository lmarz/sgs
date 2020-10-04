#include "server.h"

SSL_CTX* ctx;

static void init_ctx() {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD* method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if(ctx == NULL) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

static void load_certs(const char* certfile, const char* keyfile) {
    if(SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if(SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if(!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match with certificate\n");
        exit(EXIT_FAILURE);
    }
}

int init_server(Config* config) {
    // Create the server socket
    int server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to port 5000
    struct sockaddr_in6 server_addr;
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(config->port);

    int ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen on incoming connections
    ret = listen(server_socket, 1);
    if(ret < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    init_ctx();
    load_certs(config->tls_certfile, config->tls_keyfile);

    return server_socket;
}

SSL* get_request(int server_socket, char* msg, int size) {
    struct sockaddr_in6 address;
    int len = sizeof(address);

    int client_socket = accept(server_socket, (struct sockaddr*)&address, &len);
    if(client_socket < 0) {
        perror("accept");
        return NULL;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);

    if(SSL_accept(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_socket);
    }

    size_t length = SSL_read(ssl, msg, size);
    if(length <= 0) {
        perror("recv");
        SSL_free(ssl);
        close(client_socket);
        return NULL;
    }

    return ssl;
}

void begin_response_success(SSL* ssl) {
    char* status = "HTTP/1.1 200 OK\r\n";
    SSL_write(ssl, status, strlen(status));
}

void begin_response_fail(SSL* ssl) {
    char* protocol = "HTTP/1.1 ";
    SSL_write(ssl, protocol, strlen(protocol));
}

void send_401(SSL* ssl) {
    char* msg = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"sgs\"\r\n\r\n";
    SSL_write(ssl, msg, strlen(msg));
}

void send_403(SSL* ssl) {
    char* msg = "HTTP/1.1 403 Forbidden\r\n\r\n";
    SSL_write(ssl, msg, strlen(msg));
}

void send_500(SSL* ssl) {
    char* msg = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    SSL_write(ssl, msg, strlen(msg));
}