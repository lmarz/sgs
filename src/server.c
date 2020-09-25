#include "server.h"

int init_server(Config config) {
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
    server_addr.sin6_port = htons(config.port);

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
    return server_socket;
}

int get_request(int server_socket, char* msg, int size) {
    struct sockaddr_in6 address;
    int len = sizeof(address);

    int client_socket = accept(server_socket, (struct sockaddr*)&address, &len);
    if(client_socket < 0) {
        perror("accept");
        return -1;
    }

    size_t length = recv(client_socket, msg, size, 0);
    if(length <= 0) {
        perror("recv");
        close(client_socket);
        return -1;
    }

    return client_socket;
}

void begin_response_success(int client_socket) {
    char* status = "HTTP/1.1 200 OK\r\n";
    send(client_socket, status, strlen(status), 0);
}

void begin_response_fail(int client_socket) {
    char* protocol = "HTTP/1.1 ";
    send(client_socket, protocol, strlen(protocol), 0);
}

void send_401(int client_socket) {
    char* msg = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"sgs\"\r\n\r\n";
    send(client_socket, msg, strlen(msg), 0);
}

void send_403(int client_socket) {
    char* msg = "HTTP/1.1 403 Forbidden\r\n\r\n";
    send(client_socket, msg, strlen(msg), 0);
}