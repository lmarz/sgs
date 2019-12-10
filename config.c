#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void removeSpaces(char* str) {
    int count = 0;
    for(int i = 0; str[i]; i++) {
        if(str[i] != ' ' && str[i] != '\n') {
            str[count++] = str[i];
        }
    }
    str[count] = '\0';
}

Config loadConfig(const char* path) {
    Config config;

    FILE* file = fopen(path, "r");
    if(!file) {
        fprintf(stderr, "Config not found: %s\n", path);
        exit(-1);
    };

    char line[100];
    while(fgets(line, 100, file) != NULL) {
        if(line[0] == '#') {
            continue;
        } else if(strstr(line, "=") != NULL) {
            removeSpaces(line);
            char* name = strtok(line, "=");
            char* value = strtok(NULL, "=");

            if(strcmp(name, "repo_path") == 0) {
                config.repo_path = malloc(strlen(value));
                strcpy(config.repo_path, value);
            } else if(strcmp(name, "port") == 0) {
                config.port = atoi(value);

                if(config.port == 0) {
                    fprintf(stderr, "Something's wrong with the specified port\n");
                    exit(-1);
                }
            }
        }
    }
    fclose(file);

    if(!config.repo_path || !config.port) {
        fprintf(stderr, "Config not valid! Please fix it\n");
        exit(-1);
    }

    return config;
}

void destroyConfig(Config config) {
    free(config.repo_path);
}