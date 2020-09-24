#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef DEBUG
#include <arpa/inet.h>
#endif

#include "config.h"
#include "request_parser.h"
#include "auth.h"

int server_socket;

FILE* logfile;

// Sends a response to the client
void send_response(int client_socket, char* msg, int response_size) {
    // Error handling
    if(strstr(msg, "Status: ")) {
        char* protocol = "HTTP/1.1 ";
        send(client_socket, protocol, strlen(protocol), 0);
        send(client_socket, msg+8, response_size-8, 0);
    } else if(strstr(msg, "Content-Type: ")) {
        char* status = "HTTP/1.1 200 OK\r\n";
        send(client_socket, status, strlen(status), 0);
        send(client_socket, msg, response_size, 0);
    }
}

// Execute git-http-backend
void execute_git(Request request, const char* repo_path, const char* git_path, char* response, int* response_size) {
    int ptc[2], ctp[2];
    assert(pipe(ptc) != -1);
    assert(pipe(ctp) != -1);
    pid_t pid = fork();
    assert(pid >= 0);
    // Child process
    if(pid == 0) {
        close(ctp[0]);
        close(ptc[1]);
        if(dup2(ptc[0], STDIN_FILENO) == -1) {
            perror("dup2[0]");
            exit(EXIT_FAILURE);
        }
        if(dup2(ctp[1], STDOUT_FILENO) == .1) {
            perror("dup2[1]");
            exit(EXIT_FAILURE);
        }
        char* args[] = {"git", "http-backend", NULL};
        char* env[8];

        // GIT_HTTP_EXPORT_ALL
        env[0] = "GIT_HTTP_EXPORT_ALL=1";
#ifdef DEBUG
        printf("%s\n", env[0]);
#endif

        // GIT_PROJECT_ROOT
        int project_root_len = strlen("GIT_PROJECT_ROOT=") + strlen(repo_path);
        char project_root[project_root_len];
        memcpy(project_root, "GIT_PROJECT_ROOT=", strlen("GIT_PROJECT_ROOT=")+1);
        strcat(project_root, repo_path);
        env[1] = project_root;
#ifdef DEBUG
        printf("%s\n", project_root);
#endif

        // REQUEST_METHOD
        int request_method_len = strlen("REQUEST_METHOD=") + strlen(request.method);
        char request_method[request_method_len];
        memcpy(request_method, "REQUEST_METHOD=", strlen("REQUEST_METHOD=")+1);
        strcat(request_method, request.method);
        env[2] = request_method;
#ifdef DEBUG
        printf("%s\n",request_method);
#endif

        // PATH_INFO
        int path_info_len = strlen("PATH_INFO=") + strlen(request.uri);
        char path_info[path_info_len];
        memcpy(path_info, "PATH_INFO=", strlen("PATH_INFO=")+1);
        strcat(path_info, request.uri);
        env[3] = path_info;
#ifdef DEBUG
        printf("%s\n", path_info);
#endif

        // QUERY_STRING
        int query_string_len = strlen("QUERY_STRING=") + request.query_string_len;
        char query_string[query_string_len];
        memcpy(query_string, "QUERY_STRING=", strlen("QUERY_STRING=")+1);
        strncat(query_string, request.query_string, request.query_string_len);
        env[4] = query_string;
#ifdef DEBUG
        printf("%s\n", query_string);
#endif

        // CONTENT_TYPE
        int content_type_len = strlen("CONTENT_TYPE=") + request.content_type_len;
        char content_type[content_type_len];
        memcpy(content_type, "CONTENT_TYPE=", strlen("CONTENT_TYPE=")+1);
        strncat(content_type, request.content_type, request.content_type_len);
        env[5] = content_type;
#ifdef DEBUG
        printf("%s\n", content_type);
#endif

        // HTTP_CONTENT_ENCODING
        // int encoding_len = strlen("HTTP_CONTENT_ENCODING=") + request.encoding_len;
        // char encoding[encoding_len];
        // memcpy(encoding, "HTTP_CONTENT_ENCODING=", strlen("HTTP_CONTENT_ENCODING=")+1);
        // strncat(encoding, request.encoding, request.encoding_len);
        // env[6] = encoding;
        // printf("%s\n", encoding);

        // REMOTE_USER
        int remote_user_len = strlen("REMOTE_USER=") + request.authorization_len;
        char remote_user[remote_user_len];
        memcpy(remote_user, "REMOTE_USER=", strlen("REMOTE_USER=")+1);
        strncat(remote_user, request.authorization, request.authorization_len);
        env[6] = remote_user;
#ifdef DEBUG
        printf("%s\n", remote_user);
#endif

        env[7] = NULL;
        execve(git_path, args, env);
        perror("execve");
    // Parent process
    } else {
        close(ptc[0]);
        close(ctp[1]);
        if(strcmp(request.method, "POST") == 0) {
            write(ptc[1], request.body, request.bodySize);
        }
        close(ptc[1]);
        wait(NULL);

        char buf;
        *response_size = 0;
        while(read(ctp[0], &buf, 1) > 0) {
#ifdef DEBUG
            fputc(buf, logfile);
#endif
            response[*response_size] = buf;
            *response_size += 1;
        }
        close(ctp[0]);
    }
}

// Handler for Ctrl+C
// This is because of the infinite loop, so that no errors occur
void sigintHandler(int signal) {
    close(server_socket);
#ifdef DEBUG
    fclose(logfile);
#endif
    exit(EXIT_FAILURE);
}

void daemonize() {
    pid_t pid;

    pid = fork();

    if(pid < 0)
        exit(EXIT_FAILURE);
    if(pid > 0)
        exit(EXIT_SUCCESS);

    if(setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    umask(0);
}

int main(int argc, char *argv[]) {
    
    char* configPath = "sgs.conf";
    int daemon = 0;

    int opt;
    while((opt = getopt(argc, argv, "dc:")) != -1) {
        switch (opt) {
        case 'd':
            daemon = 1;
            break;
        case 'c':
            configPath = optarg;
            break;
        }
    }

    if(daemon) {
        daemonize();
    }

    Config config = loadConfig(configPath);

#ifdef DEBUG
    logfile = fopen("sgs.log", "w");
#else
    logfile = NULL;
#endif
    
    // Set signal handler for Ctrl+C
    signal(SIGINT, sigintHandler);
    signal(SIGSEGV, sigintHandler);

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

#ifdef DEBUG
        char address[100];
        inet_ntop(AF_INET6, &client_addr.sin6_addr, address, 100);
        fprintf(logfile, "New connection from %s\n", address);
#endif

        // Recieve the request
        char in_msg[1024];
        size_t length = recv(client_socket, in_msg, sizeof(in_msg), 0);

#ifdef DEBUG
        fprintf(logfile, "Request:\n%s\n", in_msg);
#endif
        // Parse the request
        Request request = parseRequest(in_msg);

        if(check_auth(&request, client_socket)) {
            // Get the response
            char response[2048];
            int response_size;
            execute_git(request, config.repo_path, config.git_path, response, &response_size);

            send_response(client_socket, response, response_size);
        }

        // Free allocated memory and close the connection to the client
        free_request(request);
        close(client_socket);
    }

    destroyConfig(config);
#ifdef DEBUG
    fclose(logfile);
#endif
    return 0;
}