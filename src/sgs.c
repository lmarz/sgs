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

#include "config.h"
#include "request_parser.h"
#include "auth.h"
#include "server.h"

int server_socket;

FILE* logfile;

// Execute git-http-backend
void execute_git(Request request, const char* repo_path, const char* git_path, SSL* ssl) {
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
        char response[1024];
        int response_size = 0;
        int started = 0;
        while(read(ctp[0], &buf, 1) > 0) {
#ifdef DEBUG
            fputc(buf, logfile);
#endif
            response[response_size] = buf;
            response_size += 1;

            // Check if it has failed
            if(response_size == 8 && !started) {
                response[response_size] = '\0';
                if(strcmp(response, "Status: ") == 0) {
                    begin_response_fail(ssl);
                    started = 1;
                    response_size = 0;
                } else {
                    begin_response_success(ssl);
                    started = 1;
                }
            }

            // Send larger repositories
            if(response_size == 1024) {
                SSL_write(ssl, response, response_size);
                response_size = 0;
            }
        }

        SSL_write(ssl, response, response_size);
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

    if(argc == 4) {
        if(strcmp(argv[1], "-a") == 0) {
            auth_init("users.db");
            add_user(argv[2], argv[3]);
            exit(EXIT_SUCCESS);
        }
    }
    
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

    server_socket = init_server(&config);
    auth_init("users.db");

    SSL* ssl;

    while(1) {
        // Connnect with the client
        char msg[4096];
        ssl = get_request(server_socket, msg, 4096);
        if(ssl == NULL) {
            continue;
        }

#ifdef DEBUG
        fprintf(logfile, "Request:\n%s\n", msg);
#endif
        // Parse the request
        Request request = parseRequest(msg);

        if(check_auth(&request, ssl)) {
            // Get the response
            execute_git(request, config.repo_path, config.git_path, ssl);
        }

        // Free allocated memory and close the connection to the client
        int cs = SSL_get_fd(ssl);
        SSL_shutdown(ssl);
        close(cs);
        SSL_free(ssl);
        free_request(request);
    }

    auth_destroy();
    SSL_CTX_free(ctx);
    close(server_socket);
    destroyConfig(config);
#ifdef DEBUG
    fclose(logfile);
#endif
    return 0;
}