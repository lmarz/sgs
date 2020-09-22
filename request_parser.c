#include "request_parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// A function, that counts the amount of char c in char* s
int count_chars(char* s, char c) {
    int i = 0;
    for (i=0; s[i]; s[i]==c ? i++ : *s++);
    return i;
}

// Parses the incoming http request with the help of the other functions in this file
Request parseRequest(char* msg) {
    Request request;

    // Get the method, uri and protocol
    get_first_line(&request, msg);

    // Extract the query strings
    get_query_string(&request);

    // Extract the headers
    get_headers(&request, msg);

    // Extract the body
    get_body(&request, msg);

    return request;
}

// Extract the information of the first line
void get_first_line(Request* request, char* msg) {
    char first_line[1024];
    strcpy(first_line, msg);

    // Method
    size_t method_len = strcspn(first_line, " ");
    request->method = malloc(method_len+1);
    memcpy(request->method, first_line, method_len);
    request->method[method_len] = '\0';

    // URI
    size_t uri_len = strcspn(first_line+method_len+1, " ");
    request->uri = malloc(uri_len+1);
    memcpy(request->uri, first_line+method_len+1, uri_len);
    request->uri[uri_len] = '\0';

    // HTTP version
    size_t proto_len = strcspn(first_line+method_len+uri_len+2, "\r\n");
    request->protocol = malloc(proto_len+1);
    memcpy(request->protocol, first_line+method_len+uri_len+2, proto_len);
    request->protocol[proto_len] = '\0';
}

// Separate the query string from the uri
void get_query_string(Request* request) {
    // Check if there are query strings
    if(strstr(request->uri, "?") != NULL) {
        // Copy the uri and split it
        char uri[1024];
        strcpy(uri, request->uri);
        free(request->uri);

        size_t uri_len = strcspn(uri, "?");
        request->uri = malloc(uri_len+1);
        memcpy(request->uri, uri, uri_len);
        request->uri[uri_len] = '\0';

        request->query_string_len = strlen(uri) - uri_len;
        request->query_string = malloc(request->query_string_len+1);
        memcpy(request->query_string, uri+uri_len+1, request->query_string_len);
        request->query_string[request->query_string_len] = '\0';
    } else {
        request->query_string_len = 0;
        request->query_string = NULL;
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
    request->headerCount = count_chars(header_msg, '\n');
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
    // request->encoding = NULL;
    // request->encoding_len = 0;
    // for(int i = 0; i < request->headerCount; i++) {
    //     if(strcmp(request->headers[i].name, "Accept-Encoding") == 0) {
    //         if(strstr(request->headers[i].value, "gzip") != NULL) {
    //             request->encoding = malloc(5);
    //             memcpy(request->encoding, "gzip", 5);
    //             request->encoding_len = 4;
    //         }
    //     }
    // }
    request->authorization = NULL;
    request->authorization_len = 0;
    for(int i = 0; i < request->headerCount; i++) {
        if(strcmp(request->headers[i].name, "Authorization") == 0) {
            request->authorization = request->headers[i].value;
            request->authorization_len = strlen(request->authorization);
            break;
        }
    }
}

// Extracs the body and puts it in the request struct
void get_body(Request* request, char* msg) {
    request->content_type = NULL;
    request->content_type_len = 0;

    if(strcmp(request->method, "POST") == 0) {
        // Get the length of the header
        char* header_end = strstr(msg, "\r\n\r\n");
        int header_length = header_end - msg + 4; // + \r\n\r\n

        // Get the length of the body
        for(int i = 0; i < request->headerCount; i++) {
            if(strcmp(request->headers[i].name, "Content-Length") == 0) {
                request->bodySize = atoi(request->headers[i].value);
            } else if(strcmp(request->headers[i].name, "Content-Type") == 0) {
                request->content_type = request->headers[i].value;
                request->content_type_len = strlen(request->content_type);
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
    // Free the first line
    free(request.method);
    free(request.uri);
    free(request.query_string);
    free(request.protocol);

    // Free the headers
    for(int i = 0; i < request.headerCount; i++) {
        free(request.headers[i].name);
        free(request.headers[i].value);
    }
    free(request.headers);

    // Free the body, if present
    if(request.bodySize > 0) {
        free(request.body);
    }
}