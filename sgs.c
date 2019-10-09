#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// The parameters behind the ? of the uri (?name=value)
typedef struct QueryString {
    char* name;
    char* value;
} QueryString;

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
    QueryString* queryStrings; // The query strings
    size_t queryStringCount;   // The amount of query strings
    Header* headers;           // The HTTP headers
    size_t headerCount;        // The amount of headers
    char* body;                // The body, only used on POST requsts
    size_t bodySize;           // The size of the body
} Request;

int server_socket;

// A function, that counts the amount of char c in char* s
int countChars(char* s, char c) {
    return *s == '\0' ? 0 : countChars( s + 1, c ) + (*s == c);
}

// Extracts the query strings and puts them in the request struct
void getQueryStrings(Request* request) {
    // Check if there are query strings
    if(strstr(request->uri, "?") != NULL) {
        // Copy the uri and split it
        char uri[1024];
        strcpy(uri, request->uri);
        strtok(uri, "?");
        char* query_strings = strtok(NULL, "?");

        // Get the amount of query strings and allocate memory for them
        request->queryStringCount = countChars(query_strings, '=');
        QueryString* queryStrings = malloc(request->queryStringCount * sizeof(QueryString));
        request->queryStrings = queryStrings;

        char* query_string = strtok(query_strings, "&");

        for(int i = 0; i < request->queryStringCount; i++) {
            // Get the name of the query string
            size_t nameLength = strcspn(query_string, "=");
            request->queryStrings[i].name = malloc(nameLength+1);
            memcpy(request->queryStrings[i].name, query_string, nameLength);
            request->queryStrings[i].name[nameLength] = '\0';

            // Get the value of the query string
            size_t valueLength = strlen(query_string) - nameLength - 1;
            request->queryStrings[i].value = malloc(valueLength+1);
            memcpy(request->queryStrings[i].value, query_string+nameLength+1, valueLength);
            request->queryStrings[i].value[valueLength] = '\0';

            query_string = strtok(NULL, "&");
        }

    } else {
        // If there are no query strings, just fill in nothing
        request->queryStringCount = 0;
        request->queryStrings = NULL;
    }
}

// Extracts the headers and puts them in the request struct
void getHeaders(Request* request, char* msg) {

    // Get the length of the header
    char* header_end = strstr(msg, "\r\n\r\n");
    int header_length = header_end - msg;

    // Copy only the header from msg
    char* header_msg = malloc(header_length);
    strncpy(header_msg, msg, header_length);

    // Get the amount of the headers and allocate memory for them
    request->headerCount = countChars(header_msg, '\n');
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
void getBody(Request* request, char* msg) {
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

        printf("%s\n", request->body);
    } else {
        request->body = NULL;
        request->bodySize = 0;
    }
}

// Free all the values, that where allocated using malloc()
void free_request(Request request) {
    if(request.queryStringCount > 0) {
        for(int i = 0; i < request.queryStringCount; i++) {
            free(request.queryStrings[i].name);
            free(request.queryStrings[i].value);
        }
        free(request.queryStrings);
    }
    for(int i = 0; i < request.headerCount; i++) {
        free(request.headers[i].name);
        free(request.headers[i].value);
    }
    free(request.headers);
    if(request.bodySize > 0) {
        free(request.body);
    }
}

// Handler for Ctrl+C
// This is because of the infinite loop, so that no errors occur
void sigintHandler(int signal) {
    close(server_socket);
    exit(0);
}

int main(int argc, char const *argv[]) {
    
    // Set signal handler for Ctrl+C
    signal(SIGINT, sigintHandler);

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_socket >= 0);

    // Bind the socket to port 5000
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(5000);

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
        getQueryStrings(&request);

        // Extract the headers
        getHeaders(&request, in_msg);
        
        // Extract the body
        getBody(&request, in_msg);

        // TODO: actually implement git functionality

        // Free allocated memory and close the connection to the client
        free_request(request);
        close(client_socket);
    }

    return 0;
}