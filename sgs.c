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

// The Headers of the request
typedef struct Header {
    char* name;
    char* value;
} Header;

// The request
typedef struct Request {
    char* method;              // The HTTP method (GET, POST, HEAD, etc.)
    char* uri;                 // The requested URI (domain.com/file)
    char* protocol;            // The used HTTP protocol (HTTP/1.0, HTTP/1.1)
    char* query_string;        // The full query string
    Header* headers;           // The HTTP headers
    size_t headerCount;        // The amount of headers
    char* body;                // The body, only used on POST requsts
    size_t bodySize;           // The size of the body
} Request;

int server_socket;

// A function, that counts the amount of char c in char* s
int count_chars(char* s, char c) {
    int i = 0;
    for (i=0; s[i]; s[i]==c ? i++ : *s++);
    return i;
}

// Separate the query string from the uri
void get_query_string(Request* request) {
    // Check if there are query strings
    if(strstr(request->uri, "?") != NULL) {
        // Copy the uri and split it
        char uri[1024];
        strcpy(uri, request->uri);
        request->uri = strtok(uri, "?");
        request->query_string = strtok(NULL, "?");
        // DO NOT REMOVE! SOMEHOW IT ONLY WORKS WITH THIS LINE
        printf("%s\n", request->uri);
    } else {
        request->query_string = "";
    }
}

// Extracts the headers and puts them in the request struct
void get_headers(Request* request, char* msg) {

    // Get the length of the header
    char* header_end = strstr(msg, "\r\n\r\n");
    int header_length = header_end - msg;

    // Copy only the header from msg
    char header_msg[header_length+1];
    strncpy(header_msg, msg, header_length);

    // Get the amount of the headers and allocate memory for them
    request->headerCount = count_chars(header_msg, '\n') - 1;
    Header* headers = malloc(request->headerCount * sizeof(Header));
    request->headers = headers;

    // Ignore the first line
    strtok(header_msg, "\r\n");

    char* line = strtok(NULL, "\r\n");
    for(int i = 0; i < request->headerCount; i++) {
        // Get the name of the header
        size_t nameLength = strcspn(line, ": ");
        request->headers[i].name = malloc(nameLength+1);
        memcpy(request->headers[i].name, line, nameLength);
        request->headers[i].name[nameLength] = '\0';

        // Get the value of the header
        size_t valueLength = strlen(line) - nameLength - 2;
        request->headers[i].value = malloc(valueLength+1);
        memcpy(request->headers[i].value, line+nameLength+2, valueLength);
        request->headers[i].value[valueLength] = '\0';

        line = strtok(NULL, "\r\n");
    }
}

// Extracs the body and puts it in the request struct
void get_body(Request* request, char* msg) {
    if(strcmp(request->method, "POST") == 0) {
        // Get the length of the header
        char* header_end = strstr(msg, "\r\n\r\n");
        int header_length = header_end - msg + 4; // + \r\n\r\n

        // Get the length of the body
        for(int i = 0; i < request->headerCount; i++) {
            if(strcmp(request->headers[i].name, "Content-Length") == 0) {
                request->bodySize = *request->headers[i].value;
                break;
            }
        }

        // Allocate memory and copy body
        request->body = malloc(request->bodySize);
        memcpy(request->body, msg+header_length, request->bodySize);
    } else {
        request->body = NULL;
        request->bodySize = 0;
    }
}

// Free all the values, that where allocated using malloc()
void free_request(Request request) {
    for(int i = 0; i < request.headerCount; i++) {
        free(request.headers[i].name);
        free(request.headers[i].value);
    }
    free(request.headers);
    if(request.bodySize > 0) {
        free(request.body);
    }
}

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
        char buf;
        size_t response_size = 0;
        while(read(fd[0], &buf, 1) > 0) {
            response[response_size] = buf;
            response_size++;
        }
        response[response_size] = '\0';
        close(fd[0]);
        wait(NULL);
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
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_socket >= 0);

    // Bind the socket to port 5000
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(config.port);

    int ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    assert(ret >= 0);

    // Listen on incoming connections
    ret = listen(server_socket, 1);
    assert(ret >= 0);

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int client_socket;

    while(1) {
        // Connnect with the client
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len);
        assert(client_socket >= 0);

        // Recieve the request
        char in_msg[1024];
        size_t length = recv(client_socket, in_msg, sizeof(in_msg), 0);

        // Get the method, uri and protocol
        char first_line[1024];
        strcpy(first_line, in_msg);

        Request request;

        request.method = strtok(first_line, " ");
        request.uri = strtok(NULL, " ");
        request.protocol = strtok(NULL, " \r\n");

        // Extract the query strings
        get_query_string(&request);

        // Extract the headers
        get_headers(&request, in_msg);

        // Extract the body
        get_body(&request, in_msg);

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