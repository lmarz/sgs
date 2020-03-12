#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "request_parser.h"

int server_socket;

// Sends a response to the client
void send_response(int client_socket, char* msg) {
    // Error handling
    if(strstr(msg, "Status: ")) {
        char* protocol = "HTTP/1.1 ";
        send(client_socket, protocol, strlen(protocol), 0);
        send(client_socket, msg+8, strlen(msg+8), 0);
    } else if(strstr(msg, "Content-Type: ")) {
        char* status = "HTTP/1.1 200 OK\r\n";
        send(client_socket, status, strlen(status), 0);
        send(client_socket, msg, strlen(msg), 0);
    }
}

// Execute git-http-backend
void execute_git(Request request, const char* repo_path, char* response) {
    int fd[2];
    assert(pipe(fd) != -1);
    pid_t pid = fork();
    assert(pid >= 0);
    // Child process
    if(pid == 0) {
        dup2(fd[0], STDIN_FILENO);
        dup2(fd[1], STDOUT_FILENO);
        char* args[] = {"git", "http-backend", NULL};
        char* env[6];

        // REQUEST_METHOD
        int request_method_len = strlen("REQUEST_METHOD=") + strlen(request.method);
        char request_method[request_method_len];
        memcpy(request_method, "REQUEST_METHOD=", strlen("REQUEST_METHOD=")+1);
        strcat(request_method, request.method);
        env[0] = request_method;
        
        // GIT_PROJECT_ROOT
        int project_root_len = strlen("GIT_PROJECT_ROOT=") + strlen(repo_path);
        char project_root[project_root_len];
        memcpy(project_root, "GIT_PROJECT_ROOT=", strlen("GIT_PROJECT_ROOT=")+1);
        strcat(project_root, repo_path);
        env[1] = project_root;

        // PATH_INFO
        int path_info_len = strlen("PATH_INFO=") + strlen(request.uri);
        char path_info[path_info_len];
        memcpy(path_info, "PATH_INFO=", strlen("PATH_INFO=")+1);
        strcat(path_info, request.uri);
        env[2] = path_info;

        // QUERY_STRING
        int query_string_len = strlen("QUERY_STRING=") + strlen(request.query_string);
        char query_string[query_string_len];
        memcpy(query_string, "QUERY_STRING=", strlen("QUERY_STRING=")+1);
        strcat(query_string, request.query_string);
        env[3] = query_string;

        // GIT_HTTP_EXPORT_ALL
        env[4] = "GIT_HTTP_EXPORT_ALL=1";

        env[5] = NULL;
        execve("/usr/bin/git", args, env);
        perror("execve");
    // Parent process
    } else {

        if(strcmp(request.method, "POST") == 0) {
            write(fd[1], request.body, request.bodySize);
        }
        close(fd[1]);
        wait(NULL);

        char buf;
        size_t response_size = 0;
        while(read(fd[0], &buf, 1) > 0) {
            response[response_size] = buf;
            response_size++;
        }
        close(fd[0]);
    }
}

// Handler for Ctrl+C
// This is because of the infinite loop, so that no errors occur
void sigintHandler(int signal) {
    close(server_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    
    char* configPath = "sgs.conf";

    int opt;
    while((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
        case 'c':
            configPath = optarg;
            break;
        }
    }

    Config config = loadConfig(configPath);
    
    // Set signal handler for Ctrl+C
    signal(SIGINT, sigintHandler);

    // Create the server socket
    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    assert(server_socket >= 0);

    // Bind the socket to port 5000
    struct sockaddr_in6 server_addr;
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(config.port);

    int ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    assert(ret >= 0);

    // Listen on incoming connections
    ret = listen(server_socket, 1);
    assert(ret >= 0);

    struct sockaddr_in6 client_addr;
    socklen_t len = sizeof(client_addr);

    int client_socket;

    while(1) {
        // Connnect with the client
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len);
        assert(client_socket >= 0);

        // Recieve the request
        char in_msg[1024];
        size_t length = recv(client_socket, in_msg, sizeof(in_msg), 0);

        // Parse the request
        Request request = parseRequest(in_msg);

        // Get the response
        char response[1024];
        execute_git(request, config.repo_path, response);
        printf("%s\n", response);
        send_response(client_socket, response);

        // Free allocated memory and close the connection to the client
        free_request(request);
        close(client_socket);
    }

    destroyConfig(config);
    return 0;
}