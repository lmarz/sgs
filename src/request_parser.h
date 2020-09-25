#ifndef REQUEST_PARSER_CONFIG
#define REQUEST_PARSER_CONFIG

#include <sys/types.h>

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
    int query_string_len;      // The length of the query string
    Header* headers;           // The HTTP headers
    size_t headerCount;        // The amount of headers
    // char* encoding;            // The encoding of the response
    // int encoding_len;          // The length of the encoding
    char* body;                // The body, only used on POST requsts
    int bodySize;              // The size of the body
    char* content_type;        // The content type of the body
    int content_type_len;      // The length of the content type
    char* authorization;       // The authorization
    size_t authorization_len;  // The length of the authorization
} Request;

/**
 * Parses the incoming http request with the help of the other functions in this file
 */
Request parseRequest(char* msg);

/**
 * Extracts the information of the first line
 */
void get_first_line(Request* request, char* msg);

/**
 * Separate the query string from the uri
 */
void get_query_string(Request* request);

/**
 * Extracts the headers and puts them in the request struct
 */
void get_headers(Request* request, char* msg);

/**
 * Extracs the body and puts it in the request struct
 */
void get_body(Request* request, char* msg);

/**
 * Free all the values, that where allocated using malloc()
 */
void free_request(Request request);

#endif