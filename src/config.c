#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// A function, that removes spaces from strings
void removeSpaces(char* str) {
    int count = 0;
    for(int i = 0; str[i]; i++) {
        if(str[i] != ' ' && str[i] != '\n') {
            str[count++] = str[i];
        }
    }
    str[count] = '\0';
}

// A function, that loads the configfile and puts its values into the struct
Config loadConfig(const char* path) {
    Config config;

    // Open file
    FILE* file = fopen(path, "r");
    if(!file) {
        fprintf(stderr, "Config not found: %s\n", path);
        exit(-1);
    };

    // Read every line
    char line[100];
    while(fgets(line, 100, file) != NULL) {

        // Exclude Comments
        if(line[0] == '#') {
            continue;
        } else if(strstr(line, "=") != NULL) {

            removeSpaces(line);
            char* name = strtok(line, "=");
            char* value = strtok(NULL, "=");

            // "repo_path"
            if(strcmp(name, "repo_path") == 0) {
                config.repo_path = malloc(strlen(value));
                strcpy(config.repo_path, value);

            // "port"
            } else if(strcmp(name, "port") == 0) {
                config.port = atoi(value);

                if(config.port == 0) {
                    fprintf(stderr, "Something's wrong with the specified port\n");
                    exit(-1);
                }

            // "git_path"
            } else if(strcmp(name, "git_path") == 0) {
                config.git_path = malloc(strlen(value));
                strcpy(config.git_path, value);
            }
        }
    }
    fclose(file);

    // Check if every required value is set
    if(!config.repo_path || !config.port) {
        fprintf(stderr, "Config not valid! Please fix it\n");
        exit(-1);
    }

    return config;
}

// Free all the values, that where allocated using malloc()
void destroyConfig(Config config) {
    free(config.repo_path);
    free(config.git_path);
}